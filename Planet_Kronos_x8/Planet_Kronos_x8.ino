//Planet_Kronos_x8 The Kazimier Dezember 2015 
//The MIT License (MIT)

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <OSCBundle.h>

#define DEBUG 1

#define NUM_MOTORS 2

// you can find this written on the board of some Arduino Ethernets or shields
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0E, 0x9B, 0x7F } ;

// NOTE: Alternatively, you can assign a fixed IP to configure your
//       Ethernet shield.
byte ip[] = { 192, 168, 0, 100 };


int serverPort  = 10000; //TouchOSC (incoming port)
int destPort = 9000;    //TouchOSC (outgoing port)

// motors Pins

int motorPinA[NUM_MOTORS] = {10, 9};
int motorPinB[NUM_MOTORS] = {11, 8};

//switc Pins
int switchPinA[NUM_MOTORS] = {12, 14};
int switchPinB[NUM_MOTORS] = {13, 15};


//Create UDP message object
EthernetUDP Udp;


typedef enum motorStates { // enum with motor States, just a list of Names constants
  FORWARD,
  BACKWARD,
  STOP
};

motorStates actualState[NUM_MOTORS];




void setup() {



  // start the Ethernet connection

  Ethernet.begin(mac, ip);
  Udp.begin(serverPort);

  //two pins to control the motor
  for (int i = 0; i < NUM_MOTORS; i++) {
    pinMode(motorPinA[i], OUTPUT);
    pinMode(motorPinB[i], OUTPUT);
    //two switchs INPUT,

    pinMode(switchPinA[i], INPUT);
    pinMode(switchPinB[i], INPUT);

    actualState[i] = STOP;  // start with STOP state

  }



#ifdef  DEBUG
  //Debug Mode enable in case DEBUG equal 1
  Serial.begin(19200);
#endif


}

void loop()
{

  //process received messages
  OSCMsgReceive();

  //constantly check the keys independent of the OSC messages
  //now check all motors and turn the motor off in case off the switchs are on

  for (int motorID = 0; motorID < NUM_MOTORS; motorID++) {

    boolean switchF = digitalRead(switchPinA[motorID]); // if switch is ON for both it stop and send back erro
    boolean switchB = digitalRead(switchPinB[motorID]);
    //if either switchs are on throw error
    
    if (switchF == 1 && actualState[motorID] == FORWARD) {
      sendMotorCommand(STOP, motorID); // motorID will be the index
#ifdef  DEBUG
      Serial.print("STOP ON LOOP MOTOR#:");
      Serial.println(motorID);

#endif
    }
    if (switchB == 1  && actualState[motorID] == BACKWARD) {
      sendMotorCommand(STOP, motorID); // motorID will be the index
#ifdef  DEBUG
      Serial.print("STOP ON LOOP MOTOR#:");
      Serial.println(motorID);

#endif
    }

  }

}

void OSCMsgReceive() {
  OSCMessage msgIN;
  int size;
  if ((size = Udp.parsePacket()) > 0) {
    while (size--)
      msgIN.fill(Udp.read());
    if (!msgIN.hasError()) {

      msgIN.route("/MotorIn/Forward", motorForward);
      msgIN.route("/MotorIn/Backward", motorBackward);
      msgIN.route("/MotorIn/Stop", motorStop);

    }
  }
}

void motorForward(OSCMessage &msg, int addrOffset) {

#ifdef  DEBUG
  Serial.println("FORWARD_OSC");
#endif

  //receive the motor ID from the OSC message
  int motorID;
  boolean error;

  if (msg.isInt(0)) //only if theres a number
  {
    motorID = msg.getInt(0); //get an integer from the OSC message
    //receive FORWARD update generalState and send back TRUE OSC message
    error = sendMotorCommand(FORWARD, motorID); //motor return 1 for OK 0 for error

  } else {

    error = 0; //trow an error
  }


#ifdef  DEBUG
  Serial.print("FORWARD_OSC: ");
  if (error == 0) {
    Serial.println("ERROR ERROR");
  } else {
    Serial.print("OK MOTOR#: ");
    Serial.println(motorID);
  }
#endif


  String msgText = "/MotorOut/Forward/" + motorID;
  OSCMessage msgOUT(msgText.c_str());

  msgOUT.add(error); // send TRUE we got the Foward Message


  Udp.beginPacket(Udp.remoteIP(), destPort);
  msgOUT.send(Udp);
  Udp.endPacket();
  msgOUT.empty();
}

void motorBackward(OSCMessage &msg, int addrOffset ) {

#ifdef  DEBUG
  Serial.println("BACKWARD_OSC");
#endif

  //receive the motor ID from the OSC message
  int motorID;
  boolean error;

  if (msg.isInt(0)) //only if theres a number
  {

    motorID = msg.getInt(0); //get an integer from the OSC message
    //receive BACKWARD update generalState and send back TRUE OSC message
    error = sendMotorCommand(BACKWARD, motorID); //motor return 1 for OK 0 for error

  } else {
    error = 0; //trow an error
  }

#ifdef  DEBUG
  Serial.print("BACKWARD_OSC: ");
  if (error == 0) {
    Serial.println("ERROR ERROR");
  } else {
    Serial.print("OK MOTOR#: ");
    Serial.println(motorID);
  }
#endif

  String msgText = "/MotorOut/Backward/" + motorID;
  OSCMessage msgOUT(msgText.c_str());

  msgOUT.add(error); // send TRUE we got the Foward Message

  Udp.beginPacket(Udp.remoteIP(), destPort);
  msgOUT.send(Udp);
  Udp.endPacket();
  msgOUT.empty();
}

void motorStop(OSCMessage &msg, int addrOffset ) {
  //receive STOP update generalState and send back TRUE OSC message
#ifdef  DEBUG
  Serial.println("STOP_OSC");
#endif

  int motorID;
  boolean error;

  if (msg.isInt(0)) //only if theres a number
  {

    motorID = msg.getInt(0); //get an integer from the OSC message
    //receive BACKWARD update generalState and send back TRUE OSC message
    error = sendMotorCommand(STOP, motorID); //motor return 1 for OK 0 for error

  } else {
    error = 0; //trow an error
  }


#ifdef  DEBUG
  Serial.print("STOP_OSC: ");
  if (error == 0) {
    Serial.println("ERROR ERROR");
  } else {
    Serial.print("OK MOTOR#: ");
    Serial.println(motorID);
  }


#endif

  String msgText = "/MotorOut/Stop/" + motorID;
  OSCMessage msgOUT(msgText.c_str());
  msgOUT.add(error); // send TRUE we got the Foward Message


  Udp.beginPacket(Udp.remoteIP(), destPort);
  msgOUT.send(Udp);
  Udp.endPacket();
  msgOUT.empty();
}


boolean sendMotorCommand(enum motorStates state, int motorID) {

  //first thig to check before sending new motor commands is.
  //the switches are pressed/
  //if any of then are pressed, just turnOFF the motor and return

  boolean switchF = digitalRead(switchPinA[motorID]); // if switch is ON for both it stop and send back erro
  boolean switchB = digitalRead(switchPinB[motorID]);

  //if either switchs are on throw erro
  if (switchF == 1 && state == FORWARD) {
    actualState[motorID] = STOP; // should stop

    //send motors to off
    digitalWrite(motorPinA[motorID], LOW);
    digitalWrite(motorPinB[motorID], LOW);


    return 0; //return 0 //error message

  }

  //if either switchs are on throw erro
  if (switchB == 1 && state == BACKWARD) {
    actualState[motorID] = STOP; // should stop

    //send motors to off
    digitalWrite(motorPinA[motorID], LOW);
    digitalWrite(motorPinB[motorID], LOW);


    return 0; //return 0 //error message

  }

  //only send motor messages if it' a new OSC message a new state

  if (actualState[motorID] != state) {
    actualState[motorID] = state; // update actual state

    switch (state) {
      case FORWARD:
        digitalWrite(motorPinA[motorID], HIGH);
        digitalWrite(motorPinB[motorID], LOW);

        break;
      case BACKWARD:

        //check this, not sure how to make your motor go Backward
        digitalWrite(motorPinA[motorID], LOW);
        digitalWrite(motorPinB[motorID], HIGH);
        break;
      case STOP:
        //dont need to check switchs to make it stop..

        //check how to make the motor STOP
        digitalWrite(motorPinA[motorID], LOW);
        digitalWrite(motorPinB[motorID], LOW);

        break;

    }

  }

  //read switchs and return error in case


  //if it gets here I hope everything is ok

  return 1; //return 1 for OK


}