// Enable debug prints to serial monitor
#define MY_DEBUG

// Must Define Node ID either numerical 1-255 or AUTO.
#define MY_NODE_ID 2

// Enable RS485 transport layer in lieu of RF.
#define MY_RS485

// Define this to enables DE-pin management on defined pin
#define MY_RS485_DE_PIN 2

// Set RS485 baud rate to use
#define MY_RS485_BAUD_RATE 9600

#include <SPI.h>
#include <MySensors.h>
#include <Bounce2.h>

//Setup Relay
#define RELAY_PIN  6  // Arduino Digital I/O pin number for relay
//#define BUTTON_PIN  5  // Arduino Digital I/O pin number for button
#define RELAY_CHILD_ID_1 6   // Id of the sensor child
#define RELAY_ON 1
#define RELAY_OFF 0

//Setup Reed Switches
#define CHILD_ID2 4
#define BUTTON_PIN2  4  // Arduino Digital I/O pin for button/reed switch
#define CLOSED 0
#define OPEN 1

//Setup PIR
#define MOTION_CHILD_ID4 3
#define MOTION_DIO 3   // The digital input you attached your motion sensor.  (Only 2 and 3 generates interrupt!)
//uint32_t SLEEP_TIME = 120000; // Sleep time between reports (in milliseconds)
//#define INTERRUPT DIGITAL_INPUT_SENSOR-2 // Usually the interrupt = pin -2 (on uno/nano anyway)


Bounce debouncer2 = Bounce();
int oldValue2=-1;

bool state;
bool initialValueSent = false;
boolean lastTripped = 0;
//byte StatePIR=0;
//byte oldStatePIR=0;


MyMessage msg1(RELAY_CHILD_ID_1, V_STATUS);
MyMessage msg2(CHILD_ID2, V_TRIPPED);
MyMessage msg4(MOTION_CHILD_ID4, V_TRIPPED);

void setup()
{
  // Setup the button
  pinMode(BUTTON_PIN2,INPUT);
  // Activate internal pull-up
  digitalWrite(BUTTON_PIN2,HIGH);

  // After setting up the button, setup debouncer
  debouncer2.attach(BUTTON_PIN2);
  debouncer2.interval(5);

  // Make sure relays are off when starting up
  digitalWrite(RELAY_PIN, RELAY_OFF);
  // Then set relay pins in output mode
  pinMode(RELAY_PIN, OUTPUT);

  // Set relay to last known state (using eeprom storage)
  state = loadState(RELAY_CHILD_ID_1);
  digitalWrite(RELAY_PIN, state?RELAY_ON:RELAY_OFF);

  pinMode(MOTION_DIO,INPUT);      // sets the motion sensor digital pin as input
}

void presentation()
{
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Garage Multi Sensor", "1.0");

  // Register all sensors to gw (they will be created as child devices)
  present(RELAY_CHILD_ID_1, S_BINARY);
  present(CHILD_ID2, S_DOOR);
  present(MOTION_CHILD_ID4, S_MOTION);
}

/*
*  Example on how to asynchronously check for new messages from gw
*/
void loop()
{
  if (!initialValueSent) {
    Serial.println("Sending initial value");
    send(msg1.set(state?RELAY_ON:RELAY_OFF));
    send(msg2.set(state?OPEN:CLOSED));
    Serial.println("Requesting initial value from controller");
    request(RELAY_CHILD_ID_1, V_STATUS);
    request(CHILD_ID2, V_STATUS);
    request(MOTION_CHILD_ID4, V_STATUS);
    wait(2000, C_SET, V_STATUS);
  }
  debouncer2.update();
  // Get the update value
  int value = debouncer2.read();

  if (value != oldValue2) {
      send(msg2.set(value==HIGH ? 1 : 0));
      //send(msg.set(state?false:true), true); // Send new state and request ack back
      oldValue2 = value;
  }

  // Check for motion change value
    boolean tripped = digitalRead(MOTION_DIO) == HIGH;

    if (lastTripped != tripped ) {
      send(msg4.set(tripped?"1":"0")); // Send new state and request ack back
      Serial.println("Tripped");
      lastTripped=tripped;
    }


}

void receive(const MyMessage &message) {
  // We only expect one type of message from controller. But we better check anyway.
  if (message.isAck()) {
     Serial.println("This is an ack from gateway");
  }
  if (message.type == V_STATUS) {
    if (!initialValueSent) {
      Serial.println("Receiving initial value from controller");
      initialValueSent = true;
    }
  }
  if (message.type == V_STATUS) {
     // Change relay state
     state = message.getBool();
     // Uncomment below for Relay to stay on.
     //digitalWrite(RELAY_PIN, state?RELAY_ON:RELAY_OFF);

     // Below Allows Relay to be momentary. For Garage Door.
     digitalWrite(RELAY_PIN, RELAY_ON);
     wait (500); //Adjust to allow relay to stay closed longer.
     digitalWrite(RELAY_PIN, RELAY_OFF);

     // Store state in eeprom
     saveState(RELAY_CHILD_ID_1, state);
     saveState(CHILD_ID2, state);

     // Write some debug info
     Serial.print("Incoming change for sensor:");
     Serial.print(message.sensor);
     Serial.print(", New status: ");
     Serial.println(message.getBool());
   }
}
