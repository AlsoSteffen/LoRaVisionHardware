/*********
   Name: Costandino Hiripis, Isaac Semackor and Christof Du Toit
   Date: December 2, 2020 18:21PM
*********/

#include <OneWire.h>
#include <DallasTemperature.h>
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <U8x8lib.h>

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
#define navigationSystem 17

//Defining the pin that is used to switch ON/OFF the mobile terminal
#define mobileTerminal 23

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

//Creating the display
U8X8_SSD1306_128X64_NONAME_HW_I2C display(/*rst*/ 16, /*scl*/ 15, /*sda*/ 4);

// Addresses of the DS18B20 sensors
// replace FILLMEIN according to the documentation
String interior[8];
String battery[8];

// This key should be in little endian format(lsb)
static const u1_t PROGMEM APPEUI[8] = { 0x94, 0x91, 0x03, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 };
void os_getArtEui (u1_t* buf) 
{
  memcpy_P(buf, APPEUI, 8);
}

// This key should be in little endian format(lsb)
static const u1_t PROGMEM DEVEUI[8] = { 0x13, 0xBB, 0x9B, 0xA6, 0x39, 0xD5, 0xE4, 0x00 };
void os_getDevEui (u1_t* buf) 
{
  memcpy_P(buf, DEVEUI, 8);
}

// This key should be in big endian format(msb)
static const u1_t PROGMEM APPKEY[16] = { 0xBC, 0x3C, 0xDB, 0x8B, 0x81, 0xE3, 0xF0, 0x60, 0xE7, 0xCE, 0x19, 0x96, 0x6E, 0xDD, 0xB7, 0xF3 };
void os_getDevKey (u1_t* buf) 
{
  memcpy_P(buf, APPKEY, 16);
}

byte payload[6];

static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty cycle limitations).
const unsigned TX_INTERVAL = 30;

// Pin mapping
const lmic_pinmap lmic_pins = { 
  .nss = 18, 
  .rxtx = LMIC_UNUSED_PIN, 
  .rst = 14, 
  .dio = {/*dio0*/ 26, /*dio1*/ 33, /*dio2*/ 32} 
  };

void setup() 
{
  sensors.begin();
  display.begin();
  display.setFont(u8x8_font_victoriamedium8_r);
  
  pinMode(navigationSystem, OUTPUT);
  pinMode(mobileTerminal, OUTPUT);
  digitalWrite(navigationSystem, HIGH);
  digitalWrite(mobileTerminal, HIGH);

  // LMIC init
  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();
  // Start job (sending automatically starts OTAA too)
  do_send(&sendjob);
}

void loop() 
{
  os_runloop_once();
}

// function to create a payload to send to the things network
byte getPayload() 
{
    uint32_t interiorTemp = 12 * 100;
    uint32_t batteryTemp = 24 * 100;
    uint32_t battryVoltage = 11 * 100;

    payload[0] = highByte(interiorTemp);
    payload[1] = lowByte(interiorTemp);
    payload[2] = highByte(batteryTemp);
    payload[3] = lowByte(batteryTemp);
    payload[4] = highByte(battryVoltage);
    payload[5] = lowByte(battryVoltage);
}

// function to setup the temperature sensors
//void temperatureSensorSetup() {
//  while(!Serial)
//  // locating devices
//  deviceCount = sensors.getDeviceCount();
//  
//  for (int i = 0;  i < deviceCount;  i++)
//  {
//    sensors.getAddress(tempDeviceAddress, i);
//    getDeviceAddress(tempDeviceAddress);
//  }
//}
//
//void getDeviceAddress(DeviceAddress deviceAddress) 
//{
//  if(interior == NULL) 
//  {
//    for (uint8_t i = 0; i < 8; i++)
//    {
//      interior[i] = "0x";
//      if (deviceAddress[i] < 0x10) 
//      {
//        interior[i] = (interior[i] + "0");
//      }
//      interior[i] = (deviceAddress[i], HEX);
//      if (i < 7) 
//      {
//        interior[i] = (interior[i] + ", ");
//      }
//    } 
//  }
//  else
//  {
//    for (uint8_t i = 0; i < 8; i++)
//    {
//      battery[i] = "0x";
//      if (deviceAddress[i] < 0x10) 
//      {
//        battery[i] = (battery[i] + "0");
//      }
//      battery[i] = (deviceAddress[i], HEX);
//      if (i < 7) 
//      {
//        battery[i] = (battery[i] + ", ");
//      }
//    } 
//  }
//}

// function to get data from the temperature sensors
double getTemperature(DeviceAddress deviceAddress) 
{
  sensors.requestTemperatures();
  float tempC = sensors.getTempC(deviceAddress);
  return tempC;
}

// function to read the battery voltage
double getBatteryVoltage() 
{
  sensorValue = analogRead(voltageDivider);
  float voltage = (sensorValue * (batteryVoltage / 1024.0)) * ((R1 + R2) / R2);
  
  // return the value
  return voltage;
}

// function to restart the navigation system
void restartNavigationSystem() 
{
  digitalWrite(navigationSystem, LOW);
  delay(resetInterval);
  digitalWrite(navigationSystem, HIGH);
  delay(resetInterval);
}

// function to restart the mobile terminal
void restartMobileTerminal() 
{
  digitalWrite(mobileTerminal, LOW);
  delay(resetInterval);
  digitalWrite(mobileTerminal, HIGH);
  delay(resetInterval);
}

void onEvent (ev_t ev) 
{
    switch(ev) 
    {
        case EV_SCAN_TIMEOUT:
            display.clear();
            display.drawString(0,1,"EV_SCAN_TIMEOUT");
            break;
        case EV_BEACON_FOUND:
            display.clear();
            display.drawString(0,1,"EV_BEACON_FOUND");
            break;
        case EV_BEACON_MISSED:
            display.clear();
            display.drawString(0,1,"EV_BEACON_MISSED");
            break;
        case EV_BEACON_TRACKED:
            display.clear();
            display.drawString(0,1,"EV_BEACON_TRACKED");
            break;
        case EV_JOINING:
            display.clear();
            display.drawString(0,1,"EV_JOINING");
            break;
        case EV_JOINED:
            display.clear();
            display.drawString(0,1,"EV_JOINED");
            LMIC_setLinkCheckMode(0);
            break;
        case EV_JOIN_FAILED:
            display.clear();
            display.drawString(0,1,"EV_JOIN_FAILED");
            break;
        case EV_REJOIN_FAILED:
            display.clear();
            display.drawString(0,1,"EV_REJOIN_FAILED");
            break;
        case EV_TXCOMPLETE:
            display.clear();
            display.drawString(0,1,"EV_TXCOMPLETE");
            if (LMIC.dataLen) 
            {
              if (LMIC.dataLen == 1) 
              {
                uint8_t result = LMIC.frame[LMIC.dataBeg + 0];
                if (result == 48)  
                {
                  display.drawString(0,2,"RESETTING");
                  display.drawString(0,3,"NAVIGATION");
                  restartNavigationSystem();
                  display.drawString(0,4,"RESETTING");
                  display.drawString(0,5,"MOBILE TERMINAL");
                  restartMobileTerminal();
                }              
                if (result == 49)  
                {
                  display.drawString(0,2,"RESETTING");
                  display.drawString(0,3,"MOBILE TERMINAL");
                  restartMobileTerminal();                 
                } 
                if (result == 50)  
                {
                  display.drawString(0,2,"RESETTING");
                  display.drawString(0,3,"NAVIGATION");
                  restartNavigationSystem();               
                }                                      
              }
            }
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            display.clear();
            display.drawString(0,1,"EV_TXCOMPLETE");
            display.drawString(0,2,"WAITING TO SEND");
            display.drawString(0,3,"NEXT PAYLOAD");
            break;
        case EV_LOST_TSYNC:
            display.clear();
            display.drawString(0,1,"EV_LOST_TSYNC");
            break;
        case EV_RESET:
            display.clear();
            display.drawString(0,1,"EV_RESET");
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            display.clear();
            display.drawString(0,1,"EV_RXCOMPLETE");
            break;
        case EV_LINK_DEAD:
            display.clear();
            display.drawString(0,1,"EV_LINK_DEAD");
            break;
        case EV_LINK_ALIVE:
            display.clear();
            display.drawString(0,1,"EV_LINK_ALIVE");
            break;
        case EV_TXSTART:
            display.clear();
            display.drawString(0,1,"EV_TXSTART");
            break;
        case EV_TXCANCELED:
            display.clear();
            display.drawString(0,1,"EV_TXCANCELED");
            break;
        case EV_RXSTART:
            break;
        case EV_JOIN_TXCOMPLETE:
            display.clear();
            display.drawString(0,1,"EV_JOIN_TXCOMPLETE:");
            display.drawString(0,2,"no JoinAccept");
            break;
        default:
            display.clear();
            display.drawString(0,1,"Unknown event");
            break;
    }
}

void do_send(osjob_t* j) {
  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) 
  {
    display.clear();
    display.drawString(0,1,"OP_TXRXPEND");
    display.drawString(0,1,"not sending");
  } 
  else 
  {
    getPayload();
    // Prepare upstream data transmission at the next possible time.
    LMIC_setTxData2(1, payload, sizeof(payload), 0);
    display.clear();
    display.drawString(0,1,"PACKET QUEUED");
  }
  // Next TX is scheduled after TX_COMPLETE event.
}
