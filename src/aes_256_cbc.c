#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "esp32/aes.h"
#include "aes_256_cbc.h"

/* 
  For  Encryption time: 1802.40us  (9.09 MB/s) at 16kB blocks.
*/

static inline int32_t _getCycleCount(void)
{
    int32_t ccount;
    __asm__ __volatile__("rsr %0,ccount"
                         : "=a"(ccount));
    return ccount;
}
char iv[16];
char key[32];
char plaintext[4096];
char encrypted[4096];
esp_aes_context ctx;
esp_err_t encrypt(char *plain_text,char *_key,char *init_vector)
{
    // uint8_t key[32];
    // uint8_t iv[16];
    memset(iv, 0, sizeof(iv));
    //Right now, I am using a key of all zeroes.  This should change.  You should fill the key
    //out with actual data.
    memset(key, 0, sizeof(key));
    memset(plaintext, 0, sizeof(plaintext));
    memcpy((char *)key, _key, 32);
    memcpy((char *)iv, init_vector, 16);
    strcpy(plaintext, plain_text);
    //Just FYI - you must be encrypting/decrypting data that is in BLOCKSIZE chunks!!!
    esp_aes_context ctx;
    esp_aes_init(&ctx);
    esp_aes_setkey(&ctx, (uint8_t *)key, 256);
    int32_t start = _getCycleCount();
    esp_aes_crypt_cbc(&ctx, ESP_AES_ENCRYPT, sizeof(plaintext), (uint8_t *)iv, (uint8_t *)plaintext, (uint8_t *)encrypted);
    int32_t end = _getCycleCount();
    float enctime = (end - start) / 240.0;
    printf("Encryption time: %.2fus  (%f MB/s)\n", enctime, (sizeof(plaintext) * 1.0) / enctime);
    //See encrypted payload, and wipe out plaintext.
    memset(plaintext, 0, sizeof(plaintext));
    int i;
    for (i = 0; i < 128; i++)
    {
        //printf( "%02x[%c]%c", encrypted[i], (encrypted[i]>31)?encrypted[i]:' ', ((i&0xf)!=0xf)?' ':'\n' );
        printf("%02x", encrypted[i]);
    }
    printf("\n");
    //Must reset IV.
    //XXX TODO: Research further: I found out if you don't reset the IV, the first block will fail
    //but subsequent blocks will pass.  Is there some strange cryptoalgebra going on that permits this?
    //printf("IV: %02x\n", iv[0]);
    memset(iv, 0, sizeof(iv));
    memcpy((char *)iv, init_vector, 16);
    //Use the ESP32 to decrypt the CBC block.
    esp_aes_crypt_cbc(&ctx, ESP_AES_DECRYPT, sizeof(encrypted), (uint8_t *)iv, (uint8_t *)encrypted, (uint8_t *)plaintext);
    printf("%s", "Decrypt : ");
    //Verify output
    for (i = 0; i < strlen(plaintext); i++)
    {
        //printf( "%02x[%c]%c", plaintext[i],  (plaintext[i]>31)?plaintext[i]:' ', ((i&0xf)!=0xf)?' ':'\n' );
        printf("%c", (plaintext[i] > 31) ? plaintext[i] : ' ');
        if (plaintext[i] != plain_text[i]) return ESP_FAIL;
    }
    printf("\n");
    esp_aes_free(&ctx);
    return ESP_OK;
}
