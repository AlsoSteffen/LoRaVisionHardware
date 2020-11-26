/*********
 * Name: Costandino Hiripis, Isaac Semackor and Christof Du Toit
 * Date: November 20, 2020 12:21PM
*********/
#include <OneWire.h>
#include <DS18B20.h>

#define mobileRelay 10
DS18B20 ds(A1);

void setup() {
  pinMode(10, OUTPUT);
  Serial.begin(9600);
  Serial.print("Devices: ");
  Serial.println(ds.getNumberOfDevices());
  Serial.println();
}

void loop() {
  while (ds.selectNext()) {
    Serial.print("ID: ");
    Serial.println(ds.getResolution());
    Serial.print("Temperature: ");
    Serial.print(ds.getTempC());
    Serial.print(" C ");
    Serial.println();
  }

  delay(5000);

  digitalWrite(mobileRelay, LOW);
  delay(1000);
  digitalWrite(mobileRelay, HIGH);
  delay(1000);
}
