#ifndef CRYPTO_SHA256_H_
#define CRYPTO_SHA256_H_

#include "Platform_Types.h"

#define SHA256_BLOCK_SIZE   (64u)
#define SHA256_HASH_SIZE    (32u)

typedef struct
{
    uint32 state[8];
    uint64 bitcount;
    uint8  buffer[SHA256_BLOCK_SIZE];
    uint32 buflen;
} sha256_ctx_t;

void sha256_init(sha256_ctx_t *ctx);
void sha256_update(sha256_ctx_t *ctx, const uint8 *data, uint32 len);
void sha256_final(sha256_ctx_t *ctx, uint8 hash[SHA256_HASH_SIZE]);

/* One-shot helper */
void sha256(const uint8 *data, uint32 len, uint8 hash[SHA256_HASH_SIZE]);

#endif /* CRYPTO_SHA256_H_ */
