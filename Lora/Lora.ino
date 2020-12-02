/*********
 * Name: Costandino Hiripis, Isaac Semackor and Christof Du Toit
 * Date: December 2, 2020 18:21PM
*********/

#include <OneWire.h>
#include <DallasTemperature.h>

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
#define resetInterval 5000

// Creating a oneWire instance(object)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire object reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

//To store number of sensor connected
int numberOfDevices;

//To store data from the voltage divider circuit
int sensorValue;

// Variable to store a single sensor address
DeviceAddress tempDeviceAddress; 

void setup() {
 
  Serial.begin(9600);
  sensors.begin();
  
  // calling the function temperatureSensorSetup
  temperatureSensorSetup();

  pinMode(navigationSystem, OUTPUT);
  pinMode(mobileTerminal, OUTPUT);
  
}

void loop() { 
  
  // calling the function getTemperature
  getTemperature();

  // calling the function getBatteryVoltage
  getBatteryVoltage();

  // calling the function restartMobileTerminal
  restartMobileTerminal();

  // calling the function restartNavigationSystem
  restartNavigationSystem();
}

// function to setup the temperature sensors
void temperatureSensorSetup() {
  while(!Serial)
  // Get the number of sensors connected to the the wire( digital pin 1)
  numberOfDevices = sensors.getDeviceCount();
  
  Serial.print(numberOfDevices, DEC);
  Serial.println(" temperature devices.");

  // Loop through each sensor and print out address
  for(int i=0; i<numberOfDevices; i++) {
    
    // Search the data wire for address and store the address in "tempDeviceAddress" variable
    if(sensors.getAddress(tempDeviceAddress, i)) {
      
      Serial.print("Found device ");
      Serial.print(i, DEC);
      Serial.print(" with address: ");
      printAddress(tempDeviceAddress);
      Serial.println();
      
    } else {
      
      Serial.print("Found ghost device at ");
      Serial.print(i, DEC);
      Serial.print(" but could not detect address. Check power and cabling");
      
    }
    
  }
  
}

// function to get data from the temperature sensors 
void getTemperature() {
  
  sensors.requestTemperatures(); // Send the command to get temperatures from all sensors.
  
  // Loop through each device, print out temperature one by one
  for(int i=0; i<numberOfDevices; i++) {
    
    // Search the wire for address and store the address in tempDeviceAddress
    if(sensors.getAddress(tempDeviceAddress, i)){
    
      Serial.print("Temperature from sensor number: ");
      Serial.println(i,DEC);
      Serial.print("Temperature from sensor address: ");
      printAddress(tempDeviceAddress);
      Serial.println();
      

      // Print the temperature
      float tempC = sensors.getTempC(tempDeviceAddress); //Temperature in degree celsius
      Serial.print("Temp C: ");
      Serial.println(tempC);

    }   
    
  }
  delay(1000);
  
}

// function to print a sensor address
void printAddress(DeviceAddress deviceAddress) {
  
  for (uint8_t i = 0; i < 8; i++) {
    
    if (deviceAddress[i] < 16) 
      Serial.print("0");
      Serial.print(deviceAddress[i], HEX);
      
  }
  
}

// function to read the battery voltage
void getBatteryVoltage() {
 
  sensorValue = analogRead(voltageDivider);
  float voltage = (sensorValue * (batteryVoltage / 1024.0)) * ((R1 + R2)/ R2);
  // print out the value you read:
  Serial.print("Battery voltage: ");
  Serial.println(voltage);
  delay(1000);
  
}

// function to restart the navigation system
void restartNavigationSystem(){

  digitalWrite(navigationSystem, LOW);
  Serial.print("Navigation system state: ");
  Serial.println("ON");
  delay(resetInterval);    
  digitalWrite(navigationSystem, HIGH);
  Serial.print("Navigation system state: ");
  Serial.println("OFF");
  delay(resetInterval);
  
}

// function to restart the mobile terminal
void restartMobileTerminal(){

  digitalWrite(mobileTerminal, LOW);
  Serial.print("Mobile terminal state: ");
  Serial.println("ON");
  delay(resetInterval);
  digitalWrite(mobileTerminal, HIGH);
  Serial.print("Mobile terminal state: ");
  Serial.println("OFF");
  delay(resetInterval);
  
}
