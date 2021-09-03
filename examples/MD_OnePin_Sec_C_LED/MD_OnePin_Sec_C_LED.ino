// MD_OnePin example SEC node
// 
// An example sketch written in C that implements an ISR to detect the message 
// start, with the rest of the message processed in the loop() function.
//
// The application flashes a LED at a rate determined by an internal variable.
// Data writes from the PRI will set a new rate in milliseconds for the flash 
// rate. Data Reads from the PRI will cause the SEC to send the current flash 
// count.
//
// The MD_OnePin_Test_Pri sketch can be used to write and read this SEC node.
// 
#include <MD_OnePin_Protocol.h>

// The comms pin needs to be an external interrupt pin.
// See https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/
// for valid pins for different architectures.
const uint8_t OP_PIN = 2;

// SEC packet size and typedef.
// Override the default values for this SEC by chaning here
const uint8_t MY_BPP_SEC = BPP_SEC;
typedef opSecPacket_t mySecPacket_t;

// LED management parameters
const uint8_t LED_PIN = 4;
uint32_t timeLedFlash = 1000;   // LED flash time
uint32_t timeLedStart = 0;      // LED base time for period
mySecPacket_t flashCount = 0;   // LED flash count

//#define DEBUG
const uint8_t DBG_PIN = 3;
const uint8_t LATCH_PIN = 2;
#ifdef DEBUG
// Debugging LEDs

#define DBG_SET(p)    digitalWrite(p, HIGH)
#define DBG_CLR(p)    digitalWrite(p, LOW)
#define DBG_TOGGLE(p) digitalWrite(p, digitalRead(p) == LOW ? HIGH : LOW)
#else
#define DBG_SET(p)
#define DBG_CLR(p)
#define DBG_TOGGLE(P)
#endif

// ---- Macros for local inline code (time critical sections)
#define SET_INPUT(p)  pinMode(p, INPUT_PULLUP)
#define SET_OUTPUT(p) pinMode(p, OUTPUT)

// ---- ISR for the change on the OnePin link pin
// The ISR is called on a change to the external input.
// The falling edge is the start of a signal and the time
// to the rising edge identifies the type of signal.
volatile bool processSignal = false;       // flag for main program to process packet
volatile uint32_t timeSignalDuration = 0;  // duration of the signal

void pinISR(void)
{
  static uint8_t ISRstate = 0;
  static uint32_t timeStart;    // signal start time in micros()

  if (!processSignal)
  {
    switch (ISRstate)
    {
    default:
      // Waiting for start of signal. 
      // This will be a transition from HIGH to LOW, so we check if 
      // the signal is LOW here before committing staing the timed wait.
      if (digitalRead(OP_PIN) == LOW)
      {
        ISRstate = 1;
        timeStart = micros();
      }
      break;

    case 1:
      // Timing the duration of the LOW dignal detected earlier.
      // The ISR is triggered when the signal changes to HIGH, so  
      // we time how long it took from IDLE to now.
      timeSignalDuration = micros() - timeStart;
      ISRstate = 0;
      processSignal = true;   // tell main loop to process
      DBG_TOGGLE(LATCH_PIN);
      break;
    }
  }
}

void setup(void)
{
  SET_OUTPUT(LED_PIN);
#ifdef DEBUG
  SET_OUTPUT(DBG_PIN);
  SET_OUTPUT(LATCH_PIN);
#endif

  SET_INPUT(OP_PIN);
  attachInterrupt(digitalPinToInterrupt(OP_PIN), pinISR, CHANGE);
}

void loop(void)
{
  // ---- Comms data coordination
  static mySecPacket_t sndData = 0;
  static opPriPacket_t rcvData = 0;
  static uint8_t bit = 0;
  bool allRcv = false;    // all bits received from PRI

  // handle the LED
  if (millis() - timeLedStart >= timeLedFlash)
  {
    digitalWrite(LED_PIN, digitalRead(LED_PIN) == LOW ? HIGH : LOW);
    flashCount++;
    timeLedStart = millis();
  }

  // handle the comms
  if (processSignal)
  {
    // Process the signal based on start duration
    if (timeSignalDuration <= OPT_WR1_DETECT)      // less than the Write1 signal threshold
    {
      rcvData |= (1 << bit);
      bit++;
      DBG_TOGGLE(LATCH_PIN);
      allRcv = (bit == BPP_PRI);
    }
    else if (timeSignalDuration <= OPT_WR0_DETECT) // less than the Write0 signal threshold
    {
      bit++;
      DBG_TOGGLE(LATCH_PIN);
      allRcv = (bit == BPP_PRI);
    }
    else if (timeSignalDuration <= OPT_RD_DETECT)  // less than the Read request signal threshold
    {
      if (bit == 0) sndData = flashCount;         // we are just starting a new run, get latest data
      SET_OUTPUT(OP_PIN);
      digitalWrite(OP_PIN, sndData & (1 << bit) ? HIGH : LOW);
      delayMicroseconds(OPT_RD0_SIGNAL);
      SET_INPUT(OP_PIN);
      DBG_TOGGLE(LATCH_PIN);
      bit++;
    }
    else                              // the only thing left is a Reset signal
    {
      SET_OUTPUT(OP_PIN);
      digitalWrite(OP_PIN, LOW);
      delayMicroseconds(OPT_RST_PRESENCE);
      SET_INPUT(OP_PIN);              // with PULLUP sets this high
      DBG_TOGGLE(LATCH_PIN);
      rcvData = bit = 0;
    }

    // if we have received/sent a whole packet, deal with the data
    if (allRcv)
      timeLedFlash = rcvData;

    processSignal = false;     // reset the flag to stop blocking IS
  }
}

