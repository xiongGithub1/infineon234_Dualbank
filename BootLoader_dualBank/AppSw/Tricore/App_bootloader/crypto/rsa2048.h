#ifndef CRYPTO_RSA2048_H_
#define CRYPTO_RSA2048_H_

#include "Platform_Types.h"

#define RSA2048_MODULUS_LEN   (256u)   /* 2048 bits */
#define RSA2048_EXPONENT_LEN  (4u)     /* 65537 */
#define RSA2048_SIG_LEN       (256u)
#define RSA2048_HASH_LEN      (32u)    /* SHA-256 */

/*
 * RSA-2048 signature verification with PKCS#1 v1.5 padding.
 *
 * @param signature   256-byte signature (big-endian)
 * @param hash        32-byte SHA-256 hash to verify against
 * @param modulus     256-byte RSA modulus n (big-endian)
 * @param exponent    4-byte RSA public exponent e (big-endian, usually 0x00010001)
 * @return 1 if signature valid, 0 otherwise
 */
uint8 rsa2048_verify(const uint8 *signature,
                     const uint8 *hash,
                     const uint8 *modulus,
                     const uint8 *exponent);

#endif /* CRYPTO_RSA2048_H_ */
