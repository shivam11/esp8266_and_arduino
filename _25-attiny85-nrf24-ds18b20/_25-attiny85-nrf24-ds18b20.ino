// pins
/* 24l01    85
   1  gnd   4
   2  vcc   8
   3  ce    1
   4  csn   3
   5  sck   7
   6  mosi  6
   7  miso  5
*/

#include <LowPower.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <nRF24L01.h>
#include <RF24.h>

//Disabling ADC saves ~230uAF. Needs to be re-enable for the internal voltage check
#define adc_disable() (ADCSRA &= ~(1<<ADEN)) // disable ADC
#define adc_enable()  (ADCSRA |=  (1<<ADEN)) // re-enable ADC

#define CE_PIN 5
#define CSN_PIN 4

#define DEVICE_ID 2
#define CHANNEL 100

const uint64_t pipes[2] = { 0xFFFFFFFFFFLL, 0xCCCCCCCCCCLL };

typedef struct {
  uint32_t _salt;
  uint16_t volt;
  int16_t data1;
  int16_t data2;
  uint8_t devid;
} data;

data payload;

RF24 radio(CE_PIN, CSN_PIN);

// DS18B20
#define ONE_WIRE_BUS 3
#define TEMPERATURE_PRECISION 9

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress Thermometer01, Thermometer02;

float tempC01;
float tempC02;

void setup() {
  delay(20);
  unsigned long startmilis = millis();
  adc_disable();
  sensors.begin();

  if ( (!sensors.getAddress(Thermometer01, 0)) || (!sensors.getAddress(Thermometer02, 1)) ){
    sleep();
    //return;
  }
  // ds18b20 getTem 766ms
  // 12bit - 750ms, 11bit - 375ms, 10bit - 187ms, 9bit - 93.75ms
  sensors.setResolution(Thermometer01, TEMPERATURE_PRECISION);
  sensors.setResolution(Thermometer02, TEMPERATURE_PRECISION);

  // radio begin to power down : 80 ms
  radio.begin();
  radio.setChannel(CHANNEL);
  //radio.setPALevel(RF24_PA_LOW);
  radio.setPALevel(RF24_PA_MIN);
  //radio.setDataRate(RF24_250KBPS);
  radio.setDataRate(RF24_1MBPS);
  //radio.setAutoAck(1); 
  radio.setRetries(15, 15);
  radio.enableDynamicPayloads();
  //radio.setCRCLength(RF24_CRC_8);
  //radio.setPayloadSize(11);
  //
  radio.openWritingPipe(pipes[0]);
  radio.stopListening();
  
  unsigned long stopmilis = millis();
  //payload.data2 = ( stopmilis - startmilis ) * 10 ;

  payload._salt = 0;
  payload.devid = DEVICE_ID;
}

void loop() {
  payload._salt++;
  unsigned long startmilis = millis();

  sensors.requestTemperatures();
  tempC01 = sensors.getTempC(Thermometer01);
  tempC02 = sensors.getTempC(Thermometer02);

  if (isnan(tempC01) || tempC01 < -50 || tempC01 > 60 || isnan(tempC02) || tempC02 < -50 || tempC02 > 60 ) {
    sleep();
    return;
  }

  payload.data1 = (tempC01 * 10);
  payload.data2 = (tempC02 * 10);

  payload.volt = readVcc();

  if ( payload.volt > 5000 || payload.volt <= 0 ) {
    sleep();
    return;
  }

/*
  if ( payload.data2 < 0 ) {
    sleep();
    return;
  }
*/
  
  radio.powerUp();
  radio.write(&payload , sizeof(payload));
  radio.powerDown();
  unsigned long stopmilis = millis();

  //payload.data2 = ( stopmilis - startmilis ) * 10  ;
  sleep();
}

void sleep() {
  /* 60 seconds */
  for (int i = 0; i < 7; i++) {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  }
  LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);
  
  /* 1 sec
  LowPower.powerDown(SLEEP_500MS, ADC_OFF, BOD_OFF);
  LowPower.powerDown(SLEEP_250MS, ADC_OFF, BOD_OFF);
  LowPower.powerDown(SLEEP_120MS, ADC_OFF, BOD_OFF);
  */
  /* 10 sec */
  /*
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
  LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
  */
}

int readVcc() {
  adc_enable();
  ADMUX = _BV(MUX3) | _BV(MUX2);

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both

  long result = (high << 8) | low;

  //result = 1126400L / result; // Calculate Vcc (in mV);
  result = 1074835L / result;

  //Disable ADC
  adc_disable();

  return (int)result; // Vcc in millivolts
}
