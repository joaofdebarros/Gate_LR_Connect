/*
 * Crypto.h
 *
 *  Created on: 6 de mai. de 2026
 *      Author: joao.victor
 */

#ifndef API_CRYPTO_CRYPTO_H_
#define API_CRYPTO_CRYPTO_H_

#include "psa/crypto.h"
#include <string.h>
#include "ember-types.h"

psa_status_t aes_ecb_encrypt_key(
    const uint8_t *transport_key,    // 16 bytes
    const uint8_t *plaintext_key,    // 16 bytes
    uint8_t *encrypted_key           // 16 bytes (saída)
);

psa_status_t aes_ecb_decrypt_key(
    const uint8_t *transport_key,    // 16 bytes
    const uint8_t *encrypted_key,    // 16 bytes
    uint8_t *plaintext_key           // 16 bytes (saída)
);
#endif /* API_CRYPTO_CRYPTO_H_ */
