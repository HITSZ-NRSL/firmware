/**
 ******************************************************************************
  Copyright (c) 2013-2014 IntoRobot Team.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
  ******************************************************************************
*/

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "aes.h"
#include "cmac.h"
#include "mqttcrypto.h"
#include "string_convert.h"

/*!
 * AES computation context variable
 */
static aes_context AesContext;

/*!
 * CMAC computation context variable
 */
static AES_CMAC_CTX AesCmacCtx[1];


void MqttConnectComputeCmac( const uint8_t *buffer, uint16_t size, const uint8_t *key, uint8_t *cMac )
{
    AES_CMAC_Init( AesCmacCtx );
    AES_CMAC_SetKey( AesCmacCtx, key );
    AES_CMAC_Update( AesCmacCtx, buffer, size );
    AES_CMAC_Final( cMac, AesCmacCtx );
}

void MqttConnectComputeSKeys( const uint8_t *key, const int random, uint8_t *nwkSKey, uint8_t *appSKey )
{
    uint8_t nonce[16];
    uint8_t ramdom_array[4];

    ramdom_array[0] = ( random >> 24 ) & 0xFF;
    ramdom_array[1] = ( random >> 16 ) & 0xFF;
    ramdom_array[2] = ( random >> 8 ) & 0xFF;
    ramdom_array[3] = ( random ) & 0xFF;

    memset( nonce, 0, sizeof( nonce ) );
    nonce[0] = 0x01;
    hex2string(ramdom_array, sizeof(ramdom_array), (char *)&nonce[1], false);
    MqttConnectComputeCmac( nonce, 16, key, appSKey );

    memset( nonce, 0, sizeof( nonce ) );
    nonce[0] = 0x02;
    hex2string(ramdom_array, sizeof(ramdom_array), (char *)&nonce[1], false);
    MqttConnectComputeCmac( nonce, 16, key, nwkSKey );
}

void MqttComputeMic( const uint8_t *buffer, uint16_t size, const uint8_t *key, uint8_t *mic )
{
    uint8_t Mic[16];

    AES_CMAC_Init( AesCmacCtx );
    AES_CMAC_SetKey( AesCmacCtx, key );
    AES_CMAC_Update( AesCmacCtx, buffer, size );
    AES_CMAC_Final( Mic, AesCmacCtx );

    memcpy(mic, Mic, 4);
}

void MqttPayloadEncrypt( const uint8_t *buffer, uint16_t size, const uint8_t *key, uint8_t dir, uint16_t seqId, char *device_id, uint8_t *encBuffer )
{
    uint16_t i;
    uint8_t bufferIndex = 0;
    uint16_t ctr = 1;
    uint8_t aBlock[16] = {0};
    uint8_t sBlock[16] = {0};

    i = strlen(device_id);
    if(i < 10) {
        return;
    }
    memset( AesContext.ksch, '\0', 240 );
    aes_set_key1( key, 16, &AesContext );

    aBlock[0] = dir;
    aBlock[1] = ( seqId >> 8 ) & 0xFF;
    aBlock[2] = ( seqId ) & 0xFF;
    memcpy(&aBlock[3], &device_id[i-10], 10);

    while( size >= 16 ) {
        aBlock[14] = ( ctr >> 8 ) & 0xFF;
        aBlock[15] = ( ( ctr ) & 0xFF );
        ctr++;
        aes_encrypt1( aBlock, sBlock, &AesContext );
        for( i = 0; i < 16; i++ ) {
            encBuffer[bufferIndex + i] = buffer[bufferIndex + i] ^ sBlock[i];
        }
        size -= 16;
        bufferIndex += 16;
    }

    if( size > 0 ) {
        aBlock[14] = ( ctr >> 8 ) & 0xFF;
        aBlock[15] = ( ( ctr ) & 0xFF );
        aes_encrypt1( aBlock, sBlock, &AesContext );
        for( i = 0; i < size; i++ ) {
            encBuffer[bufferIndex + i] = buffer[bufferIndex + i] ^ sBlock[i];
        }
    }
}

void MqttPayloadDecrypt( const uint8_t *buffer, uint16_t size, const uint8_t *key, uint8_t dir, uint16_t seqId, char *device_id, uint8_t *encBuffer )
{
    MqttPayloadEncrypt( buffer, size, key, dir, seqId, device_id, encBuffer );
}

