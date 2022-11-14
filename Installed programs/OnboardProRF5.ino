/******************************************************************************
  File: OnboardProRFV5.ino
  Board: On-Board Pro-RF 
  Task 1: Recieve datastream from on-board Redboard Artemis via serial UART RX/TX
  Task 2: Recieve datastream to ground Pro-RF via RF transmission
  Version Info: v5.0 11/6/22 16:51
  Notes: Based on example server client 1
  Author: David Schull
  References:
  Fischer Moseley @ SparkFun Electronics
******************************************************************************/

// RADIO CONFIGURATION ////////////////////////////////////////////////////////
#include <RadioHead.h>

#include <SPI.h>

#include <RH_RF95.h>

RH_RF95 rf95(12, 6); // RFM95 module's chip select and interrupt pins
float frequency = 915; // Broadcast frequency 915MHz
char receivedChars[251]; // Char array to store serial data from Redboard
boolean newData = false; // Flow control
int packetCounter = 0; // Counts how many datastream packets are recieved
int ndxLength; // Stores the legth of the recived flight packet

void setup() {

  Serial1.begin(115200); // Open UART RX port, set baud rate to 115200 bps
  SerialUSB.begin(115200); // Open USB serial port, set baud rate to 115200 bps

  //Initialize the Radio. 
  if (rf95.init() == false) {
    //SerialUSB.println("Radio Init Failed - Freezing");
    while (1);
  } 
  else {
    SerialUSB.println("Receiver up!");
  }

  rf95.setFrequency(frequency);
}

void loop() {
  recordFlightDataPacket();
  showFlightDataPacket();
  sendFlightDataPacketRadio();
}

void recordFlightDataPacket() {
  static int ndx = 0;
  char endMarker = '\n';
  char rc;

  while (Serial1.available() > 0 && newData == false) {
    rc = Serial1.read();
    // maybe putsomething in here to look for starting char?
    if (rc != endMarker) {
      receivedChars[ndx] = rc;
      ndx++;
    } else {
      receivedChars[ndx] = '\0'; // terminate the string
      ndx = 0;
      newData = true;
      packetCounter++;
    }
  }
}

void showFlightDataPacket() {
  if (newData == true) {
    SerialUSB.print("Datastream from OB-FDS Redboard: ");
    SerialUSB.println(receivedChars);
    //newData = false;
  }
}


void sendFlightDataPacketRadio() {
  //if (rf95.available() && newData == true) {
  if (newData == true){ 
    rf95.waitPacketSent();
    rf95.send((uint8_t * ) receivedChars, 251);
    // SerialUSB.print("Flight Data Packet ");
    //SerialUSB.print(packetCounter);
    //SerialUSB.print(" sent.");
    // SerialUSB.println("");
    newData = false;
   delay(250);
  }

}