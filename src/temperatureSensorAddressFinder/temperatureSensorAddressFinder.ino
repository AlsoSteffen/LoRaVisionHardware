#include <OneWire.h>

// Data wire is plugged into port 22 on the TTGO
#define ONE_WIRE_BUS 22

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

byte deviceAddress[8];

void setup()
{
  // start serial port
  Serial.begin(9600);
}

void loop()
{
  if (!oneWire.search(deviceAddress)) 
  {
    Serial.println(" No more addresses");
    Serial.println();
    oneWire.reset_search();
    delay(250);
    return;
  }
  
  Serial.print(" Device Address = ");

  for (uint8_t i = 0; i < 8; i++) 
  {
    Serial.print("0x");
    if (deviceAddress[i] < 0x10) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
    if (i < 7) Serial.print(", ");
  }
  Serial.println("");
}
