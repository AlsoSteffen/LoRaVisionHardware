/*********
   Name: Costandino Hiripis
   Date: June 17, 2020 19:23
*********/

#include <OneWire.h>
#include <DallasTemperature.h>
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <U8x8lib.h>

// Addresses of the DS18B20 sensors
DeviceAddress interior = { FILLMEIN };
DeviceAddress battery = { FILLMEIN };

// This key should be in little endian format(lsb)
static const u1_t PROGMEM APPEUI[8] = { FILLMEIN };
void os_getArtEui (u1_t* buf) 
{
  memcpy_P(buf, APPEUI, 8);
}

// This key should be in little endian format(lsb)
static const u1_t PROGMEM DEVEUI[8] = { FILLMEIN };
void os_getDevEui (u1_t* buf) 
{
  memcpy_P(buf, DEVEUI, 8);
}

// This key should be in big endian format(msb)
static const u1_t PROGMEM APPKEY[16] = { FILLMEIN };
void os_getDevKey (u1_t* buf) 
{
  memcpy_P(buf, APPKEY, 16);
}

// All DS18B20 Sensors are connected to pin 1 on the LoRa32u4II board
#define ONE_WIRE_BUS 22

//Defining the value of R1 in the voltage divider circuit
float R1 = 330000.0;

//Defining the value of R2 in the voltage divider circuit
float R2 = 47000.0;

//Defining the pin that is used to read the battery voltage
#define voltageDivider 36

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

//To store data from the voltage divider circuit
int sensorValue;

//To store the downlink
String HexDownlink = "";

//Creating the display  
U8X8_SSD1306_128X64_NONAME_HW_I2C display(/*rst*/ 16, /*scl*/ 15, /*sda*/ 4);

byte payload[6];

static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty cycle limitations).
const unsigned TX_INTERVAL = 898.750;

// Pin mapping
const lmic_pinmap lmic_pins = { 
  .nss = 18, 
  .rxtx = LMIC_UNUSED_PIN, 
  .rst = 14, 
  .dio = {/*dio0*/ 26, /*dio1*/ 33, /*dio2*/ 32} 
  };

void setup() 
{
  Serial.begin(9600);
  sensors.begin();
  display.begin();
  display.setFont(u8x8_font_victoriamedium8_r);
  
  pinMode(navigationSystem, OUTPUT);
  pinMode(mobileTerminal, OUTPUT);

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
    uint32_t interiorTemp = getTemperature(interior) * 100;
    uint32_t batteryTemp = getTemperature(battery) * 100;
    uint32_t battryVoltage = getBatteryVoltage() * 100;

    payload[0] = highByte(interiorTemp);
    payload[1] = lowByte(interiorTemp);
    payload[2] = highByte(batteryTemp);
    payload[3] = lowByte(batteryTemp);
    payload[4] = highByte(battryVoltage);
    payload[5] = lowByte(battryVoltage);
}

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
  //Multiply the analog value by the number of millivolts per tick 
  float voltage = twoHundredAndFifty() * 0.0063492063492063;
  
  // return the value
  return voltage;
}

float twoHundredAndFifty()
{
  int sumOfSensorValue = 0.0;
  for(int i = 0; i < 250; i++)
  {
    sensorValue = analogRead(voltageDivider);
    sumOfSensorValue += sensorValue;
    delay(5);
  }
  return sumOfSensorValue/250;
} 

// function to restart the navigation system
void restartNavigationSystem() 
{
  digitalWrite(navigationSystem, HIGH);
  delay(resetInterval);
  digitalWrite(navigationSystem, LOW);
  delay(resetInterval);
}

// function to restart the mobile terminal
void restartMobileTerminal() 
{
  digitalWrite(mobileTerminal, HIGH);
  delay(resetInterval);
  digitalWrite(mobileTerminal, LOW);
  delay(resetInterval);
}

// function to convert hex to decimal
unsigned int hexToDec(String hexString) {

  unsigned int decValue = 0;
  int nextInt;

  for (int i = 0; i < hexString.length(); i++) {

    nextInt = int(hexString.charAt(i));
    if (nextInt >= 48 && nextInt <= 57) nextInt = map(nextInt, 48, 57, 0, 9);
    if (nextInt >= 65 && nextInt <= 70) nextInt = map(nextInt, 65, 70, 10, 15);
    if (nextInt >= 97 && nextInt <= 102) nextInt = map(nextInt, 97, 102, 10, 15);
    nextInt = constrain(nextInt, 0, 15);

    decValue = (decValue * 16) + nextInt;
  }

  return decValue;
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
                //Storing the hex in a variable
                for (int i = 0; i < LMIC.dataLen; i++) {
                   HexDownlink += String(LMIC.frame[LMIC.dataBeg + i], HEX);
                }

                  if (hexToDec(HexDownlink) == 48)  
                  {
                    display.drawString(0,2,"RESETTING");
                    display.drawString(0,3,"NAVIGATION");
                    restartNavigationSystem();
                    display.drawString(0,4,"RESETTING");
                    display.drawString(0,5,"MOBILE TERMINAL");
                    restartMobileTerminal();
                  }              
                           
                  if (hexToDec(HexDownlink) == 49) 
                  {
                    display.drawString(0,2,"RESETTING");
                    display.drawString(0,3,"MOBILE TERMINAL");
                    restartMobileTerminal();                 
                  } 
                  
                  if (hexToDec(HexDownlink) == 50)  
                  {
                    display.drawString(0,2,"RESETTING");
                    display.drawString(0,3,"NAVIGATION");
                    restartNavigationSystem();               
                  }  
                  
                  HexDownlink = "";
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

void do_send(osjob_t* j) 
{
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
