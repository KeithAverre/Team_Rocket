/******************************************************************************
  File: GroundProRFv5.ino
  Board: Ground Pro-RF 
  Task 1: Recieve datastream from on-board Pro-RF via RF transmission
  Version Info: v5.0 11/6/22 16:51
  Notes: Based on example client radio 2
  Author: David Schull
  References:
  Fischer Moseley @ SparkFun Electronics
******************************************************************************/

#include <RadioHead.h>
#include <SPI.h>
#include <RH_RF95.h> 

RH_RF95 rf95(12, 6); // RFM95 module's chip select and interrupt pins
float frequency = 915; // Broadcast frequency 915MHz

void setup()
{

  SerialUSB.begin(115200); // Open USB serial port, set baud rate to 115200 bps

  //Initialize the Radio.
  if (rf95.init() == false){
    SerialUSB.println("Radio Init Failed - Freezing");
    while (1);
  }
  else{
    SerialUSB.println("Transmitter up!"); 
  }

  // Set frequency
  rf95.setFrequency(frequency);
  rf95.setTxPower(14, false);
}


void loop()
{
  //SerialUSB.println("Sending message");

  //Send a message to the other radio
  //uint8_t toSend[] = "Requesting Flight Data Packet";
  //rf95.send(toSend, sizeof(toSend));
  //rf95.waitPacketSent();

  // Now wait for a reply
  //byte buf[RH_RF95_MAX_MESSAGE_LEN];
  byte buf[251];
  byte len = sizeof(buf);

  if (rf95.available()) {
    // Should be a reply message for us now
    if (rf95.recv(buf, &len)) {
      SerialUSB.print("Received packet: ");
      SerialUSB.println((char*)buf);
      //SerialUSB.print(" RSSI: ");
      //SerialUSB.print(rf95.lastRssi(), DEC);
    }
  }
  delay(250);
}
