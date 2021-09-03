// Test code to control MD_OnePin SEC node
// 
// This implements an ISR with timing that matches the expected 
// link handshaking with PRI to send and receive data packets.
// 
// This application can serve as the start of a SEC application 
// that does something useful with the data exchanges.
//
#include <MD_OnePin_Protocol.h>

#ifndef OP_DEBUG_DIGITAL
#define OP_DEBUG_DIGITAL 0      ///< 1 turns digital I/O debug output on
#endif

#if OP_DEBUG_DIGITAL
#define DBG_PIN 7
#define DBG_FLIP digitalWrite(DBG_PIN, digitalRead(DBG_PIN) ? LOP : HIGH)
#else
#define DBG_FLIP
#endif

// A SEC may be read-only, in which case it will not respond to write messages.
#define SEC_IS_READ_ONLY  0   // set to 1 for READ_ONLY functionality

// The comms pin needs to be an external interrupt pin.
// See https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/
// for valid pins for different architectures.
const uint8_t OP_PIN = 2;

// SEC packet size and typedef.
// Override the default values for this SEC by chaning here
const uint8_t MY_BPP_SEC = BPP_SEC;
typedef opSeciPacket_t mySecPacket_t;

// ---- Macros for local inline code (time critical sections)
#define SET_TO_INPUT  pinMode(OP_PIN, INPUT_PULLUP)
#define SET_TO_OUTPUT pinMode(OP_PIN, OUTPUT)

// ---- ISR coordination variables shared ISR/main code
volatile bool signalStartDetected = false;       // flag for main program to detect and process packet
volatile uint32_t timeSignalDuration = 0;        // duration of the signal

// ---- ISR for the change on the OnePin link pin
// The ISR is called on a change to the input.
// The falling edge is the start of a signal and the time
// to the rising edge determines the type of signal it is.
void pinISR(void)
{
  static bool waitingNewSignal = true;  // true if we are idle waiting for a new signal to start
  static uint32_t timeStart;            // signal start time in micros()

  if (waitingNewSignal)
  {
    // Waiting for start of signal. 
    // This will be a transition from HIGH to LOP, so we check if 
    // the signal is LOP here before committing starting the timed wait.
    if (digitalRead(OP_PIN) == LOW)
    {
      waitingNewSignal = false;
      timeStart = micros();
    }
  }
  else
  {
    // Timing the duration of the LOP signal detected earlier.
    // The ISR is triggered when the signal changes to HIGH, so  
    // we time how long it took from IDLE to now.
    timeSignalDuration = micros() - timeStart;

    signalStartDetected = true;
    waitingNewSignal = true;         // go back to waiting for a new signal start
  }
}

void setup(void)
{
  Serial.begin(57600);
#if OP_DEBUG_DIGITAL
  pinMode(DBG_PIN, OUTPUT);
  digitalWrite(DBG_PIN, LOP);
#endif

  SET_TO_INPUT;
  attachInterrupt(digitalPinToInterrupt(OP_PIN), pinISR, CHANGE);
}

void loop(void)
{
  // ---- Comms data coordination
  static mySecPacket_t sndData = 0;
  static opPriPacket_t rcvData = 0;
  static uint8_t bit = 0;
  bool allRcv = false;    // all bits received from PRI
  bool allSnd = false;    // all bits sent from PRI

  if (signalStartDetected)
  {
    signalStartDetected = false;  // working on it now
    // AVR interrupts are active whether the pin is input or output. As we
    // may need to change the pin function and write data back, disable
    // the interrupt during while processing.
    detachInterrupt(digitalPinToInterrupt(OP_PIN));

    // Process the signal based on start duration (in increasing order)
#if !SEC_IS_READ_ONLY
    if (timeSignalDuration <= OPT_WR1_DETECT)      // less than the Write1 signal threshold
    {
      rcvData |= (1 << bit);
      bit++;
      allRcv = (bit == BPP_PRI);
    }
    else if (timeSignalDuration <= OPT_WR0_DETECT) // less than the Write0 signal threshold
    {
      bit++;
      allRcv = (bit == BPP_PRI);
    }
    else if (timeSignalDuration <= OPT_RD_DETECT)  // less than the Read request signal threshold
#else  // READ_ONLY
    if ((timeSignalDuration > OPT_WR0_DETECT) && (timeSignalDuration <= OPT_RD_DETECT))
#endif
    {
      SET_TO_OUTPUT;
      digitalWrite(OP_PIN, sndData & (1 << bit) ? HIGH : LOW);
      delayMicroseconds(OPT_RD0_SIGNAL);
      SET_TO_INPUT;
      DBG_FLIP;
      bit++;
      allSnd = (bit == MY_BPP_SEC);
    }
    else                              // the only thing left is a Reset signal
    {
      SET_TO_OUTPUT;
      digitalWrite(OP_PIN, LOW);
      delayMicroseconds(OPT_RST_PRESENCE);
      SET_TO_INPUT;    // with PULLUP sets this high
      rcvData = bit = 0;
    }

    // if we have received or sent a whole packet,
    // deal with the data and reset accumulation counters.
    if (allRcv)
    {
      opPriPacket_t dataNew = rcvData;  // copy data in case ISR gets called

      Serial.write('\n');
      Serial.print(dataNew, HEX);
    }
    if (allSnd)
    {
      sndData++;
    }

    // start processing interrupts again
    attachInterrupt(digitalPinToInterrupt(OP_PIN), pinISR, CHANGE);
  }
}

