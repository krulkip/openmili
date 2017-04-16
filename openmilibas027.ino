#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h> //http://tmrh20.github.io/RF24/
#include "PL1167_nRF24.h"
#include "MiLightRadio.h"

// define connection pins for nRF24L01 shield on www.arduino-projects4u.com
#define CE_PIN 9   //ESP8266 2
#define CSN_PIN 10 //ESP8266 15

#define V2_OFFSET(byte, key, jumpStart) ( \
  V2_OFFSETS[byte-1][key%4] \
    + \
  ((jumpStart > 0 && key >= jumpStart && key <= jumpStart+0x80) ? 0x80 : 0) \
) 
int mode=1; // Transmitting mode 1 = RGBW   2 = CCT   3 = RGB + CCT   4 = RGB
#define V2_OFFSET_JUMP_START 0x54

uint8_t const V2_OFFSETS[][4] = {
  { 0x45, 0x1F, 0x14, 0x5C },
  { 0x2B, 0xC9, 0xE3, 0x11 },
  { 0xEE, 0xDE, 0x0B, 0xAA },
  { 0xAF, 0x03, 0x1D, 0xF3 },
  { 0x1A, 0xE2, 0xF0, 0xD1 },
  { 0x04, 0xD8, 0x71, 0x42 },
  { 0xAF, 0x04, 0xDD, 0x07 },
  { 0xE1, 0x93, 0xB8, 0xE4 }
};
uint8_t xorKey(uint8_t key) {
  // Generate most significant nibble
  const uint8_t shift = (key & 0x0F) < 0x04 ? 0 : 1;
  const uint8_t x = (((key & 0xF0) >> 4) + shift + 6) % 8;
  const uint8_t msn = (((4 + x) ^ 1) & 0x0F) << 4;
  // Generate least significant nibble
  const uint8_t lsn = ((((key & 0x0F) + 4)^2) & 0x0F);
  return ( msn | lsn );
}
uint8_t decodeByte(uint8_t byte, uint8_t s1, uint8_t xorKey, uint8_t s2) {
  uint8_t value = byte - s2;
  value = value ^ xorKey;
  value = value - s1;
  return value;
}
uint8_t encodeByte(uint8_t byte, uint8_t s1, uint8_t xorKey, uint8_t s2) {
  uint8_t value = byte + s1;
  value = value ^ xorKey;
  value = value + s2;
  return value;
}
void decodeV2Packet(uint8_t *packet) {
  uint8_t key = xorKey(packet[0]);
  for (size_t i = 1; i <= 8; i++) {
    packet[i] = decodeByte(packet[i], 0, key, V2_OFFSET(i, packet[0], V2_OFFSET_JUMP_START));
  }
}
RF24 radio(CE_PIN, CSN_PIN);
PL1167_nRF24 prf(radio);
MiLightRadio mlr(prf);

void setup()
{
  Serial.begin(115200);
  Serial.println();
  delay(1000);
  Serial.println("# OpenMiLight Receiver/Transmitter starting");
  mlr.begin();
}

static int dupesPrinted = 0;
static enum {
  IDLE,
  HAVE_NIBBLE,
  COMPLETE,
} state;

void loop()
{
    if (mlr.available()) {
      uint8_t packet[9];
      size_t packet_length = sizeof(packet);
      Serial.println();
      Serial.print("<-- ");
      mlr.read(packet, packet_length);
      if (packet_length<0x10) Serial.print("0");
      Serial.print(packet_length,HEX);
      Serial.print(" ");     
      for (int i = 0; i < packet_length; i++) {
        if (packet[i]<0x10) Serial.print("0");
        Serial.print(packet[i],HEX);
        Serial.print(" ");
      }
      if (packet_length!=7){
      Serial.print("Decoded package = ");
      uint8_t key = xorKey(packet[0]);
      uint8_t sum = key;
      Serial.print(key,HEX);Serial.print(" ");
      for (size_t i = 1; i <= 7; i++) {
        packet[i] = decodeByte(packet[i], 0, key, V2_OFFSET(i, packet[0], V2_OFFSET_JUMP_START));
        sum += packet[i];
        if (packet[i]<0x10) {Serial.print("0");}
        Serial.print(packet[i],HEX);Serial.print(" ");
      }
    }
    }
    int dupesReceived = mlr.dupesReceived();
    for (; dupesPrinted < dupesReceived; dupesPrinted++) {
    Serial.print(".");
    } 
}



