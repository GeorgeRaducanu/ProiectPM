#include <DHT11.h>
#include <SPI.h>
#include <IRremote.h>

// asta adaug pt remote
const int RECV_PIN = 11;  // Pin connected to the IR receiver module
volatile bool commandReceived = false;
IRrecv irrecv(RECV_PIN);
//
DHT11 dht11(4);

void handleIRInterrupt() {
  commandReceived = true;
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  irrecv.enableIRIn();  // Start the IR receiver
  attachInterrupt(digitalPinToInterrupt(RECV_PIN), handleIRInterrupt, CHANGE);
}

void loop() {
  int temperature = dht11.readTemperature();

  if (temperature != DHT11::ERROR_CHECKSUM && temperature != DHT11::ERROR_TIMEOUT) {
    Serial.print("Temperatura: ");
    Serial.print(temperature);
    Serial.println(" °C");
  } else {
    // Print error message based on the error code.
    Serial.println(DHT11::getErrorString(temperature));
  }
  if (commandReceived) {
    // Process the received command
    // For example, print the received code
    if (irrecv.decode()) {
      Serial.println(irrecv.decodedIRData.decodedRawData);
      irrecv.resume();  // Receive the next value
    }
    commandReceived = false;
  }
  delay(1000);
}
