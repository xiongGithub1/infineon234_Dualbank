/**********************************************************************************************************************
 * \file    rsa2048.c
 * \brief   Minimal RSA-2048 signature verification for TC234 Bootloader
 *          - Public exponent hard-coded assumption: e = 65537 (0x10001)
 *          - Uses schoolbook multiplication + long division
 *          - No dynamic allocation, no recursion
 *********************************************************************************************************************/

#include "rsa2048.h"

#define RSA_WORDS   (64u)   /* 2048 / 32 */
#define RSA_DWORDS  (128u)  /* 4096 / 32, for multiplication result */

/* ============================================================================
 *  Big-integer helpers (uint32 words, little-endian in array)
 * ============================================================================ */

static void be_to_le(const uint8 *be, uint32 *le, uint32 word_count)
{
    uint32 i;
    for (i = 0; i < word_count; i++)
    {
        uint32 idx = (word_count - 1 - i) * 4;
        le[i] = ((uint32)be[idx] << 24) |
                ((uint32)be[idx + 1] << 16) |
                ((uint32)be[idx + 2] << 8) |
                ((uint32)be[idx + 3]);
    }
}

static void le_to_be(const uint32 *le, uint8 *be, uint32 word_count)
{
    uint32 i;
    for (i = 0; i < word_count; i++)
    {
        uint32 idx = (word_count - 1 - i) * 4;
        be[idx]     = (uint8)(le[i] >> 24);
        be[idx + 1] = (uint8)(le[i] >> 16);
        be[idx + 2] = (uint8)(le[i] >> 8);
        be[idx + 3] = (uint8)(le[i]);
    }
}

static void bn_cpy(uint32 *dst, const uint32 *src, uint32 n)
{
    uint32 i;
    for (i = 0; i < n; i++) dst[i] = src[i];
}

static void bn_set_zero(uint32 *a, uint32 n)
{
    uint32 i;
    for (i = 0; i < n; i++) a[i] = 0;
}

/* result = a * b, result has a_words + b_words elements */
static void bn_mul(const uint32 *a, uint32 a_words,
                   const uint32 *b, uint32 b_words,
                   uint32 *result)
{
    uint32 i, j;
    bn_set_zero(result, a_words + b_words);
    for (i = 0; i < a_words; i++)
    {
        uint64 carry = 0;
        for (j = 0; j < b_words; j++)
        {
            uint64 product = (uint64)a[i] * b[j] + result[i + j] + carry;
            result[i + j] = (uint32)product;
            carry = product >> 32;
        }
        /* Propagate carry through remaining result words */
        uint32 k = i + b_words;
        while (carry && k < a_words + b_words)
        {
            uint64 sum = (uint64)result[k] + carry;
            result[k] = (uint32)sum;
            carry = sum >> 32;
            k++;
        }
    }
}

/* r = a mod m
 * Simple bit-by-bit reduction (correct but slower than Knuth).
 * For RSA-2048 with e=65517, total verification is ~100-300ms on 100MHz TriCore.
 */
static void bn_mod(const uint32 *a, uint32 a_words,
                   const uint32 *m, uint32 m_words,
                   uint32 *r)
{
    uint32 rem[RSA_DWORDS + 1];
    uint32 i;
    int a_bits, m_bits, bit;

    /* Initialize remainder buffer to zero */
    bn_set_zero(rem, RSA_DWORDS + 1);

    /* Find highest set bit in a */
    a_bits = a_words * 32 - 1;
    while (a_bits >= 0 && ((a[a_bits / 32] >> (a_bits % 32)) & 1) == 0)
        a_bits--;

    /* Find highest set bit in m */
    m_bits = m_words * 32 - 1;
    while (m_bits >= 0 && ((m[m_bits / 32] >> (m_bits % 32)) & 1) == 0)
        m_bits--;

    /* If a < m, just copy */
    if (a_bits < m_bits)
    {
        bn_set_zero(r, m_words);
        for (i = 0; i < a_words; i++) r[i] = a[i];
        return;
    }

    /* Bit-by-bit reduction */
    for (bit = a_bits; bit >= 0; bit--)
    {
        /* rem = (rem << 1) | current_bit_of_a */
        uint32 carry = 0;
        for (i = 0; i < RSA_DWORDS + 1; i++)
        {
            uint32 new_carry = rem[i] >> 31;
            rem[i] = (rem[i] << 1) | carry;
            carry = new_carry;
        }
        rem[0] |= (a[bit / 32] >> (bit % 32)) & 1;

        /* If rem >= m, subtract m */
        int cmp = 0;
        if (rem[m_words] > 0)
        {
            cmp = 1;
        }
        else
        {
            for (i = m_words; i > 0; i--)
            {
                if (rem[i - 1] > m[i - 1]) { cmp = 1; break; }
                if (rem[i - 1] < m[i - 1]) { cmp = -1; break; }
            }
        }

        if (cmp >= 0)
        {
            uint64 borrow = 0;
            for (i = 0; i < m_words; i++)
            {
                uint64 diff = (uint64)rem[i] - m[i] - borrow;
                rem[i] = (uint32)diff;
                borrow = (diff >> 63) & 1;
            }
            /* Propagate borrow to higher words */
            i = m_words;
            while (borrow && i < RSA_DWORDS + 1)
            {
                uint64 diff = (uint64)rem[i] - borrow;
                rem[i] = (uint32)diff;
                borrow = (diff >> 63) & 1;
                i++;
            }
        }
    }

    /* Copy result */
    bn_set_zero(r, m_words);
    for (i = 0; i < m_words; i++) r[i] = rem[i];
}

/* result = base^exp mod mod
 * exp_bits: number of bits in exponent (e.g. 17 for 65537)
 */
static void bn_modexp(const uint32 *base, const uint32 *exp, uint32 exp_bits,
                      const uint32 *mod, uint32 *result)
{
    uint32 i;
    uint32 sq[RSA_DWORDS];
    uint32 prod[RSA_DWORDS];
    uint32 t[RSA_DWORDS];

    /* result = 1 */
    bn_set_zero(result, RSA_WORDS);
    result[0] = 1;

    /* Copy base -> sq */
    bn_set_zero(sq, RSA_WORDS);
    for (i = 0; i < RSA_WORDS; i++) sq[i] = base[i];

    for (i = 0; i < exp_bits; i++)
    {
        uint32 bit = (exp[i / 32] >> (i % 32)) & 1;

        if (bit)
        {
            /* result = result * sq mod mod */
            bn_mul(result, RSA_WORDS, sq, RSA_WORDS, prod);
            bn_mod(prod, RSA_WORDS * 2, mod, RSA_WORDS, t);
            bn_cpy(result, t, RSA_WORDS);
        }

        /* sq = sq * sq mod mod */
        bn_mul(sq, RSA_WORDS, sq, RSA_WORDS, prod);
        bn_mod(prod, RSA_WORDS * 2, mod, RSA_WORDS, t);
        bn_cpy(sq, t, RSA_WORDS);
    }
}

/* ============================================================================
 *  PKCS#1 v1.5 padding verification
 * ============================================================================ */

/* pkcs1v15_verify */
static uint8 pkcs1v15_verify(const uint8 *em, uint32 em_len,
                             const uint8 *hash, uint32 hash_len)
{
    uint32 i;
    uint32 ps_end;
    static const uint8 sha256_digest_info[] = {
        0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86,
        0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05,
        0x00, 0x04, 0x20
    };
    #define DI_LEN (19u)

    /* Minimum length: 1 + 1 + 8 + 1 + DI_LEN + hash_len = 11 + 19 + 32 = 62 */
    if (em_len < (11 + DI_LEN + hash_len))
        return 0;

    /* EM[0] = 0x00, EM[1] = 0x01 */
    if (em[0] != 0x00 || em[1] != 0x01)
        return 0;

    /* Padding string: all 0xFF
     * ps_end points to the separator 0x00.
     * EM layout: 0x00 || 0x01 || PS || 0x00 || DigestInfo || Hash
     * Total: 2 + PS_len + 1 + DI_LEN + hash_len = em_len
     * Therefore: ps_end (separator index) = em_len - 1 - DI_LEN - hash_len
     */
    ps_end = em_len - 1u - DI_LEN - hash_len;
    for (i = 2; i < ps_end; i++)
    {
        if (em[i] != 0xFF)
            return 0;
    }

    /* Separator: 0x00 */
    if (em[ps_end] != 0x00)
        return 0;

    /* DigestInfo */
    for (i = 0; i < DI_LEN; i++)
    {
        if (em[ps_end + 1 + i] != sha256_digest_info[i])
            return 0;
    }

    /* Hash */
    for (i = 0; i < hash_len; i++)
    {
        if (em[ps_end + 1 + DI_LEN + i] != hash[i])
            return 0;
    }

    return 1;
}

/* ============================================================================
 *  Public API
 * ============================================================================ */

uint8 rsa2048_verify(const uint8 *signature,
                     const uint8 *hash,
                     const uint8 *modulus,
                     const uint8 *exponent)
{
    uint32 sig_le[RSA_WORDS];
    uint32 mod_le[RSA_WORDS];
    uint32 exp_le[1];
    uint32 result_le[RSA_WORDS];
    uint8  result_be[RSA2048_SIG_LEN];

    /* Convert inputs to little-endian word arrays */
    be_to_le(signature, sig_le, RSA_WORDS);
    be_to_le(modulus, mod_le, RSA_WORDS);
    exp_le[0] = ((uint32)exponent[0] << 24) |
                ((uint32)exponent[1] << 16) |
                ((uint32)exponent[2] << 8) |
                ((uint32)exponent[3]);

    /* Modular exponentiation: result = sig^e mod n */
    bn_modexp(sig_le, exp_le, 17, mod_le, result_le);  /* 65537 = 2^16 + 1 */

    /* Convert back to big-endian */
    le_to_be(result_le, result_be, RSA_WORDS);

    /* Verify PKCS#1 v1.5 padding */
    return pkcs1v15_verify(result_be, RSA2048_SIG_LEN, hash, RSA2048_HASH_LEN);
}
