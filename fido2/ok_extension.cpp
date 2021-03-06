/* Tim Steiner
 * Copyright (c) 2015-2018, CryptoTrust LLC.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by CryptoTrust LLC. for
 *    the OnlyKey Project (http://www.crp.to/ok)"
 *
 * 4. The names "OnlyKey" and "OnlyKey Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    admin@crp.to.
 *
 * 5. Products derived from this software may not be called "OnlyKey"
 *    nor may "OnlyKey" or "CryptoTrust" appear in their names without
 *    specific prior written permission. For written permission, please
 *    contact admin@crp.to.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by CryptoTrust LLC. for
 *    the OnlyKey Project (http://www.crp.to/ok)"
 *
 * 7. Redistributions in any form must be accompanied by information on
 *    how to obtain complete source code for this software and any
 *    accompanying software that uses this software. The source code
 *    must either be included in the distribution or be available for
 *    no more than the cost of distribution plus a nominal fee, and must
 *    be freely redistributable under reasonable conditions. For a
 *    binary file, complete source code means the source code for all
 *    modules it contains.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS
 * ARE GRANTED BY THIS LICENSE. IF SOFTWARE RECIPIENT INSTITUTES PATENT
 * LITIGATION AGAINST ANY ENTITY (INCLUDING A CROSS-CLAIM OR COUNTERCLAIM
 * IN A LAWSUIT) ALLEGING THAT THIS SOFTWARE (INCLUDING COMBINATIONS OF THE
 * SOFTWARE WITH OTHER SOFTWARE OR HARDWARE) INFRINGES SUCH SOFTWARE
 * RECIPIENT'S PATENT(S), THEN SUCH SOFTWARE RECIPIENT'S RIGHTS GRANTED BY
 * THIS LICENSE SHALL TERMINATE AS OF THE DATE SUCH LITIGATION IS FILED. IF
 * ANY PROVISION OF THIS AGREEMENT IS INVALID OR UNENFORCEABLE UNDER
 * APPLICABLE LAW, IT SHALL NOT AFFECT THE VALIDITY OR ENFORCEABILITY OF THE
 * REMAINDER OF THE TERMS OF THIS AGREEMENT, AND WITHOUT FURTHER ACTION
 * BY THE PARTIES HERETO, SUCH PROVISION SHALL BE REFORMED TO THE MINIMUM
 * EXTENT NECESSARY TO MAKE SUCH PROVISION VALID AND ENFORCEABLE. ALL
 * SOFTWARE RECIPIENT'S RIGHTS UNDER THIS AGREEMENT SHALL TERMINATE IF IT
 * FAILS TO COMPLY WITH ANY OF THE MATERIAL TERMS OR CONDITIONS OF THIS
 * AGREEMENT AND DOES NOT CURE SUCH FAILURE IN A REASONABLE PERIOD OF
 * TIME AFTER BECOMING AWARE OF SUCH NONCOMPLIANCE. THIS SOFTWARE IS
 * PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR  PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,  EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "device.h"
#include "onlykey.h"
#ifdef STD_VERSION
#include "log.h"
#include "wallet.h"
//#include APP_CONFIG
#include "util.h"
#include "storage.h"
#include "ctap.h"
#include "ctap_errors.h"
#include "crypto.h"
#include "u2f.h"
#include "extensions.h"
#include "ok_extension.h"

#define DERIVE_PUBLIC_KEY 1
#define DERIVE_SHARED_SECRET 2
#define NO_ENCRYPT_RESP 0
#define ENCRYPT_RESP 1

extern uint8_t* large_resp_buffer;
extern int large_resp_buffer_offset;
extern uint8_t profilemode;
extern uint8_t isfade;
extern uint8_t NEO_Color;
extern uint8_t type;
extern uint8_t CRYPTO_AUTH;
extern int outputmode;
extern uint8_t ecc_public_key[(MAX_ECC_KEY_SIZE*2)+1];
extern uint8_t ecc_private_key[MAX_ECC_KEY_SIZE];
extern uint8_t recv_buffer[64];
extern uint8_t pending_operation;
extern int packet_buffer_offset;
extern uint8_t packet_buffer_details[5];


int16_t bridge_to_onlykey(uint8_t * _appid, uint8_t * keyh, int handle_len, uint8_t * output)
{

    int8_t ret = 0;
	uint8_t client_handle[256];
	handle_len-=10;
	uint8_t cmd = keyh[0];
	uint8_t opt1 = keyh[1];
	uint8_t opt2 = keyh[2];
	uint8_t opt3 = keyh[3];
	uint8_t browser;
	uint8_t os;
	uint8_t temp[256];

	memcpy(client_handle, keyh+10, handle_len);
		
	#ifdef DEBUG
    Serial.println("Keyhandle:");
    byteprint(client_handle, handle_len);
	#endif

    if (webcryptcheck(_appid, client_handle)) {
      outputmode=DISCARD; // Discard output 
		if (cmd == OKCONNECT && !CRYPTO_AUTH) {
			large_buffer_offset = 0;
			set_time(client_handle);
			memset(ecc_public_key, 0, sizeof(ecc_public_key));
			// crypto_box_keypair uses RNG2 to create random 32 byte private
			// crypto_box_keypair puts generated private in ecc_private_key and public in ecc_public_key along with OnlyKey version info
			crypto_box_keypair(ecc_public_key+sizeof(UNLOCKED), ecc_private_key); //Generate keys
			#ifdef DEBUG
			Serial.println("OnlyKey public = ");
			byteprint(ecc_public_key+sizeof(UNLOCKED), 32);
			#endif
			memcpy(ecc_public_key, UNLOCKED, sizeof(UNLOCKED));
			// Response goes out via WEBAUTHN
			outputmode=WEBAUTHN;
			memcpy(temp, ecc_public_key, sizeof(ecc_public_key));
			// Check opt1 to see if additional info goes in response
			if (opt1==DERIVE_PUBLIC_KEY) {
				//TODO HKDF
				// Test public key value all 1s
				memset(temp+sizeof(ecc_public_key), 1, 32);
				send_transport_response(temp, sizeof(ecc_public_key)+32, false, false); //Encrypt if opt3 and send right away
			} else if (opt1==DERIVE_SHARED_SECRET) {
				//TODO HKDF
				//Input RPID, Derivation Private, Optional additional data
				//if (shared_secret (ecc_public_key, shared)) {
				//	ret = CTAP2_ERR_OPERATION_DENIED;
				//	printf2(TAG_ERR,"Error with ECC Shared Secret\n");
				//	return ret;
				//}

				// Test public key value all 1s
				memset(temp+sizeof(ecc_public_key), 1, 32);
				// Test shared secret value all 2s
				memset(temp+sizeof(ecc_public_key)+32, 2, 32);
				send_transport_response(temp, sizeof(ecc_public_key)+32+32, opt3, false); //Encrypt if opt3 and send right away
			} else {
				send_transport_response (ecc_public_key, 32+sizeof(UNLOCKED), opt3, false); //Don't encrypt and send right away
			}
			memcpy(ecc_public_key, client_handle+9, 32); //Get app public key
			browser = client_handle[9+32];
			os = client_handle[9+32+1];
			#ifdef DEBUG
			Serial.println("App public = ");
			byteprint(ecc_public_key, 32);
			Serial.print("Browser = ");
			Serial.println((char)browser);
			Serial.print("OS = ");
			Serial.println((char)os);
			#endif
			uint8_t shared[32];
			type = 1;
			if (shared_secret (ecc_public_key, shared)) {
			ret = CTAP2_ERR_OPERATION_DENIED;
			printf2(TAG_ERR,"Error with ECC Shared Secret\n");
			return ret;
			}
			#ifdef DEBUG
			Serial.println("Shared Secret = ");
			byteprint(shared, 32);
			#endif
			SHA256_CTX context;
			sha256_init(&context);
			sha256_update(&context, shared, 32);
			sha256_final(&context, ecc_private_key);
			#ifdef DEBUG
			Serial.println("AES Key = ");
			byteprint(ecc_private_key, 32);
			#endif
			pending_operation=CTAP2_ERR_DATA_READY;
		} else if (webcryptcheck(_appid, client_handle)>1) {  // Protected mode, only allow crp.to and localhost
			//Todo add localhost support
			aes_crypto_box (client_handle, handle_len, true);
			#ifdef DEBUG
			Serial.println("Decrypted client handle");
			byteprint(client_handle, handle_len);
			Serial.println("Received FIDO2 request to send data to OnlyKey");
			#endif

			if (cmd == OKPING) { //Ping
				outputmode=WEBAUTHN;
				if(!CRYPTO_AUTH && !large_resp_buffer_offset) {
					#ifdef DEBUG
					Serial.println("Error incorrect challenge was entered");
					#endif
					hidprint("Error incorrect challenge was entered");
				} else {
					#ifdef DEBUG
					Serial.println("Sending stored data from ping request");
					#endif
				}
			}
			// Break the FIDO message into packets
			else if (!CRYPTO_AUTH) {
				int i=0;
				if (!packet_buffer_details[3]) packet_buffer_details[3] = opt3; // first packet
				else if (opt3 <= packet_buffer_details[3]) return 0; // duplicate packet, thanks to win 10 1903 sending all FIDO2 messages twice

				while(handle_len>0) { // Max size packet minus header
					memset(recv_buffer, 0, sizeof(recv_buffer));
					if (handle_len>=57) memmove(recv_buffer+7, client_handle+(i*57), 57);
					else memmove(recv_buffer+7, client_handle+(i*57), handle_len);
					memset(recv_buffer, 0xFF, 4);
					recv_buffer[4] = cmd;
					recv_buffer[5] = opt1; //slot
					recv_buffer[6] = 0xFF;
					if (opt2 && handle_len<=57) recv_buffer[6] = handle_len; // last packet
					if (cmd == OKDECRYPT) {
						packet_buffer_details[3] = opt3;
						NEO_Color = 128; //Turquoise
						large_buffer_offset = 0;
						outputmode=WEBAUTHN;
						#ifdef DEBUG
						Serial.println("OKDECRYPT Chunk");
						byteprint(recv_buffer, 64);
						#endif
						DECRYPT(recv_buffer);
					} else if (cmd == OKSIGN) {
						packet_buffer_details[3] = opt3;
						NEO_Color = 213; //Purple
						large_buffer_offset = 0;
						outputmode=WEBAUTHN;
						#ifdef DEBUG
						Serial.println("OKSIGN Chunk");
						byteprint(recv_buffer, 64);
						#endif
						SIGN(recv_buffer);
					}
					handle_len-=57;
					i++;
				}
				ret = 0;
				}
		}
		ret = send_stored_response(output);
		return ret;
			
		//if (!isfade) fadeon(NEO_Color);
	} 

    ret = CTAP2_ERR_EXTENSION_NOT_SUPPORTED; //APPID doesn't match
    wipedata();
    return ret;
}

int16_t send_stored_response(uint8_t * output) {
  int16_t ret = 0;
	if(profilemode!=NONENCRYPTEDPROFILE) {
		#ifdef DEBUG
		Serial.println("Sending data on OnlyKey via Webauthn");
		byteprint(large_resp_buffer, large_resp_buffer_offset);
		Serial.println(large_resp_buffer_offset);
		#endif
    // Check if large response is ready
		if (pending_operation==CTAP2_ERR_OPERATION_PENDING) {
			#ifdef DEBUG
			Serial.print("CTAP2_ERR_OPERATION_PENDING");
			#endif
			ret = CTAP2_ERR_OPERATION_PENDING;
		} else if (large_resp_buffer_offset) {
			extension_writeback_init(output, large_resp_buffer_offset);
			extension_writeback(large_resp_buffer, large_resp_buffer_offset);
			// Windows 10 1903 bug, it sends every fido2 request/response twice
			// Everything happens twice, and the computer only pays attention to the 2nd request/response.
			// This means we can't wipe the response after it's retrieved, have to wipe
			// based on a timer
			//memset(large_resp_buffer, 0, LARGE_RESP_BUFFER_SIZE);
			wipedata(); // Wipe timer started
		} else if (CRYPTO_AUTH || packet_buffer_offset) {
			#ifdef DEBUG
			Serial.println("Ping success");
			#endif
			memset(large_resp_buffer, 0, LARGE_RESP_BUFFER_SIZE);
			ret = CTAP2_ERR_USER_ACTION_PENDING;
		} else if (!CRYPTO_AUTH) {
			#ifdef DEBUG
			Serial.print("Error no data ready to be retrieved");
			#endif
			//custom_error(6);
      		ret = CTAP2_ERR_NO_OPERATION_PENDING;
			fadeoff(1);
		}
		return ret; 
	}
}

#endif
