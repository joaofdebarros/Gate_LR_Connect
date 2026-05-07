/*
 * Crypto.c
 *
 *  Created on: 6 de mai. de 2026
 *      Author: joao.victor
 */
#include "Crypto.h"

const uint8_t transport_key[16] = {
        0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
        0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C
};
/**
 * @brief Criptografa uma chave de 16 bytes usando AES-128-ECB
 *
 * @param transport_key Chave de transporte pré-compartilhada (16 bytes)
 * @param plaintext_key Chave em texto claro a ser enviada (16 bytes)
 * @param encrypted_key Buffer de saída para chave criptografada (16 bytes)
 * @return psa_status_t PSA_SUCCESS se bem-sucedido
 */
psa_status_t aes_ecb_encrypt_key(
    const uint8_t *transport_key,    // 16 bytes
    const uint8_t *plaintext_key,    // 16 bytes
    uint8_t *encrypted_key           // 16 bytes (saída)
)
{
    psa_status_t status;
    psa_key_id_t key_id = 0;
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    size_t output_length = 0;

    // 1. Configurar atributos da chave de transporte
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_ENCRYPT);
    psa_set_key_algorithm(&attributes, PSA_ALG_ECB_NO_PADDING);
    psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&attributes, 128);
    psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_VOLATILE); // Chave temporária

    // 2. Importar a chave de transporte
    status = psa_import_key(&attributes, transport_key, 16, &key_id);
    if (status != PSA_SUCCESS) {
        return status;
    }

    // 3. Criptografar usando AES-ECB (aceleração de hardware)
    status = psa_cipher_encrypt(
        key_id,                      // ID da chave
        PSA_ALG_ECB_NO_PADDING,      // Algoritmo ECB
        plaintext_key,               // Entrada (16 bytes)
        16,                          // Tamanho da entrada
        encrypted_key,               // Saída (16 bytes)
        16,                          // Tamanho do buffer de saída
        &output_length               // Bytes escritos (será 16)
    );

    // 4. Destruir a chave temporária (segurança)
    psa_destroy_key(key_id);

    return status;
}

/**
 * @brief Descriptografa uma chave de 16 bytes usando AES-128-ECB
 *
 * @param transport_key Chave de transporte pré-compartilhada (16 bytes)
 * @param encrypted_key Chave criptografada recebida (16 bytes)
 * @param plaintext_key Buffer de saída para chave descriptografada (16 bytes)
 * @return psa_status_t PSA_SUCCESS se bem-sucedido
 */
psa_status_t aes_ecb_decrypt_key(
    const uint8_t *transport_key,    // 16 bytes
    const uint8_t *encrypted_key,    // 16 bytes
    uint8_t *plaintext_key           // 16 bytes (saída)
)
{
    psa_status_t status;
    psa_key_id_t key_id = 0;
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    size_t output_length = 0;

    // 1. Configurar atributos da chave de transporte
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DECRYPT);
    psa_set_key_algorithm(&attributes, PSA_ALG_ECB_NO_PADDING);
    psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&attributes, 128);
    psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_VOLATILE);

    // 2. Importar a chave de transporte
    status = psa_import_key(&attributes, transport_key, 16, &key_id);
    if (status != PSA_SUCCESS) {
        return status;
    }

    // 3. Descriptografar usando AES-ECB (aceleração de hardware)
    status = psa_cipher_decrypt(
        key_id,                      // ID da chave
        PSA_ALG_ECB_NO_PADDING,      // Algoritmo ECB
        encrypted_key,               // Entrada criptografada (16 bytes)
        16,                          // Tamanho da entrada
        plaintext_key,               // Saída descriptografada (16 bytes)
        16,                          // Tamanho do buffer de saída
        &output_length               // Bytes escritos (será 16)
    );

    // 4. Destruir a chave temporária
    psa_destroy_key(key_id);

    return status;
}

