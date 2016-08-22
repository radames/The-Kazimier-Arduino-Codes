#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// called this way, it uses the default address 0x40
// you can also call it with a different address you want
Adafruit_PWMServoDriver servo1 = Adafruit_PWMServoDriver(0x40);
Adafruit_PWMServoDriver servo2 = Adafruit_PWMServoDriver(0x42);
Adafruit_PWMServoDriver light1 = Adafruit_PWMServoDriver(0x43);
Adafruit_PWMServoDriver light2 = Adafruit_PWMServoDriver(0x44);

// Depending on your servo make, the pulse width min and max may vary, you
// want these to be as small/large as possible without hitting the hard stop
// for max range. You'll have to tweak them as necessary to match the servos you
// have!
#define SERVOMIN  150 // this is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  600 // this is the 'maximum' pulse length count (out of 4096)

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <OSCBundle.h>

#define DEBUG 1

// you can find this written on the board of some Arduino Ethernets or shields
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0E, 0x9B, 0x7F } ;

// NOTE: Alternatively, you can assign a fixed IP to configure your
//       Ethernet shield.
byte ip[] = { 192, 168, 0, 110 };


int serverPort  = 11000; //TouchOSC (incoming port)
int destPort = 9000;    //TouchOSC (outgoing port)


//Create UDP message object
EthernetUDP Udp;



void setup() {



  // start the Ethernet connection

  Ethernet.begin(mac, ip);
  Udp.begin(serverPort);



#ifdef  DEBUG
  //Debug Mode enable in case DEBUG equal 1
  Serial.begin(19200);
#endif
  //Servos
  servo1.begin();
  servo1.setPWMFreq(60);  // Analog servos run at ~60 Hz updates
  servo2.begin();
  servo2.setPWMFreq(60);  // Analog servos run at ~60 Hz updates
  //ligths
  light1.begin();
  light1.setPWMFreq(1600);  // Light
  light2.begin();
  light2.setPWMFreq(1600);  // Light

  yield();

}

void loop()
{

  //process received messages
  OSCMsgReceive();



}
//receive OSC messages end send it to the PWM Driver
//Format 
//Servo/Number - Float 0-1 Angle
//Light/Number - Float 0-1 Ligth intensitiy
void OSCMsgReceive() {

  OSCMessage msgIN;
  int size;
  if ((size = Udp.parsePacket()) > 0) {

    while (size--)
      msgIN.fill(Udp.read());
    if (!msgIN.hasError()) {

      msgIN.route("/Servo", servoSet);
      msgIN.route("/Light", lightSet);

    }
  }
}



void servoSet(OSCMessage &msg, int addrOffset ) {
  for (int i = 0; i < 32; i++) {
    String servoNum = "/Servo/" + String(i);
    if (msg.fullMatch(servoNum.c_str())) {
      float fValue = msg.getFloat(0); //Float 0,1 
      int pulselen = SERVOMIN + (SERVOMAX-SERVOMIN)*fValue; //a number between 0 4096, adafruit example puts 150, 600
      
      if (i < 16) {
        servo1.setPWM(i % 16, 0, pulselen);
      } else {
        servo2.setPWM(i % 16, 0, pulselen);
      }
#ifdef  DEBUG
      Serial.println("Servo: " + servoNum + " " + String(pulselen));
#endif
    }
  }
}

void lightSet(OSCMessage &msg, int addrOffset ) {
  for (int i = 0; i < 32; i++) {
    String lightNum = "/Light/" + String(i);
    if (msg.fullMatch(lightNum.c_str())) {
      float fValue = msg.getFloat(0); 
      int pulselen = fValue*4096;
     
      if (i < 16) {
        light1.setPin(i % 16, pulselen, 1); // setPin special do deal with 0
      } else {
        light2.setPin(i % 16, pulselen, 1);
      }
#ifdef  DEBUG
      Serial.println("Light: " + lightNum + " " + String(pulselen));
#endif
    }
  }
}