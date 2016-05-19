/* okcore.cpp
*/

/*
 * Modifications by Tim Steiner
 * Copyright (c) 2016 , CryptoTrust LLC.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 *Original 
 *Copyright (c) 2015, Yohanes Nugroho
 *All rights reserved.
 *
 *Redistribution and use in source and binary forms, with or without
 *modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 *Redistributions in binary form must reproduce the above copyright notice,
 *this list of conditions and the following disclaimer in the documentation
 *and/or other materials provided with the distribution.
 *
 *THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 *FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <EEPROM.h>
#include <Time.h>
#include "sha256.h"
#include "flashkinetis.h"
#include "uecc.h"
#include <password.h>
#include "ykcore.h"
#include "yksim.h"
#include "onlykey.h"
#include "T3MacLib.h"
#include <Crypto.h>
#include <RNG.h>
#include <transistornoisesource.h>
#include <AES.h>
#include <GCM.h>

uint32_t unixTimeStamp;
const int ledPin = 13;
int PINSET = 0;
bool unlocked = false;
bool initialized = false;
bool PDmode = false;

//U2F Assignments
/*************************************/
byte expected_next_packet;
int large_data_len;
int large_data_oflashset;
byte large_buffer[1024];
byte large_resp_buffer[1024];
byte recv_buffer[64];
byte resp_buffer[64];
byte handle[64];
byte sha256_hash[32];
/*************************************/

//Password.cpp Assignments
/*************************************/
Password password = Password( "not used" );
extern uint8_t phash[32];
extern uint8_t sdhash[32];
extern uint8_t pdhash[32];
extern uint8_t nonce[32];
/*************************************/

const char attestation_key[] = "\xf3\xfc\xcc\x0d\x00\xd8\x03\x19\x54\xf9"
  "\x08\x64\xd4\x3c\x24\x7f\x4b\xf5\xf0\x66\x5c\x6b\x50\xcc"
  "\x17\x74\x9a\x27\xd1\xcf\x76\x64";

const char attestation_der[] = "\x30\x82\x01\x3c\x30\x81\xe4\xa0\x03\x02"
  "\x01\x02\x02\x0a\x47\x90\x12\x80\x00\x11\x55\x95\x73\x52"
  "\x30\x0a\x06\x08\x2a\x86\x48\xce\x3d\x04\x03\x02\x30\x17"
  "\x31\x15\x30\x13\x06\x03\x55\x04\x03\x13\x0c\x47\x6e\x75"
  "\x62\x62\x79\x20\x50\x69\x6c\x6f\x74\x30\x1e\x17\x0d\x31"
  "\x32\x30\x38\x31\x34\x31\x38\x32\x39\x33\x32\x5a\x17\x0d"
  "\x31\x33\x30\x38\x31\x34\x31\x38\x32\x39\x33\x32\x5a\x30"
  "\x31\x31\x2f\x30\x2d\x06\x03\x55\x04\x03\x13\x26\x50\x69"
  "\x6c\x6f\x74\x47\x6e\x75\x62\x62\x79\x2d\x30\x2e\x34\x2e"
  "\x31\x2d\x34\x37\x39\x30\x31\x32\x38\x30\x30\x30\x31\x31"
  "\x35\x35\x39\x35\x37\x33\x35\x32\x30\x59\x30\x13\x06\x07"
  "\x2a\x86\x48\xce\x3d\x02\x01\x06\x08\x2a\x86\x48\xce\x3d"
  "\x03\x01\x07\x03\x42\x00\x04\x8d\x61\x7e\x65\xc9\x50\x8e"
  "\x64\xbc\xc5\x67\x3a\xc8\x2a\x67\x99\xda\x3c\x14\x46\x68"
  "\x2c\x25\x8c\x46\x3f\xff\xdf\x58\xdf\xd2\xfa\x3e\x6c\x37"
  "\x8b\x53\xd7\x95\xc4\xa4\xdf\xfb\x41\x99\xed\xd7\x86\x2f"
  "\x23\xab\xaf\x02\x03\xb4\xb8\x91\x1b\xa0\x56\x99\x94\xe1"
  "\x01\x30\x0a\x06\x08\x2a\x86\x48\xce\x3d\x04\x03\x02\x03"
  "\x47\x00\x30\x44\x02\x20\x60\xcd\xb6\x06\x1e\x9c\x22\x26"
  "\x2d\x1a\xac\x1d\x96\xd8\xc7\x08\x29\xb2\x36\x65\x31\xdd"
  "\xa2\x68\x83\x2c\xb8\x36\xbc\xd3\x0d\xfa\x02\x20\x63\x1b"
  "\x14\x59\xf0\x9e\x63\x30\x05\x57\x22\xc8\xd8\x9b\x7f\x48"
  "\x88\x3b\x90\x89\xb8\x8d\x60\xd1\xd9\x79\x59\x02\xb3\x04"
  "\x10\xdf";

//key handle: (private key + app parameter) ^ this array
//TODO generate this from root key
const char handlekey[] = "-YOHANES-NUGROHO-YOHANES-NUGROHO-";

const struct uECC_Curve_t * curve = uECC_secp256r1(); //P-256
uint8_t private_k[36]; //32
uint8_t public_k[68]; //64

struct ch_state {
  int cid;
  byte state;
  int last_millis;
};

ch_state channel_states[MAX_CHANNEL];



void cleanup_timeout()
{
  int i;
  for (i = 0;  i < MAX_CHANNEL; i++) {
    //free channel that is inactive
    ch_state &c = channel_states[i];
    int m = millis();
    if (c.state != STATE_CHANNEL_AVAILABLE) {
      if ((m - c.last_millis) > TIMEOUT_VALUE) {
        c.state = STATE_CHANNEL_AVAILABLE;
      }
    }
  }
}

int allocate_new_channel()
{
  int i;
  //alloace new channel_id
  int channel_id = 1;
  int retry = 2;
  do {
    bool found = false;
    for (i = 0;  i < MAX_CHANNEL; i++) {
      if (channel_states[i].state != STATE_CHANNEL_AVAILABLE) {
        if (channel_states[i].cid == channel_id) {
          found = true;
          channel_id++;
          break;
        }
      }
    }
    if (!found)
      break;
  } while (true);
  return channel_id;
}

int allocate_channel(int channel_id)
{
  int i;
  if (channel_id==0) {
    channel_id =  allocate_new_channel();
  }

  bool has_free_slots = false;
  for (i = 0;  i < MAX_CHANNEL; i++) {
    if (channel_states[i].state == STATE_CHANNEL_AVAILABLE) {
      has_free_slots = true;
      break;
    }
  }
  if (!has_free_slots)
    cleanup_timeout();

  for (i = 0;  i < MAX_CHANNEL; i++) {
    ch_state &c = channel_states[i];
    if (c.state == STATE_CHANNEL_AVAILABLE) {
      c.cid = channel_id;
      c.state = STATE_CHANNEL_WAIT_PACKET;
      c.last_millis = millis();
      return channel_id;
    }
  }
  return 0;
}

int initResponse(byte *buffer)
{
#ifdef DEBUG
  Serial.print("INIT RESPONSE");
#endif
  int cid = *(int*)buffer;
#ifdef DEBUG
  Serial.println(cid, HEX);
#endif
  int len = buffer[5] << 8 | buffer[6];
  int i;
  memcpy(resp_buffer, buffer, 5);
  SET_MSG_LEN(resp_buffer, 17);
  memcpy(resp_buffer + 7, buffer + 7, len); //nonce
  i = 7 + len;
  if (cid==-1) {
    cid = allocate_channel(0);
  } else {
#ifdef DEBUG
    Serial.println("using existing CID");
#endif
    allocate_channel(cid);
  }
  memcpy(resp_buffer + i, &cid, 4);
  i += 4;
  resp_buffer[i++] = U2FHID_IF_VERSION;
  resp_buffer[i++] = 1; //major
  resp_buffer[i++] = 0;
  resp_buffer[i++] = 1; //build
  //resp_buffer[i++] = CAPABILITY_WINK; //capabilities
  resp_buffer[i++] = 0; //capabilities
#ifdef DEBUG
  Serial.println("SENT RESPONSE 1");
#endif  
  RawHID.send(resp_buffer, 100);
#ifdef DEBUG
  Serial.println(cid, HEX);
#endif  
  return cid;
}


void errorResponse(byte *buffer, int code)
{
        memcpy(resp_buffer, buffer, 4);
        resp_buffer[4] = U2FHID_ERROR;
        SET_MSG_LEN(resp_buffer, 1);
        resp_buffer[7] = code & 0xff;
#ifdef DEBUG
  Serial.print("SENT RESPONSE error:");
  Serial.println(code);
#endif
  RawHID.send(resp_buffer, 100);
}


//find channel index and update last access
int find_channel_index(int channel_id)
{
  int i;

  for (i = 0;  i < MAX_CHANNEL; i++) {
    if (channel_states[i].cid==channel_id) {
      channel_states[i].last_millis = millis();
      return i;
    }
  }

  return -1;
}





void respondErrorPDU(byte *buffer, int err)
{
  SET_MSG_LEN(buffer, 2); //len("") + 2 byte SW
  byte *datapart = buffer + 7;
  APPEND_SW(datapart, (err >> 8) & 0xff, err & 0xff);
  RawHID.send(buffer, 100);
}

void sendLargeResponse(byte *request, int len)
{
#ifdef DEBUG  
  Serial.print("Sending large response ");
  Serial.println(len);
  for (int i = 0; i < len; i++) {
    Serial.print(large_resp_buffer[i], HEX);
    Serial.print(" ");
  }
  Serial.println("\n--\n");
#endif  
  memcpy(resp_buffer, request, 4); //copy cid
  resp_buffer[4] = U2FHID_MSG;
  int r = len;
  if (r>MAX_INITIAL_PACKET) {
    r = MAX_INITIAL_PACKET;
  }

  SET_MSG_LEN(resp_buffer, len);
  memcpy(resp_buffer + 7, large_resp_buffer, r);

  RawHID.send(resp_buffer, 100);
  len -= r;
  byte p = 0;
  int oflashset = MAX_INITIAL_PACKET;
  while (len > 0) {
    //memcpy(resp_buffer, request, 4); //copy cid, doesn't need to recopy
    resp_buffer[4] = p++;
    memcpy(resp_buffer + 5, large_resp_buffer + oflashset, MAX_CONTINUATION_PACKET);
    RawHID.send(resp_buffer, 100);
    len-= MAX_CONTINUATION_PACKET;
    oflashset += MAX_CONTINUATION_PACKET;
    delayMicroseconds(2500);
  }
}



int getCounter() {
  unsigned int eeAddress = EEpos_U2Fcounter; //EEPROM address to start reading from
  unsigned int counter;
  EEPROM.get( eeAddress, counter );
  return counter;
}

void setCounter(int counter)
{
  unsigned int eeAddress = EEpos_U2Fcounter; //EEPROM address to start reading from
  EEPROM.put( eeAddress, counter );
}


int u2f_button = 0;


void processMessage(byte *buffer)
{
  int len = buffer[5] << 8 | buffer[6];
#ifdef DEBUG  
  Serial.println(F("Got message"));
  Serial.println(len);
  Serial.println(F("Data:"));
#endif  
  byte *message = buffer + 7;
#ifdef DEBUG
  for (int i = 7; i < 7+len; i++) {
    Serial.print(buffer[i], HEX);
  }
  Serial.println(F(""));
#endif  
  //todo: check CLA = 0
  byte CLA = message[0];

  if (CLA!=0) {
    respondErrorPDU(buffer, SW_CLA_NOT_SUPPORTED);
    return;
  }

  byte INS = message[1];
  byte P1 = message[2];
  byte P2 = message[3];
  int reqlength = (message[4] << 16) | (message[5] << 8) | message[6];

  switch (INS) {
  case U2F_INS_REGISTER:
    {
      if (reqlength!=64) {
        respondErrorPDU(buffer, SW_WRONG_LENGTH);
        return;
      }

   
      if (u2f_button==0) {
        respondErrorPDU(buffer, SW_CONDITIONS_NOT_SATISFIED);
        }
      else if (u2f_button==1) {
          Serial.println("U2F button pressed for register");
        return;
      }
    

      byte *datapart = message + 7;
      byte *challenge_parameter = datapart;
      byte *application_parameter = datapart+32;

      memset(public_k, 0, sizeof(public_k));
      memset(private_k, 0, sizeof(private_k));
      uECC_make_key(public_k + 1, private_k, curve); //so we ca insert 0x04
      public_k[0] = 0x04;
#ifdef DEBUG
      Serial.println(F("Public K"));
      for (int i =0; i < sizeof(public_k); i++) {
        Serial.print(public_k[i], HEX);
        Serial.print(" ");
      }
      Serial.println("");
      Serial.println(F("Private K"));
      for (int i =0; i < sizeof(private_k); i++) {
        Serial.print(private_k[i], HEX);
        Serial.print(" ");
      }
      Serial.println("");
#endif      
      //construct hash

      memcpy(handle, application_parameter, 32);
      memcpy(handle+32, private_k, 32);
      for (int i =0; i < 64; i++) {
        handle[i] ^= handlekey[i%(sizeof(handlekey)-1)];
      }

      SHA256_CTX ctx;
      sha256_init(&ctx);
      large_resp_buffer[0] = 0x00;
      sha256_update(&ctx, large_resp_buffer, 1);
#ifdef DEBUG      
      Serial.println(F("App Parameter:"));
      for (int i =0; i < 32; i++) {
        Serial.print(application_parameter[i], HEX);
        Serial.print(" ");
      }
      Serial.println("");
#endif
      sha256_update(&ctx, application_parameter, 32);
#ifdef DEBUG
      Serial.println(F("Chal Parameter:"));
      for (int i =0; i < 32; i++) {
        Serial.print(challenge_parameter[i], HEX);
        Serial.print(" ");
      }
      Serial.println("");
#endif
      sha256_update(&ctx, challenge_parameter, 32);
#ifdef DEBUG
      Serial.println(F("Handle Parameter:"));
      for (int i =0; i < 64; i++) {
        Serial.print(handle[i], HEX);
        Serial.print(" ");
      }
      Serial.println("");
#endif
      sha256_update(&ctx, handle, 64);
      sha256_update(&ctx, public_k, 65);
#ifdef DEBUG      
      Serial.println(F("Public key:"));
      for (int i =0; i < 65; i++) {
        Serial.print(public_k[i], HEX);
        Serial.print(" ");
      }
      Serial.println("");
#endif
      sha256_final(&ctx, sha256_hash);
#ifdef DEBUG
      Serial.println(F("Hash:"));
      for (int i =0; i < 32; i++) {
        Serial.print(sha256_hash[i], HEX);
        Serial.print(" ");
      }
      Serial.println("");
#endif

      uint8_t *signature = resp_buffer; //temporary
      
      //TODO add uECC_sign_deterministic need to create *hash_context
      uECC_sign((uint8_t *)attestation_key,
          sha256_hash,
          32,
          signature,
          curve);

      int len = 0;
      large_resp_buffer[len++] = 0x05;
      memcpy(large_resp_buffer + len, public_k, 65);
      len+=65;
      large_resp_buffer[len++] = 64; //length of handle
      memcpy(large_resp_buffer+len, handle, 64);
      len += 64;
      memcpy(large_resp_buffer+len, attestation_der, sizeof(attestation_der));
      len += sizeof(attestation_der)-1;
      //convert signature format
      //http://bitcoin.stackexchange.com/questions/12554/why-the-signature-is-always-65-13232-bytes-long
      large_resp_buffer[len++] = 0x30; //header: compound structure
      large_resp_buffer[len++] = 0x44; //total length (32 + 32 + 2 + 2)
      large_resp_buffer[len++] = 0x02;  //header: integer
      large_resp_buffer[len++] = 32;  //32 byte
      memcpy(large_resp_buffer+len, signature, 32); //R value
      len +=32;
      large_resp_buffer[len++] = 0x02;  //header: integer
      large_resp_buffer[len++] = 32;  //32 byte
      memcpy(large_resp_buffer+len, signature+32, 32); //R value
      len +=32;

      byte *last = large_resp_buffer+len;
      APPEND_SW_NO_ERROR(last);
      len += 2;
    
      u2f_button = 0;
     
      sendLargeResponse(buffer, len);
    }

    break;
  case U2F_INS_AUTHENTICATE:
    {

      //minimum is 64 + 1 + 64
      if (reqlength!=(64+1+64)) {
        respondErrorPDU(buffer, SW_WRONG_LENGTH);
        return;
      }

      byte *datapart = message + 7;
      byte *challenge_parameter = datapart;
      byte *application_parameter = datapart+32;
      byte handle_len = datapart[64];
      byte *client_handle = datapart+65;

      if (handle_len!=64) {
        //not from this device
        respondErrorPDU(buffer, SW_WRONG_DATA);
        return;
      }
     
      if (u2f_button==0) {
        respondErrorPDU(buffer, SW_CONDITIONS_NOT_SATISFIED);
        }
      else if (u2f_button==1) { 
                    Serial.println("U2F button pressed for authenticate");
        return;
      }

      memcpy(handle, client_handle, 64);
      for (int i =0; i < 64; i++) {
        handle[i] ^= handlekey[i%(sizeof(handlekey)-1)];
      }
      uint8_t *key = handle + 32;

      if (memcmp(handle, application_parameter, 32)!=0) {
        //this handle is not from us
        respondErrorPDU(buffer, SW_WRONG_DATA);
        return;
      }

      if (P1==0x07) { //check-only
        respondErrorPDU(buffer, SW_CONDITIONS_NOT_SATISFIED);
      } else if (P1==0x03) { //enforce-user-presence-and-sign
        int counter = getCounter();
        SHA256_CTX ctx;
        sha256_init(&ctx);
        sha256_update(&ctx, application_parameter, 32);
        large_resp_buffer[0] = 0x01; // user_presence

        int ctr = ((counter>>24)&0xff) | // move byte 3 to byte 0
          ((counter<<8)&0xff0000) | // move byte 1 to byte 2
          ((counter>>8)&0xff00) | // move byte 2 to byte 1
          ((counter<<24)&0xff000000); // byte 0 to byte 3

        memcpy(large_resp_buffer + 1, &ctr, 4);

        sha256_update(&ctx, large_resp_buffer, 5); //user presence + ctr

        sha256_update(&ctx, challenge_parameter, 32);
        sha256_final(&ctx, sha256_hash);

        uint8_t *signature = resp_buffer; //temporary

        //TODO add uECC_sign_deterministic need to create *hash_context
        uECC_sign((uint8_t *)key,
            sha256_hash,
            32,
            signature,
            curve);

        int len = 5;

        //convert signature format
        //http://bitcoin.stackexchange.com/questions/12554/why-the-signature-is-always-65-13232-bytes-long
        large_resp_buffer[len++] = 0x30; //header: compound structure
        large_resp_buffer[len++] = 0x44; //total length (32 + 32 + 2 + 2)
        large_resp_buffer[len++] = 0x02;  //header: integer
        large_resp_buffer[len++] = 32;  //32 byte
        memcpy(large_resp_buffer+len, signature, 32); //R value
        len +=32;
        large_resp_buffer[len++] = 0x02;  //header: integer
        large_resp_buffer[len++] = 32;  //32 byte
        memcpy(large_resp_buffer+len, signature+32, 32); //R value
        len +=32;
        byte *last = large_resp_buffer+len;
        APPEND_SW_NO_ERROR(last);
        len += 2;
#ifdef DEBUG
        Serial.print("Len to send ");
        Serial.println(len);
#endif
           
        u2f_button = 0;
      
        sendLargeResponse(buffer, len);

        setCounter(counter+1);
      } else {
        //return error
      }
    }
    break;
  case U2F_INS_VERSION:
    {
      if (reqlength!=0) {
        respondErrorPDU(buffer, SW_WRONG_LENGTH);
        return;
      }
      //reuse input buffer for sending
      SET_MSG_LEN(buffer, 8); //len("U2F_V2") + 2 byte SW
      byte *datapart = buffer + 7;
      memcpy(datapart, "U2F_V2", 6);
      datapart += 6;
      APPEND_SW_NO_ERROR(datapart);
      RawHID.send(buffer, 100);
    }
    break;
  default:
    {
      respondErrorPDU(buffer, SW_INS_NOT_SUPPORTED);
    }
    ;
  }

}

void processPacket(byte *buffer)
{
#ifdef DEBUG  
  Serial.print("Process CMD ");
#endif
  char cmd = buffer[4]; //cmd or continuation
#ifdef DEBUG
  Serial.println((int)cmd, HEX);
#endif

  int len = buffer[5] << 8 | buffer[6];

  if (cmd > U2FHID_INIT || cmd==U2FHID_LOCK) {
    errorResponse(recv_buffer, ERR_INVALID_CMD);
    return;
  }
  if (cmd==U2FHID_PING) {
    if (len <= MAX_INITIAL_PACKET) {
#ifdef DEBUG      
      Serial.println("Sending ping response");
#endif      
      RawHID.send(buffer, 100);
    } else {
      //large packet
      //send first one
#ifdef DEBUG      
      Serial.println("SENT RESPONSE 3");
#endif      
      RawHID.send(buffer, 100);
      len -= MAX_INITIAL_PACKET;
      byte p = 0;
      int oflashset = 7 + MAX_INITIAL_PACKET;
      while (len > 0) {
        memcpy(resp_buffer, buffer, 4); //copy cid
        resp_buffer[4] = p++;
        memcpy(resp_buffer + 5, buffer + oflashset, MAX_CONTINUATION_PACKET);
        RawHID.send(resp_buffer, 100);
        len-= MAX_CONTINUATION_PACKET;
        oflashset += MAX_CONTINUATION_PACKET;
        delayMicroseconds(2500);
      }
#ifdef DEBUG      
      Serial.println("Sending large ping response");
#endif      
    }
  }
  if (cmd==U2FHID_MSG) {
    processMessage(buffer);
  }

}

void setOtherTimeout()
{
  //we can process the data
  //but if we find another channel is waiting for continuation, we set it as timeout
  for (int i = 0; i < MAX_CHANNEL; i++) {
    if (channel_states[i].state==STATE_CHANNEL_WAIT_CONT) {
#ifdef DEBUG      
      Serial.println("Set other timeout");
#endif      
      channel_states[i].state= STATE_CHANNEL_TIMEOUT;
    }
  }

}

int cont_start = 0;

void recvmsg() {
  int n;
  int c;
  int z;
  
  //Serial.print("");
  n = RawHID.recv(recv_buffer, 0); // 0 timeout = do not wait
#ifdef DEBUG   
/*
    Serial.print(F("U2F Counter = "));
    EEPROM.get(0, c );
     delay(100);
    Serial.println(c);
    */
#endif 
  if (n > 0) {
#ifdef DEBUG    
    Serial.print(F("\n\nReceived packet"));
    for (z=0; z<64; z++) {
        Serial.print(recv_buffer[z], HEX);
    }
	
	   //Support for additional vendor defined commands
char cmd_or_cont = recv_buffer[4]; //cmd or continuation

  switch (cmd_or_cont) {
      case OKSETPIN:
      SETPIN(recv_buffer);
      return;
      break;
      case OKSETSDPIN:
      SETSDPIN(recv_buffer);
      return;
      break;
      case OKSETPDPIN:
      SETPDPIN(recv_buffer);
      return;
      break;
      case OKSETTIME:
      SETTIME(recv_buffer);
      return;
      break;
      case OKGETLABELS:
	   if(initialized==false && unlocked==true) 
	   {
		hidprint("No PIN set, You must set a PIN first");
		return;
	   }else if (initialized==true && unlocked==true) 
	   {
		GETLABELS(recv_buffer);
	   }
	   else
	   {
	   hidprint("ERROR DEVICE LOCKED");
	   return;
	   }	
      return;
      break;
      case OKSETSLOT:
	   if(initialized==false && unlocked==true) 
	   {
		hidprint("No PIN set, You must set a PIN first");
		return;
	   }else if (initialized==true && unlocked==true) 
	   {
		SETSLOT(recv_buffer);
	   }
	   else
	   {
	   hidprint("ERROR DEVICE LOCKED");
	   return;
	   }
      return;
      break;
      case OKWIPESLOT:
	   if(initialized==false && unlocked==true) 
	   {
		hidprint("No PIN set, You must set a PIN first");
		return;
	   }else if (initialized==true && unlocked==true) 
	   {
		WIPESLOT(recv_buffer);
	   }
	   else
	   {
	   hidprint("ERROR DEVICE LOCKED");
	   return;
	   }	
      return;
      break;
      case OKSETU2FPRIV:
	   if(initialized==false && unlocked==true) 
	   {
		hidprint("No PIN set, You must set a PIN first");
		return;
	   }else if (initialized==true && unlocked==true) 
	   {
		SETU2FPRIV(recv_buffer);
	   }
	   else
	   {
	   hidprint("ERROR DEVICE LOCKED");
	   return;
	   }	
      return;
      break;
      case OKWIPEU2FPRIV:
	   if(initialized==false && unlocked==true) 
	   {
		hidprint("No PIN set, You must set a PIN first");
		return;
	   }else if (initialized==true && unlocked==true) 
	   {
		WIPEU2FPRIV(recv_buffer);
	   }
	   else
	   {
	   hidprint("ERROR DEVICE LOCKED");
	   return;
	   }	
      break;
      case OKSETU2FCERT:
	   if(initialized==false && unlocked==true) 
	   {
		hidprint("No PIN set, You must set a PIN first");
		return;
	   }else if (initialized==true && unlocked==true) 
	   {
		SETU2FCERT(recv_buffer);
	   }
	   else
	   {
	   hidprint("ERROR DEVICE LOCKED");
	   return;
	   }	
      return;
      break;
      case OKWIPEU2FCERT:
	   if(initialized==false && unlocked==true) 
	   {
		hidprint("No PIN set, You must set a PIN first");
		return;
	   }else if (initialized==true && unlocked==true) 
	   {
		WIPEU2FCERT(recv_buffer);
	   }
	   else
	   {
	   hidprint("ERROR DEVICE LOCKED");
	   return;
	   }	
      return;
      break;
      default: 
      break;
    }

	
#endif    
    int cid = *(int*)recv_buffer;
#ifdef DEBUG    
    Serial.println(cid, HEX);
#endif    
    if (cid==0) {
     #ifdef DEBUG 
     Serial.println("Invalid CID 0");
     #endif 
      errorResponse(recv_buffer, ERR_INVALID_CID);
      return;
    }

    int len = (recv_buffer[5]) << 8 | recv_buffer[6];

#ifdef DEBUG
    if (IS_NOT_CONTINUATION_PACKET(cmd_or_cont)) {
      Serial.print(F("LEN "));
      Serial.println((int)len);
    }
#endif
 
    //don't care about cid
    if (cmd_or_cont==U2FHID_INIT) {
      setOtherTimeout();
      cid = initResponse(recv_buffer);
      int cidx = find_channel_index(cid);
      channel_states[cidx].state= STATE_CHANNEL_WAIT_PACKET;
      return;
    }

    if (cid==-1) {
      #ifdef DEBUG 
      Serial.println("Invalid CID -1");
      #endif 
      errorResponse(recv_buffer, ERR_INVALID_CID);
      return;
    }

    int cidx = find_channel_index(cid);

    if (cidx==-1) {
#ifdef DEBUG      
      Serial.println("allocating new CID");
#endif      
      allocate_channel(cid);
      cidx = find_channel_index(cid);
      if (cidx==-1) {
        errorResponse(recv_buffer, ERR_INVALID_CID);
        return;
      }

    }

    if (IS_NOT_CONTINUATION_PACKET(cmd_or_cont)) {

      if (len > MAX_TOTAL_PACKET) {
        #ifdef DEBUG 
        Serial.println("Invalid Length");
        #endif 
        errorResponse(recv_buffer, ERR_INVALID_LEN); //invalid length
        return;
      }

      if (len > MAX_INITIAL_PACKET) {
        //if another channel is waiting for continuation, we respond with busy
        for (int i = 0; i < MAX_CHANNEL; i++) {
          if (channel_states[i].state==STATE_CHANNEL_WAIT_CONT) {
            if (i==cidx) {
              #ifdef DEBUG 
              Serial.println("Invalid Sequence");
              #endif 
              errorResponse(recv_buffer, ERR_INVALID_SEQ); //invalid sequence
              channel_states[i].state= STATE_CHANNEL_WAIT_PACKET;
            } else {
              #ifdef DEBUG 
              Serial.println("Channel Busy");
              #endif 
              errorResponse(recv_buffer, ERR_CHANNEL_BUSY);
              return;
            }

            return;
          }
        }
        //no other channel is waiting
        channel_states[cidx].state=STATE_CHANNEL_WAIT_CONT;
        cont_start = millis();
        memcpy(large_buffer, recv_buffer, 64);
        large_data_len = len;
        large_data_oflashset = MAX_INITIAL_PACKET;
        expected_next_packet = 0;
        return;
      }

      setOtherTimeout();
      processPacket(recv_buffer);
      channel_states[cidx].state= STATE_CHANNEL_WAIT_PACKET;
    } else {

      if (channel_states[cidx].state!=STATE_CHANNEL_WAIT_CONT) {
#ifdef DEBUG        
        Serial.println("ignoring stray packet");
        Serial.println(cid, HEX);
#endif        
        return;
      }

      //this is a continuation
      if (cmd_or_cont != expected_next_packet) {
        errorResponse(recv_buffer, ERR_INVALID_SEQ); //invalid sequence
        #ifdef DEBUG 
        Serial.println("Invalid Sequence");
        #endif 
        channel_states[cidx].state= STATE_CHANNEL_WAIT_PACKET;
        return;
      } else {

        memcpy(large_buffer + large_data_oflashset + 7, recv_buffer + 5, MAX_CONTINUATION_PACKET);
        large_data_oflashset += MAX_CONTINUATION_PACKET;

        if (large_data_oflashset < large_data_len) {
          expected_next_packet++;
#ifdef DEBUG          
          Serial.println("Expecting next cont");
#endif          
          return;
        }
#ifdef DEBUG        
        Serial.println("Completed");
#endif        
        channel_states[cidx].state= STATE_CHANNEL_WAIT_PACKET;
        processPacket(large_buffer);
        return;
      }
    }
  } else {

    for (int i = 0; i < MAX_CHANNEL; i++) {
      if (channel_states[i].state==STATE_CHANNEL_TIMEOUT) {
#ifdef DEBUG        
        Serial.println("send timeout");
        Serial.println(channel_states[i].cid, HEX);
#endif        
        memcpy(recv_buffer, &channel_states[i].cid, 4);
        errorResponse(recv_buffer, ERR_MSG_TIMEOUT);
        #ifdef DEBUG 
        Serial.println("Message Timeout");
        #endif 
        channel_states[i].state= STATE_CHANNEL_WAIT_PACKET;

      }
      if (channel_states[i].state==STATE_CHANNEL_WAIT_CONT) {

        int now = millis();
        if ((now - channel_states[i].last_millis)>500) {
#ifdef DEBUG          
          Serial.println("SET timeout");
#endif          
          channel_states[i].state=STATE_CHANNEL_TIMEOUT;
        }
      }
    }
  }
}



void SETPIN (byte *buffer)
{
      Serial.println("OKSETPIN MESSAGE RECEIVED");
	  
switch (PINSET) {
      case 0:
      Serial.println("Enter PIN");
      PINSET = 1;
      return;
      case 1:
      PINSET = 2;
      if(strlen(password.guess) > 6 && strlen(password.guess) < 11)
      {
        Serial.println("Storing PIN");
		static char passguess[10];
      for (int i =0; i <= strlen(password.guess); i++) {
		passguess[i] = password.guess[i];
      }
		password.set(passguess);
        password.reset();
      }
      else
      {
        
		Serial.println("Error PIN is not between 7 - 10 characters");
		hidprint("Error PIN is not between 7 - 10 characters");
        password.reset();
		PINSET = 0;
      }
      
      return;
      case 2:
      Serial.println("Confirm PIN");
      PINSET = 3;
      return;
      case 3:
	  PINSET = 0;
       if(strlen(password.guess) >= 7 && strlen(password.guess) < 11)
      {
	  
          if (password.evaluate()) {
            Serial.println("Both PINs Match");
			uint8_t temp[32];
			uint8_t *ptr;
			ptr = temp;
			//Copy characters to byte array
			for (int i =0; i <= strlen(password.guess); i++) {
			temp[i] = (byte)password.guess[i];
			}
			SHA256_CTX pinhash;
			sha256_init(&pinhash);
			sha256_update(&pinhash, temp, strlen(password.guess)); //Add new PIN to hash
			getrng(ptr, 32); //Fill temp with random data
			Serial.println("Generating NONCE");
			onlykey_flashset_noncehash (ptr); //Store in eeprom
			
			sha256_update(&pinhash, temp, 32); //Add nonce to hash
			sha256_final(&pinhash, temp); //Create hash and store in temp
			Serial.println("Hashing PIN and storing to EEPROM");
			onlykey_flashset_pinhash (ptr);

	  		initialized = true;
	  		Serial.println();
			Serial.println("Successfully set PIN, you must remove OnlyKey and reinsert to configure");
			hidprint("Successfully set PIN, you must remove OnlyKey and reinsert to configure");
            password.reset();
          }
          else {
            Serial.println("Error PINs Don't Match");
			hidprint("Error PINs Don't Match");
            password.reset();
			PINSET = 0;
          }
      }
      else
      {
        Serial.println("Error PIN is not between 7 - 10 characters");
		hidprint("Error PIN is not between 7 - 10 characters");
        password.reset();
		PINSET = 0;
      }
      
      return;
      
      blink(3);
      return;
}
}

void SETSDPIN (byte *buffer)
{
      Serial.println("OKSETSDPIN MESSAGE RECEIVED");
	  
	switch (PINSET) {
      case 0:
      Serial.println("Enter PIN");
      PINSET = 4;
      return;
      case 4:
	  PINSET = 5;
      if(strlen(password.guess) >= 7 && strlen(password.guess) < 11)
      {
        Serial.println("Storing PIN");
		static char passguess[10];
      for (int i =0; i <= strlen(password.guess); i++) {
		passguess[i] = password.guess[i];
      }
		password.set(passguess);
        password.reset();
      }
      else
      {
        
		Serial.println("Error PIN is not between 7 - 10 characters");
		hidprint("Error PIN is not between 7 - 10 characters");
        password.reset();
		PINSET = 0;
      }
      
      return;
      case 5:
      Serial.println("Confirm PIN");
      PINSET = 6;
      return;
      case 6:
	  PINSET = 0;
       if(strlen(password.guess) >= 7 && strlen(password.guess) < 11)
      {
	  
          if (password.evaluate() == true) {
            Serial.println("Both PINs Match");
			uint8_t temp[32];
			uint8_t *ptr;
			ptr = temp;
			//Copy characters to byte array
			for (int i =0; i <= strlen(password.guess); i++) {
			temp[i] = (byte)password.guess[i];
			}
			SHA256_CTX pinhash;
			sha256_init(&pinhash);
			sha256_update(&pinhash, temp, strlen(password.guess)); //Add new PIN to hash
			Serial.println("Getting NONCE");
			onlykey_flashget_noncehash (ptr, 32); //Store in eeprom
			
			sha256_update(&pinhash, temp, 32); //Add nonce to hash
			sha256_final(&pinhash, temp); //Create hash and store in temp
			Serial.println("Hashing SDPIN and storing to EEPROM");
			onlykey_flashset_selfdestructhash (ptr);
			Serial.print(F("SDPIN Hash:")); //TODO remove debug
      for (int i =0; i < 32; i++) {
        Serial.print(temp[i], HEX);
      }
	  
			hidprint("Successfully set PIN, you must remove OnlyKey and reinsert to configure");
            password.reset();
          }
          else {
            Serial.println("Error PINs Don't Match");
			hidprint("Error PINs Don't Match");
            password.reset();
			PINSET = 0;
          }
      }
      else
      {
        Serial.println("Error PIN is not between 7 - 10 characters");
		hidprint("Error PIN is not between 7 - 10 characters");
        password.reset();
		PINSET = 0;
      }
      
      return;
      
      blink(3);
      return;
}
}

void SETPDPIN (byte *buffer)
{
if (PDmode) return;
      Serial.println("OKSETPDPIN MESSAGE RECEIVED");
	  
	switch (PINSET) {
      case 0:
      Serial.println("Enter PIN");
      PINSET = 7;
      return;
      case 7:
	  PINSET = 8;
      if(strlen(password.guess) >= 7 && strlen(password.guess) < 11)
      {
        Serial.println("Storing PIN");
		static char passguess[10];
      for (int i =0; i <= strlen(password.guess); i++) {
		passguess[i] = password.guess[i];
      }
		password.set(passguess);
        password.reset();
      }
      else
      {
        
		Serial.println("Error PIN is not between 7 - 10 characters");
		hidprint("Error PIN is not between 7 - 10 characters");
        password.reset();
		PINSET = 0;
      }
      
      return;
      case 8:
      Serial.println("Confirm PIN");
      PINSET = 9;
      return;
      case 9:
	  PINSET = 0;
       if(strlen(password.guess) >= 7 && strlen(password.guess) < 11)
      {
	  
          if (password.evaluate() == true) {
            Serial.println("Both PINs Match");
			uint8_t temp[32];
			uint8_t *ptr;
			ptr = temp;
			//Copy characters to byte array
			for (int i =0; i <= strlen(password.guess); i++) {
			temp[i] = (byte)password.guess[i];
			}
			SHA256_CTX pinhash;
			sha256_init(&pinhash);
			sha256_update(&pinhash, temp, strlen(password.guess)); //Add new PIN to hash
			Serial.println("Getting NONCE");
			onlykey_flashget_noncehash (ptr, 32); //Store in eeprom
			
			sha256_update(&pinhash, temp, 32); //Add nonce to hash
			sha256_final(&pinhash, temp); //Create hash and store in temp
			Serial.println("Hashing PDPIN and storing to EEPROM");
			onlykey_flashset_plausdenyhash (ptr);
			Serial.print(F("PDPIN Hash:")); //TODO remove debug
      for (int i =0; i < 32; i++) {
        Serial.print(temp[i], HEX);
      }
	  
			hidprint("Successfully set PDPIN, you must remove OnlyKey and reinsert to configure");
            password.reset();
          }
          else {
            Serial.println("Error PINs Don't Match");
			hidprint("Error PINs Don't Match");
            password.reset();
			PINSET = 0;
          }
      }
      else
      {
        Serial.println("Error PIN is not between 7 - 10 characters");
		hidprint("Error PIN is not between 7 - 10 characters");
        password.reset();
		PINSET = 0;
      }
      
      return;
      
      blink(3);
      return;
}
}

void SETTIME (byte *buffer)
{
      Serial.println();
	  Serial.println("OKSETTIME MESSAGE RECEIVED");        
	   if(initialized==false && unlocked==true) 
	   {
		Serial.print("UNINITIALIZED");
		hidprint("UNINITIALIZED");
		return;
	   }else if (initialized==true && unlocked==true) 
	   {
		Serial.print("UNLOCKED");
		hidprint("UNLOCKED");
	   
    int i, j;                
    for(i=0, j=3; i<4; i++, j--){
    unixTimeStamp |= ((uint32_t)buffer[j + 5] << (i*8) );
    Serial.println(buffer[j+5], HEX);
    }
                      
      time_t t2 = unixTimeStamp;
      Serial.print(F("Received Unix Epoch Time: "));
      Serial.println(unixTimeStamp, HEX); 
      setTime(t2); 
      Serial.print(F("Current Time Set to: "));
      digitalClockDisplay();  
	  }
	  else
	  {
	    Serial.print("FLASH ERROR");
		factorydefault();
	  }
      RawHID.send(resp_buffer, 0);
      blink(3);
      return;
}

void GETLABELS (byte *buffer)
{
      Serial.println();
	  Serial.println("OKGETLABELS MESSAGE RECEIVED");
	  uint8_t label[EElen_label+2];
	  uint8_t *ptr;
	  char labelchar[EElen_label+2];
	  int offset  = 0;
	  ptr=label+2;
	  delay(20);
	  if (PDmode) offset = 12;
	  
	  onlykey_eeget_label(ptr, (offset + 1));
	  ByteToChar(label, labelchar, EElen_label);
	  label[0] = (byte)0x01;
	  label[1] = (byte)0x7C;
	  hidprint(labelchar);
	  delay(20);
	  
	  onlykey_eeget_label(ptr, (offset   + 2));
	  ByteToChar(label, labelchar, EElen_label);
	  label[0] = (byte)0x02;
	  label[1] = (byte)0x7C;
      	  hidprint(labelchar);
      	  delay(20);
	  
	  onlykey_eeget_label(ptr, (offset  + 3));
	  ByteToChar(label, labelchar, EElen_label);
	  label[0] = (byte)0x03;
	  label[1] = (byte)0x7C;
          hidprint(labelchar);
          delay(20);
	  
	  onlykey_eeget_label(ptr, (offset  + 4));
	  ByteToChar(label, labelchar, EElen_label);
	  label[0] = (byte)0x04;
	  label[1] = (byte)0x7C;
          hidprint(labelchar);
          delay(20);
	  
	  onlykey_eeget_label(ptr, (offset  + 5));
	  ByteToChar(label, labelchar, EElen_label);
	  label[0] = (byte)0x05;
	  label[1] = (byte)0x7C;
          hidprint(labelchar);
          delay(20);
	  
	  onlykey_eeget_label(ptr, (offset  + 6));
	  ByteToChar(label, labelchar, EElen_label);
	  label[0] = (byte)0x06;
	  label[1] = (byte)0x7C;
          hidprint(labelchar);
          delay(20);
	  
	  onlykey_eeget_label(ptr, (offset  + 7));
	  ByteToChar(label, labelchar, EElen_label);
	  label[0] = (byte)0x07;
	  label[1] = (byte)0x7C;
          hidprint(labelchar);
          delay(20);
	  
	  onlykey_eeget_label(ptr, (offset  + 8));
	  ByteToChar(label, labelchar, EElen_label);
	  label[0] = (byte)0x08;
	  label[1] = (byte)0x7C;
          hidprint(labelchar);
          delay(20);
	  
	  onlykey_eeget_label(ptr, (offset  + 9));
	  ByteToChar(label, labelchar, EElen_label);
	  label[0] = (byte)0x09;
	  label[1] = (byte)0x7C;
          hidprint(labelchar);
          delay(20);
	  
	  onlykey_eeget_label(ptr, (offset  + 10));
	  ByteToChar(label, labelchar, EElen_label);
	  label[0] = (byte)0x10;
	  label[1] = (byte)0x7C;
          hidprint(labelchar);
          delay(20);
	  
	  onlykey_eeget_label(ptr, (offset  + 11));
	  ByteToChar(label, labelchar, EElen_label);
	  label[0] = (byte)0x11;
	  label[1] = (byte)0x7C;
          hidprint(labelchar);
          delay(20);
	  
	  onlykey_eeget_label(ptr, (offset  + 12));
	  ByteToChar(label, labelchar, EElen_label);
	  label[0] = (byte)0x12;
	  label[1] = (byte)0x7C;
          hidprint(labelchar);
          delay(20);
	  
      blink(3);
      return;
}



void SETSLOT (byte *buffer)
{
	
      char cmd = buffer[4]; //cmd or continuation
      int slot = buffer[5];
      int value = buffer[6];
      int length;
#ifdef DEBUG
      Serial.print("OKSETSLOT MESSAGE RECEIVED:");
      Serial.println((int)cmd - 0x80, HEX);
      Serial.print("Setting Slot #");
      Serial.println((int)slot, DEC);
      Serial.print("Value #");
      Serial.println((int)value, DEC);
#endif
     for (int z = 0; buffer[z + 7] + buffer[z + 8] + buffer[z + 9] + buffer[z + 10 ] != 0x00; z++) {
        length = z + 1;
#ifdef DEBUG
        Serial.print(buffer[z + 7], HEX);
#endif
       }
#ifdef DEBUG
      Serial.println(); //newline
      Serial.print("Length = ");
      Serial.println(length);
#endif
	if (PDmode) slot = slot + 12;
            switch (value) {
            case 1:
            //Set value in EEPROM
            Serial.println(); //newline
            Serial.print("Writing Label Value to EEPROM...");
            onlykey_eeset_label(buffer + 7, length, slot);
			hidprint("Successfully set Label");
            return;
            //break;
            case 2:
            //Encrypt and Set value in EEPROM
            Serial.println("Writing Username Value to EEPROM...");
            if (!PDmode) {
            #ifdef DEBUG
            Serial.println("Unencrypted");
            for (int z = 0; z < 32; z++) {
      	    Serial.print(buffer[z + 7], HEX);
            }
            Serial.println();
            #endif 
      	    aes_gcm_encrypt((buffer + 7), (uint8_t*)('u'+ID[34]+slot), phash, length);
      	    #ifdef DEBUG
      	    Serial.println("Encrypted");
            for (int z = 0; z < 32; z++) {
      	    Serial.print(buffer[z + 7], HEX);
            }
            Serial.println();
            #endif     
            Serial.println(); //newline
            }
            onlykey_eeset_username(buffer + 7, length, slot);
	    hidprint("Successfully set Username");
            return;
            //break;
            case 3:
            //Set value in EEPROM
            Serial.println(); //newline
            Serial.print("Writing Additional Character1 to EEPROM...");
            onlykey_eeset_addchar1(buffer + 7, slot);
	    hidprint("Successfully set Character1");
            return;
            //break;
            case 4:
            //Set value in EEPROM
            Serial.println(); //newline
            Serial.print("Writing Delay1 to EEPROM...");
            buffer[7] = (buffer[7] -'0');
            onlykey_eeset_delay1(buffer + 7, slot);
	    hidprint("Successfully set Delay1");
            return;
            //break;
            case 5:
            //Encrypt and Set value in EEPROM
            Serial.println("Writing Password to EEPROM...");
            if (!PDmode) {
            #ifdef DEBUG
            Serial.println("Unencrypted");
            for (int z = 0; z < 32; z++) {
      	    Serial.print(buffer[z + 7], HEX);
            }
            Serial.println();
            #endif  
            aes_gcm_encrypt((buffer + 7), (uint8_t*)('p'+ID[34]+slot), phash, length);
      	    #ifdef DEBUG
      	    Serial.println("Encrypted");
            for (int z = 0; z < 32; z++) {
      	    Serial.print(buffer[z + 7], HEX);
            }
            Serial.println();
            #endif 
            Serial.println(); //newline
            }
            onlykey_eeset_password(buffer + 7, length, slot);
	    hidprint("Successfully set Password");
            return;
            //break;
            case 6:
            //Set value in EEPROM
            Serial.println(); //newline
            Serial.print("Writing Additional Character2 to EEPROM...");
            onlykey_eeset_addchar2(buffer + 7, slot);
	    hidprint("Successfully set Character2");
            return;
            //break;
            case 7:
            //Set value in EEPROM
            Serial.println(); //newline
            Serial.print("Writing Delay2 to EEPROM...");
            buffer[7] = (buffer[7] -'0');
            onlykey_eeset_delay2(buffer + 7, slot);
	    hidprint("Successfully set Delay2");
            return;
            //break;
            case 8:
            //Set value in EEPROM
            Serial.println(); //newline
            Serial.print("Writing 2FA Type to EEPROM...");
            onlykey_eeset_2FAtype(buffer + 7, slot);
	    hidprint("Successfully set 2FA Type");
            return;
            //break;
            case 9:
            //Encrypt and Set value in EEPROM
            Serial.println("Writing TOTP Key to EEPROM...");
            if (PDmode) return;
            #ifdef DEBUG
            Serial.println("Unencrypted");
            for (int z = 0; z < 32; z++) {
      	    Serial.print(buffer[z + 7], HEX);
            }
            Serial.println();
            #endif 
            aes_gcm_encrypt((buffer + 7), (uint8_t*)('t'+ID[34]+slot), phash, length);
	    #ifdef DEBUG
	    Serial.println("Encrypted");
            for (int z = 0; z < 32; z++) {
      	    Serial.print(buffer[z + 7], HEX);
            }
            Serial.println();
            #endif    
            onlykey_eeset_totpkey(buffer + 7, length, slot);
	    hidprint("Successfully set TOTP Key");
            return;
            //break;
            case 10:
            if (PDmode) return;
            //Encrypt and Set value in EEPROM
            Serial.println("Writing onlykey AES Key, Priviate ID, and Public ID to EEPROM...");
            #ifdef DEBUG
            Serial.println("Unencrypted");
            for (int z = 0; z < 32; z++) {
      	    Serial.print(buffer[z + 7], HEX);
            }
            Serial.println();
            #endif 
            aes_gcm_encrypt((buffer + 7), (uint8_t*)('y'+ID[34]), phash, length);
      	    #ifdef DEBUG
      	    Serial.println("Encrypted");
            for (int z = 0; z < 32; z++) {
      	    Serial.print(buffer[z + 7], HEX);
            }
            Serial.println();
            #endif 
            onlykey_eeset_aeskey(buffer + 7, EElen_aeskey);
            onlykey_eeset_private((buffer + 7 + EElen_aeskey));
            onlykey_eeset_public((buffer + 7 + EElen_aeskey + EElen_private), EElen_public);
	    hidprint("Successfully set onlykey AES Key, Priviate ID, and Public ID");
            return;
            //break;
            default: 
            return;
          }
      blink(3);
      return;
}

void WIPESLOT (byte *buffer)
{
      char cmd = buffer[4]; //cmd or continuation
      int slot = buffer[5];
      int value = buffer[6];
      int length;
      Serial.print("OKWIPESLOT MESSAGE RECEIVED:");
      Serial.println((int)cmd - 0x80, HEX);
      Serial.print("Wiping Slot #");
      Serial.println((int)slot, DEC);
      Serial.print("Value #");
      Serial.println((int)value, DEC);
      for (int z = 7; z < 64; z++) {
        buffer[z] = 0x00;
        Serial.print(buffer[z], HEX);
        }
     Serial.print("Overwriting slot with 0s");
   
            switch (value) {
            case 1:
            //Set value in EEPROM
            Serial.println(); //newline
            Serial.print("Wiping Label Value...");
            onlykey_eeset_label((buffer + 7), EElen_label, slot);
            hidprint("Successfully wiped Label");
            return;
            break;
            case 2:
            //Set value in EEPROM
            Serial.println(); //newline
            Serial.print("Wiping Username Value...");
            onlykey_eeset_username((buffer + 7), EElen_username, slot);
            hidprint("Successfully wiped Username");
            return;
            break;
            case 3:
            //Set value in EEPROM
            Serial.println(); //newline
            Serial.print("Wiping Additional Character1 Value...");
            onlykey_eeset_addchar1((buffer + 7), slot);
            hidprint("Successfully wiped Additional Character 1");
            return;
            break;
            case 4:
            //Set value in EEPROM
            Serial.println(); //newline
            Serial.print("Writing Delay1 to EEPROM...");
            onlykey_eeset_delay1((buffer + 7), slot);
            hidprint("Successfully wiped Delay 1");
            return;
            break;
            case 5:
            //Set value in EEPROM
            Serial.println(); //newline
            Serial.print("Wiping Password Value...");
            onlykey_eeset_password((buffer + 7), EElen_password, slot);
            hidprint("Successfully wiped Password");
            return;
            break;
            case 6:
            //Set value in EEPROM
            Serial.println(); //newline
            Serial.print("Wiping Additional Character2 Value...");
            onlykey_eeset_addchar2((buffer + 7), slot);
            hidprint("Successfully wiped Additional Character 2");
            return;
            break;
            case 7:
            //Set value in EEPROM
            Serial.println(); //newline
            Serial.print("Wiping Delay2 Value...");
            onlykey_eeset_delay2((buffer + 7), slot);
            hidprint("Successfully wiped Delay 2");
            return;
            break;
            case 8:
            if (PDmode) return;
            //Set value in EEPROM
            Serial.println(); //newline
            Serial.print("Wiping 2FA Type Value...");
            onlykey_eeset_2FAtype((buffer + 7), slot);
            hidprint("Successfully wiped 2FA Type");
            return;
            break;
            case 9:
            if (PDmode) return;
            //Set value in EEPROM
            Serial.println(); //newline
            Serial.print("Writing TOTP Key to EEPROM...");
            onlykey_eeset_totpkey((buffer + 7), EElen_totpkey, slot);
            hidprint("Successfully wiped TOTP Key");
            return;
            break;
            case 10:
            if (PDmode) return;
            //Set value in EEPROM
            Serial.println(); //newline
            Serial.print("Wiping onlykey AES Key, Priviate ID, and Public ID...");
            onlykey_eeset_aeskey((buffer + 7), EElen_aeskey);
            onlykey_eeset_private((buffer + 7 + EElen_aeskey));
            onlykey_eeset_public((buffer + 7 + EElen_aeskey + EElen_private), EElen_public);
            hidprint("Successfully wiped AES Key, Priviate ID, and Public ID");
            return;
            break;
            default: 
            break;
          }
      blink(3);
      return;
}

void SETU2FPRIV (byte *buffer)
{
if (PDmode) return;
      Serial.println("OKSETU2FPRIV MESSAGE RECEIVED");
		
//Set pointer to first empty flash sector
  uintptr_t adr = flashFirstEmptySector();
  unsigned int length = buffer[5];
  Serial.print("Length of U2F private = ");
  Serial.println(length);
 
  onlykey_eeset_U2Fprivlen(buffer + 5); //length is number of bytes
  uint8_t addr[2];
  uint8_t *ptr;
      addr[0] = (int)((adr >> 8) & 0XFF); //convert long to array
      addr[1] = (int)((adr & 0XFF));
  ptr = addr;
  onlykey_eeset_U2Fprivpos(ptr); //Set the starting position for U2F Priv

  for( int z = 6; z <= length && z <= 58; z=z+4, adr=adr+4){
  unsigned long sector = buffer[z] | (buffer[z+1] << 8L) | (buffer[z+2] << 16L) | (buffer[z+3] << 24L);
   //Write long to empty sector 
  Serial.println();
  Serial.printf("Writing to Sector 0x%X, value 0x%X ", adr, sector);
  if ( flashProgramWord((unsigned long*)adr, &sector) ) Serial.printf("NOT ");
  //Serial.printf("successful. Read Value:0x%X\r\n", *((unsigned int*)adr));
  }
  blink(3);
      return;
}

void WIPEU2FPRIV (byte *buffer)
{
if (PDmode) return;
      Serial.println("OKWIPEU2FPRIV MESSAGE RECEIVED");
      uint8_t addr[2];
      uint8_t *ptr;
      ptr = addr;
      onlykey_eeget_U2Fprivlen(ptr); //Get the length for U2F private
      int length = addr[0] | (addr[1] << 8);
      Serial.println(length);
      
      onlykey_eeget_U2Fprivpos(ptr); //Get the starting position for U2F private
      //Set adr to position of U2F private in flash
      unsigned long address = addr[1] | (addr[0] << 8L);
      uintptr_t adr = address;
      unsigned long sector = 0x00;
      Serial.println(address);
      for( int z = 0; z <= length; z=z+4, adr=adr+4){
     //Write long to sector 
        Serial.println();
        Serial.printf("Writing to Sector 0x%X, value 0x%X ", adr, sector);
        if ( flashProgramWord((unsigned long*)adr, &sector) ) Serial.printf("NOT ");
        //Serial.printf("successful. Read Value:0x%X\r\n", *((unsigned int*)adr));
      }
      blink(3);
      return;
}

void SETU2FCERT (byte *buffer)
{
if (PDmode) return;
      Serial.println("OKSETU2FCERT MESSAGE RECEIVED");
  //Set pointer to first empty flash sector
  uintptr_t adr = flashFirstEmptySector();
  int length = buffer[5] | (buffer[6] << 8);
    if(length <= CERTMAXLENGTH){ 
    Serial.print("Length of certificate = ");
    Serial.println(length);
    onlykey_eeset_U2Fcertlen((buffer + 5)); //length is number of bytes
    }
    else {
      return;
    }
  uint8_t addr[2];
  uint8_t *ptr;
      addr[0] = (int)((adr >> 8) & 0XFF); //convert long to array
      addr[1] = (int)((adr & 0XFF));
  ptr = addr;
  onlykey_eeset_U2Fcertpos(ptr); //Set the starting position for U2F Cert


  //Write packets to flash up to reaching length of certificate

  int n;
  int x;
  int written = 0;
  while(written <= length){ 
    n = RawHID.recv(recv_buffer, 0); // 0 timeout = do not wait
    if (n > 0) {  
      Serial.print(F("\n\nReceived packet"));
      for (x=0; x<64; x++) {
          Serial.print(recv_buffer[x], HEX);
      }
  
      for( int z = 0; z <= 64; z=z+4, adr=adr+4, written=written+4){
         unsigned long sector = buffer[z] | (buffer[z+1] << 8L) | (buffer[z+2] << 16L) | (buffer[z+3] << 24L);
     //Write long to empty sector 
        Serial.println();
        Serial.printf("Writing to Sector 0x%X, value 0x%X ", adr, sector);
        if ( flashProgramWord((unsigned long*)adr, &sector) ) Serial.printf("NOT ");
        //Serial.printf("successful. Read Value:0x%X\r\n", *((unsigned int*)adr));
      }
  }
  }
  
      blink(3);
      return;
}

void WIPEU2FCERT (byte *buffer)
{
if (PDmode) return;
      Serial.println("OKWIPEU2FCERT MESSAGE RECEIVED");
      uint8_t addr[2];
      uint8_t *ptr;
      ptr = addr;
      onlykey_eeget_U2Fcertlen(ptr); //Get the length for U2F Cert
      int length = addr[0] | (addr[1] << 8);
      Serial.println(length);
      
      onlykey_eeget_U2Fcertpos(ptr); //Get the starting position for U2F Cert
      //Set adr to position of U2F Cert in flash
      unsigned long address = addr[1] | (addr[0] << 8L);
      uintptr_t adr = address;
      unsigned long sector = 0x00;
      Serial.println(address);
      for( int z = 0; z <= length; z=z+4, adr=adr+4){
     //Write long to sector 
        Serial.println();
        Serial.printf("Writing to Sector 0x%X, value 0x%X ", adr, sector);
        if ( flashProgramWord((unsigned long*)adr, &sector) ) Serial.printf("NOT ");
        //Serial.printf("successful. Read Value:0x%X\r\n", *((unsigned int*)adr));
      }
      blink(3);
      return;
}

void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year()); 
  Serial.println(); 
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void blink(int times){
  
  int i;
  for(i = 0; i < times; i++){
    digitalWrite(ledPin, HIGH);
    delay(100);
    digitalWrite(ledPin, LOW);
    delay(100);
  }
}

int RNG2(uint8_t *dest, unsigned size) {
    getrng(dest, size);
    
    return 1;
  }

void getrng(uint8_t *ptr, unsigned size) {
    // Generate output whenever 32 bytes of entropy have been accumulated.
    // The first time through, we wait for 48 bytes for a full entropy pool.
    size_t length = 48; // First block should wait for the pool to fill up.
    if (RNG.available(length)) {
        RNG.rand(ptr, size);
        length = 32;
    }
}

void printHex(const byte *data, unsigned len)
{
    static char const hexchars[] = "0123456789ABCDEF";
    while (len > 0) {
        int b = *data++;
        Keyboard.print(hexchars[(b >> 4) & 0x0F]);
        Keyboard.print(hexchars[b & 0x0F]);
        --len;
    }
    Serial.println();
}

void ByteToChar(byte* bytes, char* chars, unsigned int count){
    for(unsigned int i = 0; i < count; i++)
    	 chars[i] = (char)bytes[i];
}

void CharToByte(char* chars, byte* bytes, unsigned int count){
    for(unsigned int i = 0; i < count; i++)
    	bytes[i] = (byte)chars[i];
}

void ByteToChar2(byte* bytes, char* chars, unsigned int count, unsigned int index){
    for(unsigned int i = 0; i < count; i++)
    	 chars[i+index] = (char)bytes[i];
}

void CharToByte2(char* chars, byte* bytes, unsigned int count, unsigned int index){
    for(unsigned int i = 0; i < count; i++)
    	bytes[i+index] = (byte)chars[i];
}

void hidprint(char* chars) 
{ 
int i=0;
while(*chars) {
     resp_buffer[i] = (byte)*chars;
     chars++;
	 i++;
  }
  RawHID.send(resp_buffer, 0);
  memset(resp_buffer, 0, sizeof(resp_buffer));
}

void factorydefault() {
  //Todo add function from flashKinetis to wipe secure flash and eeprom values and flashQuickUnlockBits 
        uint8_t value;
        Serial.println("Current EEPROM Values"); //TODO remove debug
        for (int i=0; i<2048; i++) {
        value=EEPROM.read(i);
        Serial.print(i);
  	Serial.print("\t");
  	Serial.print(value, DEC);
  	Serial.println();
        }
        value=0x00;
        for (int i=0; i<2048; i++) {
        EEPROM.write(i, value);
        }
        Serial.println("EEPROM set to 0s");//TODO remove debug
        for (int i=0; i<2048; i++) {
        value=EEPROM.read(i);
        Serial.print(i);
  	Serial.print("\t");
  	Serial.print(value, DEC);
  	Serial.println();
        }
        initialized = false;
        unlocked = true;
        Serial.println("factory reset has been completed");
}



void aes_gcm_encrypt (uint8_t * state, uint8_t * iv1, const uint8_t * key, int len) {
GCM<AES256> gcm; 
uint8_t iv2[12];
uint8_t tag[16];
uint8_t *ptr;
ptr = iv2;
onlykey_flashget_noncehash(ptr, 12);

for(int i =0; i<=12; i++) {
  iv2[i]=iv2[i]^*iv1;
}


gcm.clear ();
gcm.setKey(key, sizeof(key));
gcm.setIV(iv2, 12);
gcm.encrypt(state, state, len);

gcm.computeTag(tag, sizeof(tag)); 
}

int aes_gcm_decrypt (uint8_t * state, uint8_t * iv1, const uint8_t * key, int len) {
GCM<AES256> gcm; 
uint8_t iv2[12];
uint8_t tag[16];
uint8_t *ptr;
ptr = iv2;
onlykey_flashget_noncehash(ptr, 12);

for(int i =0; i<=12; i++) {
  iv2[i]=iv2[i]^*iv1;
}

gcm.clear ();
gcm.setKey(key, sizeof(key));
gcm.setIV(iv2, 12);
gcm.decrypt(state, state, len);

if (!gcm.checkTag(tag, sizeof(tag))) {
    return 1;
}
}

/*************************************/
void onlykey_flashget_common (uint8_t *ptr, unsigned long *adr, int len) {
    for( int z = 0; z <= len-4; z=z+4){
        Serial.printf(" 0x%X", adr);
        Serial.println();
        *ptr = (uint8_t)((*(adr+z) >> 24) & 0xFF);
        Serial.printf(" 0x%X", *ptr);
        ptr++;
 	*ptr = (uint8_t)((*(adr+z) >> 16) & 0xFF);
 	Serial.printf(" 0x%X", *ptr);
 	ptr++;
 	*ptr = (uint8_t)((*(adr+z) >> 8) & 0xFF);
 	Serial.printf(" 0x%X", *ptr);
 	ptr++;
 	*ptr = (uint8_t)((*(adr+z) & 0xFF));
 	Serial.printf(" 0x%X", *ptr);
 	ptr++;
	}
	Serial.printf(" 0x%X", *adr);
	return;
}

void onlykey_flashset_common (uint8_t *ptr, unsigned long *adr, int len) {
        for( int z = 0; z <= len-4; z=z+4){
        unsigned long data = (uint8_t)*(ptr+z+3) | ((uint8_t)*(ptr+z+2) << 8) | ((uint8_t)*(ptr+z+1) << 16) | ((uint8_t)*(ptr+z) << 24);
        Serial.print("Data to write = ");
        Serial.println(data, HEX);

        //Write long to sector 
        Serial.println();
        Serial.printf("Writing to Sector 0x%X, value 0x%X ", adr, data);
        if ( flashProgramWord((unsigned long*)(adr+z), &data) ) Serial.printf("NOT ");
	}
	return;
}
/*********************************/

int onlykey_flashget_noncehash (uint8_t *ptr, int size) {
    uint8_t addr[2];
    onlykey_eeget_hashpos(addr);
    if (addr[0]+addr[1] == 0) { //pinhash not set
    	return 0;
    }
    else {
    unsigned long address = (0x01 << 16L) | (addr[0] << 8L) | addr[1];
    
    onlykey_flashget_common(ptr, (unsigned long*)address, size);
    return 1;
    }
}
void onlykey_flashset_noncehash (uint8_t *ptr) {
    uint8_t addr[2];
    uintptr_t adr;
    onlykey_eeget_hashpos(addr);
    if (addr[0]+addr[1] == 0) { //First time setting pinhash
    	adr = flashFirstEmptySector();
    	Serial.printf("First empty Sector is 0x%X\r\n", flashFirstEmptySector());
    	addr[0] = (uint8_t)((adr >> 8) & 0XFF); //convert long to array
    	Serial.print("addr 1 = "); //TODO debug remove
    	Serial.println(addr[0]); //TODO debug remove
        addr[1] = (uint8_t)((adr & 0XFF));
        Serial.print("addr 2 = "); //TODO debug remove
    	Serial.println(addr[1]); //TODO debug remove
        onlykey_eeset_hashpos(addr); //Set the starting position for hash
        onlykey_flashset_common(ptr, (unsigned long*)adr, EElen_noncehash);
        Serial.print("Nonce hash address =");
    Serial.println(adr, HEX);
    onlykey_flashget_common(ptr, (unsigned long*)adr, EElen_noncehash);
    }
    else {
      uintptr_t adr = (0x01 << 16L) | (addr[0] << 8L) | addr[1];
      onlykey_flashset_common(ptr, (unsigned long*)adr, EElen_noncehash);
      Serial.print("Nonce hash address =");
    Serial.println(adr, HEX);
    Serial.print("Nonce hash value =");
    Serial.printf(" 0x%X", (unsigned long*)adr);
      onlykey_flashget_common(ptr, (unsigned long*)adr, EElen_noncehash);
      }    
    
}


/*********************************/

/*********************************/

int onlykey_flashget_pinhash (uint8_t *ptr, int size) {
    uint8_t addr[2];
    onlykey_eeget_hashpos(addr);
    if (addr[0]+addr[1] == 0) { //pinhash not set
    	return 0;
    }
    else {
      uintptr_t adr =  (0x01 << 16L) | (addr[0] << 8L) | addr[1];
      onlykey_flashget_common(ptr, (unsigned long*)(adr+100), EElen_pinhash);
      }
    return 1;
}
void onlykey_flashset_pinhash (uint8_t *ptr) {
    uint8_t addr[2];
    onlykey_eeget_hashpos(addr);
    if (addr[0]+addr[1] == 0) { //pinhash not set
    	return;
    }
    else {
    uintptr_t adr = (0x01 << 16L) | (addr[0] << 8L) | addr[1];
    onlykey_flashset_common(ptr, (unsigned long*)(adr+100), EElen_pinhash);
    Serial.print("PIN hash address =");
    Serial.println(adr+EElen_pinhash, HEX);
    Serial.print("PIN hash value =");
    Serial.printf(" 0x%X", (unsigned long*)(adr+EElen_pinhash));
    onlykey_flashget_common(ptr, (unsigned long*)(adr+100), EElen_pinhash);
    }
}

/*********************************/
/*********************************/

int onlykey_flashget_selfdestructhash (uint8_t *ptr) {
    uint8_t addr[2];
    onlykey_eeget_hashpos(addr);
    if (addr[0]+addr[1] == 0) { //pinhash not set
    	return 0;
    }
    else {
    uintptr_t adr =  (0x01 << 16L) | (addr[0] << 8L) | addr[1];
    onlykey_flashget_common(ptr, (unsigned long*)(adr+200), EElen_selfdestructhash);
    }
}
void onlykey_flashset_selfdestructhash (uint8_t *ptr) {
    uint8_t addr[2];
    onlykey_eeget_hashpos(addr);
    if (addr[0]+addr[1] == 0) { //pinhash not set
    	return;
    }
    else {
    uintptr_t adr =  (0x01 << 16L) | (addr[0] << 8L) | addr[1];
    onlykey_flashset_common(ptr, (unsigned long*)(adr+200), EElen_selfdestructhash);
    }
}

/*********************************/
/*********************************/

int onlykey_flashget_plausdenyhash (uint8_t *ptr) {
    uint8_t addr[2];
    onlykey_eeget_hashpos(addr);
    if (addr[0]+addr[1] == 0) { //pinhash not set
    	return 0;
    }
    else {
    uintptr_t adr =  (0x01 << 16L) | (addr[0] << 8L) | addr[1];
    onlykey_flashget_common(ptr, (unsigned long*)(adr+300), EElen_plausdenyhash);
    }
}
void onlykey_flashset_plausdenyhash (uint8_t *ptr) {
    uint8_t addr[2];
    onlykey_eeget_hashpos(addr);
    if (addr[0]+addr[1] == 0) { //pinhash not set
    	return;
    }
    else {
    uintptr_t adr =  (0x01 << 16L) | (addr[0] << 8L) | addr[1];
    onlykey_flashset_common(ptr, (unsigned long*)(adr+300), EElen_plausdenyhash);
    }
}

/*********************************/

