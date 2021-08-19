#include <Arduino.h>
#include "RF24.h"
#include "mpr121.h"
#include "i2c.h"
#include <avr/sleep.h>
#include <avr/power.h>

RF24 radio(9, 10); // CE, CSN

volatile int wakeUpFromInterrupt = 1;
int touch_shield_value = 99;

static int read_touch_shield() {
  int touchstatus = (mpr121Read(0x01) << 8) | mpr121Read(0x00);
  int touchNumber = 0;
  for (int j=0; j<12; j++) {
    if (touchstatus & (1<<j)) {
      touchNumber++;
    }
  }
  if (touchNumber != 1) {
    return 99;
  }

  int value = 99;
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
  return value;
}
static void touch_shield_it() {
  if(wakeUpFromInterrupt == 0){
    Serial.println("wake up from shield");
    detachInterrupt(digitalPinToInterrupt(2));
    detachInterrupt(digitalPinToInterrupt(3));
    wakeUpFromInterrupt = 1;
  }
}
static void radio_it() {
  if(wakeUpFromInterrupt == 0){
    Serial.println("wake up from radio");
    detachInterrupt(digitalPinToInterrupt(2));
    detachInterrupt(digitalPinToInterrupt(3));
    wakeUpFromInterrupt = 1;
  }
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
  radio.setRetries(5, 5);  // max 15, 15
  radio.setAddressWidth(3);
  radio.openWritingPipe(0x000001);
  radio.openReadingPipe(1, 0x000002);
  radio.startListening();
  pinMode(3, INPUT);
  digitalWrite(3, HIGH);

  // Touch shield
  pinMode(2, INPUT);
  digitalWrite(2, HIGH);
  DDRC |= 0b00010011;
  PORTC = 0b00110000;
  i2cInit();
  delay(100);  // needed for radio too
  mpr121QuickConfig();

  // LED Ack
  pinMode(7, OUTPUT);

  Serial.begin(57600);

  digitalWrite(7, HIGH);
  delay(200);
  digitalWrite(7, LOW);
}
void loop() {
  // sleep
  if (digitalRead(2) == LOW || digitalRead(3) == LOW) {
    // Serial.println("can't sleep");
  } else {
    // Serial.println("sleeping");
    Serial.flush();
    set_sleep_mode(SLEEP_MODE_STANDBY);
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    attachInterrupt(digitalPinToInterrupt(2), touch_shield_it, FALLING);
    attachInterrupt(digitalPinToInterrupt(3), radio_it, FALLING);
    wakeUpFromInterrupt = 0;
    sleep_mode();
    sleep_disable();
    while(!wakeUpFromInterrupt); // wait IT catch before start run
  }
  while (digitalRead(2) == LOW) {
    touch_shield_value = read_touch_shield();
    if (touch_shield_value != 99) {
      char text[8];
      sprintf(text, "%d", touch_shield_value);
      Serial.println(text);
      radio.stopListening();
      radio.write(&text, sizeof(text));
      radio.startListening();
      digitalWrite(7, HIGH);
      delay(2);
      digitalWrite(7, LOW);
      delay(300);
    }
  }
  while (digitalRead(3) == LOW) {
    char radio_input[8] = "";
    radio.read(&radio_input, sizeof(radio_input));
    if (radio_input[0] == *"A") {
      digitalWrite(7, HIGH);
      delay(2);
      digitalWrite(7, LOW);
      delay(100);
      digitalWrite(7, HIGH);
      delay(2);
      digitalWrite(7, LOW);
    } else if (radio_input[0] == *"S") {
      digitalWrite(7, HIGH);
      delay(2);
      digitalWrite(7, LOW);
      delay(100);
      digitalWrite(7, HIGH);
      delay(2);
      digitalWrite(7, LOW);
      radio.stopListening();
      radio.write(&"S", sizeof("S"));
      radio.startListening();
    }
  }
}
