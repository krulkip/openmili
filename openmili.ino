#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h> //http://tmrh20.github.io/RF24/
#include "PL1167_nRF24.h"
#include "MiLightRadio.h"

// define connection pins for nRF24L01 shield on www.arduino-projects4u.com
#define CE_PIN 9
#define CSN_PIN 10


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
static bool receiving = true; //set receiving true or false
static bool escaped = false;
static uint8_t outgoingPacket[9];
static uint8_t outgoingPacketPos = 0;
static uint8_t nibble;
uint8_t crc;
static enum {
  IDLE,
  HAVE_NIBBLE,
  COMPLETE,
} state;

void loop()
{
  if (receiving) {
    if (mlr.available()) {
      uint8_t packet[9];
      size_t packet_length = sizeof(packet);
      Serial.println();//     printf("\n");
      Serial.print("<-- ");
      if (packet_length<0x10) Serial.print("0");
      Serial.print(packet_length,HEX);// printf("%02X ", packet_length);
      Serial.print(" ");     
      mlr.read(packet, packet_length);

      for (int i = 0; i < packet_length; i++) {
        if (packet[i]<0x10) Serial.print("0");
        Serial.print(packet[i],HEX);// printf("%02X ", packet[i]);
        Serial.print(" ");
      }
      if ((prf.crcprint>>8)<0x10) Serial.print("0");    
      Serial.print(prf.crcprint>>8,HEX);
      if ((prf.crcprint&0xFF)<0x10) Serial.print("0");
      Serial.print(prf.crcprint&0xFF,HEX);
      Serial.print(" ");    
    }
    int dupesReceived = mlr.dupesReceived();
    for (; dupesPrinted < dupesReceived; dupesPrinted++) {
    Serial.print(".");//      printf(".");
    }
  }

  while (Serial.available()) {
    char inChar = (char)Serial.read();
    uint8_t val = 0;
    bool have_val = true;

    if (inChar >= '0' && inChar <= '9') {
      val = inChar - '0';
    } else if (inChar >= 'a' && inChar <= 'f') {
      val = inChar - 'a' + 10;
    } else if (inChar >= 'A' && inChar <= 'F') {
      val = inChar - 'A' + 10;
    } else {
      have_val = false;
    }

    if (!escaped) {
      if (have_val) {
        switch (state) {
          case IDLE:
            nibble = val;
            state = HAVE_NIBBLE;
            break;
          case HAVE_NIBBLE:
            if (outgoingPacketPos < sizeof(outgoingPacket)) {
              outgoingPacket[outgoingPacketPos++] = (nibble << 4) | (val);
            } else {
              Serial.println("# Error: outgoing packet buffer full/packet too long");
            }
            if (outgoingPacketPos >= sizeof(outgoingPacket)) {
              state = COMPLETE;
              //Serial.println("\nCOMPLETE");
            } else {
              state = IDLE;
            }
            break;
          case COMPLETE:
            Serial.println("# Error: outgoing packet complete. Press enter to send.");
            break;
        }
      } else {
        switch (inChar) {
          case ' ':
          case '\n':
          case '\r':
          case '.':
            if (state == COMPLETE) {
              mlr.write(outgoingPacket, sizeof(outgoingPacket));
              Serial.print("\n--> ");
              if (sizeof(outgoingPacket)<0x10) Serial.print("0");              
              Serial.print(sizeof(outgoingPacket),HEX);
              Serial.print(" ");
              for (int i=0; i<sizeof(outgoingPacket);i++){
              if (outgoingPacket[i]<0x10) Serial.print("0");
              Serial.print(outgoingPacket[i],HEX);
              Serial.print(" ");
              }
              if ((prf.crcprint>>8)<0x10) Serial.print("0");    
              Serial.print(prf.crcprint>>8,HEX);
              if ((prf.crcprint&0xFF)<0x10) Serial.print("0");
              Serial.print(prf.crcprint&0xFF,HEX);
              Serial.print(" ");
            }
            if(inChar != ' ') {
              outgoingPacketPos = 0;

              state = IDLE;
            }
            if (inChar == '.') {
              mlr.resend();
              Serial.print(".");
              delay(1);
            }
            break;
          case 'x':
            Serial.println("# Escaped to extended commands: r - Toggle receiver; Press enter to return to normal mode.");
            escaped = true;
            break;
        }
      }
    } else {
      switch (inChar) {
        case '\n':
        case '\r':
          outgoingPacketPos = 0;
          state = IDLE;
          escaped = false;
          break;
        case 'r':
          receiving = !receiving;
          if (receiving) {
            Serial.println("# Now receiving");
          } else {
            Serial.println("# Now not receiving");
          }
          break;
      }
    }
  }
}

