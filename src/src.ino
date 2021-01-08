/*********
 * Name: Costandino Hiripis, Isaac Semackor and Christof Du Toit
 * Date: December 2, 2020 18:21PM
*********/

#include <OneWire.h>
#include <DallasTemperature.h>
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <ArduinoUniqueID.h>

// All DS18B20 Sensors are connected to pin 1 on the LoRa32u4II board
#define ONE_WIRE_BUS 2

//Defining the value of R1 in the voltage divider circuit
#define R1 33

//Defining the value of R2 in the voltage divider circuit
#define R2 300

//Defining the pin that is used to read the battery voltage
#define voltageDivider A0

//Defining the battery voltage of the truck
#define batteryVoltage 5.0

//Defining the pin that is used to switch ON/OFF the navigation system
#define navigationSystem 3

//Defining the pin that is used to switch ON/OFF the mobile terminal
#define mobileTerminal 5

//Defining the interval for the reset of the navigation system and mobile terminal
#define resetInterval 1000

// Creating a oneWire instance(object)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire object reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

//To store number of sensor connected
int numberOfDevices;

//To store data from the voltage divider circuit
int sensorValue;

//Setting the devices to be zero by default
int deviceCount = 0;

// Variable to store a single sensor address
DeviceAddress tempDeviceAddress; 

// Addresses of the DS18B20 sensors
// replace FILLMEIN according to the documentation
uint8_t interior[8] = { 0x28, 0xFF, 0x7E, 0xC7, 0xC0, 0x17, 0x05, 0xD6 };
uint8_t battery[8] = { 0x28, 0xFF, 0xCD, 0xC2, 0xC0, 0x17, 0x05, 0x56 };

static const u1_t PROGMEM APPEUI[8]={ 0x94, 0x91, 0x03, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 };
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

// This should also be in little endian format, see above.
static const u1_t PROGMEM DEVEUI[8]={ 0x5B, 0x0F, 0xF3, 0xDB, 0x47, 0x4E, 0x1F, 0x00 };
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

// This key should be in big endian format (or, since it is not really a
// number but a block of memory, endianness does not really apply). In
// practice, a key taken from ttnctl can be copied as-is.
static const u1_t PROGMEM APPKEY[16] = { 0x9A, 0xC4, 0x20, 0xA4, 0x9A, 0x40, 0xCA, 0x84, 0x87, 0x52, 0xAE, 0xA8, 0xB2, 0xA0, 0x0B, 0x87 };
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

uint8_t payload[16];

static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 30;

// Pin mapping
const lmic_pinmap lmic_pins = {
    .nss = 8,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 4,
    .dio = {7, 6, LMIC_UNUSED_PIN},
};

void setup() {
  Serial.begin(9600);
  sensors.begin();

  pinMode(navigationSystem, OUTPUT);
  pinMode(mobileTerminal, OUTPUT);
  delay(1000);

  // LMIC init
  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();

  // ### Relax LMIC timing ###
  // Required for ATmega328/ATmega32U4 (8MHz) otherwise OTAA joins on SF7 and SF8 will likely fail.
  #define LMIC_CLOCK_ERROR_PERCENTAGE 3
  LMIC_setClockError(LMIC_CLOCK_ERROR_PERCENTAGE * (MAX_CLOCK_ERROR / 100.0));

  // Start job (sending automatically starts OTAA too)
  do_send(&sendjob);
}

void loop() { 
  
  os_runloop_once();
}

// function to create a payload to send to the things network
uint8_t getPayload() {
  int id0 = UniqueID[0];
  int id1 = UniqueID[1];
  int id2 = UniqueID[2];
  int id3 = UniqueID[3];
  int id4 = UniqueID[4];
  int id5 = UniqueID[5];
  int id6 = UniqueID[6];
  int id7 = UniqueID[7];
  int id8 = UniqueID[8];
  
  payload[0] = id0;
  payload[1] = id1;
  payload[2] = id2;
  payload[3] = id3;
  payload[4] = id4;
  payload[5] = id5;
  payload[6] = id6;
  payload[7] = id7;
  payload[8] = id8;
  
  float interiorTemp = getTemperature(interior);
  interiorTemp = interiorTemp / 100;
  float batteryTemp = getTemperature(battery);
  batteryTemp = batteryTemp / 100;
  float voltage = getBatteryVoltage();
  voltage = voltage / 100;

  uint16_t payloadInteriorTemp = LMIC_f2sflt16(interiorTemp);
  // int -> bytes
  byte interiorTempLow = lowByte(payloadInteriorTemp);
  byte interiorTempHigh = highByte(payloadInteriorTemp);
  // place the bytes into the payload
  payload[9] = interiorTempLow;
  payload[10] = interiorTempHigh;

  uint16_t payloadBatteryTemp = LMIC_f2sflt16(batteryTemp);
  // int -> bytes
  byte batteryTempLow = lowByte(payloadBatteryTemp);
  byte batteryTempHigh = highByte(payloadBatteryTemp);
  payload[11] = batteryTempLow;
  payload[12] = batteryTempHigh;

  uint16_t payloadVoltage = LMIC_f2sflt16(voltage);
  // int -> bytes
  byte voltageLow = lowByte(payloadVoltage);
  byte voltageHigh = highByte(payloadVoltage);
  payload[13] = voltageLow;
  payload[14] = voltageHigh;

  return payload;
}

// function to get data from the temperature sensors 
double getTemperature(DeviceAddress deviceAddress) {
  
  float tempC = sensors.getTempC(deviceAddress);
  return tempC;
}

// function to read the battery voltage
double getBatteryVoltage() {
 
  sensorValue = analogRead(voltageDivider);
  float voltage = (sensorValue * (batteryVoltage / 1024.0)) * ((R1 + R2)/ R2);
  // return the value
  return voltage;
  
}

// function to restart the navigation system
void restartNavigationSystem() {
  
  digitalWrite(navigationSystem, LOW);
  delay(resetInterval);    
  digitalWrite(navigationSystem, HIGH);
  delay(resetInterval);
}

// function to restart the mobile terminal
void restartMobileTerminal() {
  
  digitalWrite(mobileTerminal, LOW);
  delay(resetInterval);
  digitalWrite(mobileTerminal, HIGH);
  delay(resetInterval);
}

void onEvent (ev_t ev) {

    switch(ev) {
        case EV_JOINED:
            // Disable link check validation (automatically enabled
            // during join, but because slow data rates change max TX
            // size, we don't use it in this example.
            LMIC_setLinkCheckMode(0);
            break;
        case EV_TXCOMPLETE:
            if (LMIC.txrxFlags & TXRX_ACK)
            if (LMIC.dataLen) {
              if (LMIC.dataLen == 1) {
                uint8_t result = LMIC.frame[LMIC.dataBeg + 0];
                if (result == 0)  {
                  restartNavigationSystem();
                  restartMobileTerminal();
                }              
                if (result == 1)  {
                  restartNavigationSystem();
                } 
                if (result == 2)  {
                  restartMobileTerminal();
                }                                              
              }
            }
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
    }
}

void do_send(osjob_t* j){
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
    } else {
        getPayload();

        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(1, payload, sizeof(payload)-1, 0);
    }
    // Next TX is scheduled after TX_COMPLETE event.
}
