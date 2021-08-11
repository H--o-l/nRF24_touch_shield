#include <Arduino.h>
#include "RF24.h"
#include "mpr121.h"
#include "i2c.h"

RF24 radio(9, 10); // CE, CSN

void radio_send(uint8_t value) {
  char text[8];
  sprintf(text, "%d", value);
  // Serial.println(text);
  radio.write(&text, sizeof(text));
  // radio.flush_tx();
}

void new_touch() {
  int touchstatus = (mpr121Read(0x01) << 8) | mpr121Read(0x00);
  int touchNumber = 0;
  for (int j=0; j<12; j++) {
    if (touchstatus & (1<<j)) {
      touchNumber++;
    }
  }
  if (touchNumber != 1) {
    return;
  }
  int value = 0;
  if (touchstatus & (1<<8)) { value = 1; }
  else if (touchstatus & (1<<5)) { value = 2; }
  else if (touchstatus & (1<<2)) { value = 3; }
  else if (touchstatus & (1<<7)) { value = 4; }
  else if (touchstatus & (1<<4)) { value = 5; }
  else if (touchstatus & (1<<1)) { value = 6; }
  else if (touchstatus & (1<<6)) { value = 7; }
  else if (touchstatus & (1<<3)) { value = 8; }
  else if (touchstatus & (1<<0)) { value = 9; }
  // extras (not used), bit offset
  // ELE9 9
  // ELE10 10
  // ELE11 11
  radio_send(value);
}

void setup() {
  // radio
  radio.begin();
  radio.setPALevel(RF24_PA_HIGH);
  radio.setDataRate(RF24_250KBPS);
  radio.setCRCLength(RF24_CRC_8);
  radio.setChannel(125);
  radio.setAutoAck(1);
  radio.setPayloadSize(8);
  // radio.disableAckPayload();
  radio.setRetries(5, 5);  // max 15, 15
  radio.setAddressWidth(3);
  radio.openWritingPipe(0x000001);
  radio.stopListening();

  // Touch shield
  pinMode(2, INPUT);
  digitalWrite(2, HIGH);
  DDRC |= 0b00010011;
  PORTC = 0b00110000;
  i2cInit();
  delay(100);
  mpr121QuickConfig();
  attachInterrupt(0, new_touch, LOW);

  Serial.begin(57600);
  radio_send(0);
}

void loop() {}
