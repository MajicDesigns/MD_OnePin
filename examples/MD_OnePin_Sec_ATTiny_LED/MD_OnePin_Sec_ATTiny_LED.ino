// MD_OnePin example SEC node
// Tailored and tested with ATTiny 13 and 85 processor.
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
#include <avr/interrupt.h>
#include <MD_OnePin_Protocol.h>

// The comms pin needs to be an external interrupt pin.
// Pins for ATTiny Series are:
// Tiny 13 - pin 1
// Tiny 25,45,85 - pin 2
const uint8_t OP_PIN = 1;

//#define DEBUG
#ifdef DEBUG
const uint8_t DBG_PIN = 0;
const uint8_t LATCH_PIN = 3;    

#define DBG_TOGGLE(p) TOGGLE(p)
#define DBG_SET(p)    SET(p)
#define DBG_CLR(p)    CLR(p)
#else
#define DBG_TOGGLE(p)
#define DBG_SET(p)
#define DBG_CLR(p)
#endif

// SEC packet size and typedef.
// Override the default values for this SEC by changing here
const uint8_t MY_BPP_SEC = BPP_SEC;
typedef opSecPacket_t mySecPacket_t;

// LED management parameters
const uint8_t LED_PIN = 4;
uint16_t timeLedFlash = 1000;   // LED flash time
uint32_t timeLedStart = 0;      // LED base time for period
mySecPacket_t flashCount = 0;   // LED flash count

// ---- Macros for local inline code
#define SET_INPUT(p)  { DDRB &= ~_BV(p); PORTB |= _BV(p); } // input and pullup
#define SET_OUTPUT(p) DDRB |= _BV(p)

#define TOGGLE(p) PINB |= _BV(p)
#define SET(p)    PORTB |= _BV(p)
#define CLR(p)    PORTB &= ~_BV(p)

#define IS_LOW(p)    ((PINB &= _BV(p)) == 0)
#define IS_HIGH(p)   (PINB &= _BV(p))

// Generate external interrupt level change
#define ENA_INT0   { MCUCR |= _BV(ISC00); MCUCR &= ~_BV(ISC01); GIMSK |= _BV(INT0); }
#define DIS_INT0   GIMSK &= ~_BV(INT0)
#define IS_INT0    (GIFR &= _BV(INTF0))

#ifdef DEBUG
void blipOut(uint8_t pin, uint8_t count)
{
  DBG_CLR(pin);
  for (uint8_t i = 0; i < count; i++)
  {
    delay(200);
    DBG_SET(pin);
    delay(100);
    DBG_CLR(pin);
  }
}
#endif

// ---- ISR for the change on the OnePin link pin
// The ISR is called on a change to the input.
// The falling edge is the start of a signal and the time
// to the rising edge determines the type of signal it is.
volatile bool processSignal = false;
volatile uint32_t timeSignalDuration = 0;  // duration of the signal

ISR(INT0_vect)
{
  static uint8_t ISRstate = 0;
  static uint32_t timeStart;       // signal start time in micros()

  if (!processSignal)
  {
    switch (ISRstate)
    {
    default:
      // Waiting for start of signal. 
      // This will be a transition from HIGH to LOW, so we check if 
      // the signal is LOW here before committing staing the timed wait.
      if (IS_LOW(OP_PIN))
      {
        ISRstate = 1;
        timeStart = micros();
      }
      break;

    case 1:
      // Timing the duration of the LOW signal detected earlier.
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
  ENA_INT0;
}

void loop(void)
{
  static mySecPacket_t sndData = 0;
  static opPriPacket_t rcvData = 0;
  static uint8_t bit = 0;
  bool allRcv = false;    // all bits received from PRI

  // handle the flashing LED
  if (millis() - timeLedStart >= timeLedFlash)
  {
    TOGGLE(LED_PIN);
    flashCount++;
    timeLedStart = millis();
  }

  // Process the signal based on start duration
  if (processSignal)
  {
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
      if (sndData & (1 << bit)) SET(OP_PIN); else CLR(OP_PIN);
      delayMicroseconds(OPT_RD0_SIGNAL);
      SET_INPUT(OP_PIN);
      DBG_TOGGLE(LATCH_PIN);
      bit++;
    }
    else                              // the only thing left is a Reset signal
    {
      SET_OUTPUT(OP_PIN);
      CLR(OP_PIN);
      delayMicroseconds(OPT_RST_PRESENCE);
      SET_INPUT(OP_PIN);    // with PULLUP sets this high
      DBG_TOGGLE(LATCH_PIN);
      rcvData = bit = 0;
    }

    // if we have received/sent a whole packet, deal with the data
    if (allRcv)
    {
      //blipOut(DBG2_PIN, rcvData);
      timeLedFlash = rcvData;
    }

    processSignal = false;     // reset the flag to stop blocking ISR
  }
}

