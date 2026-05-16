/*
 * SHA-256 implementation for TC234 Bootloader
 * Uses project-native types: uint8, uint32, uint64
 */
#include "crypto/sha256.h"

/* ========================================================================
 * Constants
 * ======================================================================== */
static const uint32 SHA256_K[64] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u,
    0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u,
    0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
    0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
    0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u,
    0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u,
    0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u,
    0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
    0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
};

static const uint32 SHA256_H[8] = {
    0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
    0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u
};

/* ========================================================================
 * Internal helpers
 * ======================================================================== */
#define ROTR(x, n)   (((x) >> (n)) | ((x) << (32u - (n))))
#define CH(x, y, z)  (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x)       (ROTR((x),  2u) ^ ROTR((x), 13u) ^ ROTR((x), 22u))
#define EP1(x)       (ROTR((x),  6u) ^ ROTR((x), 11u) ^ ROTR((x), 25u))
#define SIG0(x)      (ROTR((x),  7u) ^ ROTR((x), 18u) ^ ((x) >>  3u))
#define SIG1(x)      (ROTR((x), 17u) ^ ROTR((x), 19u) ^ ((x) >> 10u))

static void sha256_transform(uint32 state[8], const uint8 data[64])
{
    uint32 a, b, c, d, e, f, g, h;
    uint32 t1, t2;
    uint32 m[64];
    uint32 i;

    /* Prepare message schedule */
    for (i = 0u; i < 16u; i++) {
        m[i] = ((uint32)data[i * 4 + 0] << 24u) |
               ((uint32)data[i * 4 + 1] << 16u) |
               ((uint32)data[i * 4 + 2] <<  8u) |
               ((uint32)data[i * 4 + 3]       );
    }
    for (i = 16u; i < 64u; i++) {
        m[i] = SIG1(m[i - 2u]) + m[i - 7u] + SIG0(m[i - 15u]) + m[i - 16u];
    }

    a = state[0]; b = state[1]; c = state[2]; d = state[3];
    e = state[4]; f = state[5]; g = state[6]; h = state[7];

    for (i = 0u; i < 64u; i++) {
        t1 = h + EP1(e) + CH(e, f, g) + SHA256_K[i] + m[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

/* ========================================================================
 * Public API
 * ======================================================================== */
void sha256_init(sha256_ctx_t *ctx)
{
    uint8 i;
    for (i = 0u; i < 8u; i++) {
        ctx->state[i] = SHA256_H[i];
    }
    ctx->bitcount = 0u;
    ctx->buflen   = 0u;
}

void sha256_update(sha256_ctx_t *ctx, const uint8 *data, uint32 len)
{
    uint32 i;
    for (i = 0u; i < len; i++) {
        ctx->buffer[ctx->buflen] = data[i];
        ctx->buflen++;
        if (ctx->buflen == 64u) {
            sha256_transform(ctx->state, ctx->buffer);
            ctx->bitcount += 512u;
            ctx->buflen = 0u;
        }
    }
}

void sha256_final(sha256_ctx_t *ctx, uint8 hash[SHA256_HASH_SIZE])
{
    uint32 i;
    uint64 total_bits;
    uint32 orig_buflen = ctx->buflen;  /* record original data length before padding */

    /* Padding: append 0x80 */
    ctx->buffer[ctx->buflen] = 0x80u;
    ctx->buflen++;

    /* If remaining space < 8 bytes, pad to end of block and process */
    if (ctx->buflen > 56u) {
        while (ctx->buflen < 64u) {
            ctx->buffer[ctx->buflen] = 0x00u;
            ctx->buflen++;
        }
        sha256_transform(ctx->state, ctx->buffer);
        ctx->buflen = 0u;
    }

    /* Pad with zeros until 56 bytes */
    while (ctx->buflen < 56u) {
        ctx->buffer[ctx->buflen] = 0x00u;
        ctx->buflen++;
    }

    /* Append bit count (64 bits, big-endian) */
    total_bits = ctx->bitcount + ((uint64)orig_buflen * 8u);
    for (i = 0u; i < 8u; i++) {
        ctx->buffer[56u + i] = (uint8)(total_bits >> (56u - i * 8u));
    }

    sha256_transform(ctx->state, ctx->buffer);

    /* Output hash (big-endian) */
    for (i = 0u; i < 8u; i++) {
        hash[i * 4 + 0] = (uint8)(ctx->state[i] >> 24u);
        hash[i * 4 + 1] = (uint8)(ctx->state[i] >> 16u);
        hash[i * 4 + 2] = (uint8)(ctx->state[i] >>  8u);
        hash[i * 4 + 3] = (uint8)(ctx->state[i]       );
    }
}

void sha256(const uint8 *data, uint32 len, uint8 hash[SHA256_HASH_SIZE])
{
    sha256_ctx_t ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, data, len);
    sha256_final(&ctx, hash);
}
