#include <MD_OnePin.h>
#include <MD_OnePin_Protocol.h>

/**
\page pageImplementation Library Implementation Overview
## Overview
The library is implemented as a class MD_OnePin to implement the PRI 
side of the link. This has primitives to read, write and reset the link 
interface (basically all the signals), and is implemented just using 
documented Arduino library calls (ie, no assembler, timer or other hardware 
specific features). This part is designed to be portable across architectures.

The SEC side is implemented as 'C' code that responds to the PRI signalling.
The library's example SEC implementations detect and time the PRI initiating 
signal in and Interrupt Service Routine (ISR), setting a flag for the main 
loop() to do most of the processing. The majority of the SEC can be written
using portable functions, but it is possible that for some applications a 
more low level approach may be required to implement the same, or similar, 
logic. In other words, the SEC node is application dependent.

The example SEC use an ISR tied to an external interrupt connected to the PRI 
digital I/O. However, the same could work off other suitable interrupt types, 
such as pin change interrupts.

The main loop() uses the flag set in the ISR then uses this information to
implement the remaining protocol response and reconstruct data packets. In
this way the majority of the processing is implemented outside the ISR.

As the two ends of the link both read and write to the link, PRI and SEC 
orchestrate changing the I/O pin between INPUT and OUTPUT. These changes 
are described in the section below.

## Tailoring implementations
The packet size from PRI to SEC and SEC to PRI can be different sizes,
between 1 and 32 bits. PRI to SEC is expected to be larger as this flow
may include configuration parameters, command codes, queries, etc. SEC
to PRI is likely to consist of status and other 'lightweight' information.
In any event, the bits per packet for both flows are defined in the
MD_OnePin_Protocol.h header file, the class constructor for MD_OnePin() and
the SEC code implementation.

At microsecond time intervals, higher level libraries introduce noticeable
time delays when switching I/O pins between INPUT/OUTPUT and reading/writing 
writing to digital outputs. The PRI implementation measures an average 
value for this in begin() and adjusts timing parameters/delays to take 
these into account.

Another source of potential issues are the timing gap between packet detection
in SEC and the processing of the packet in loop(). For short T slots it is 
possibler that PRI has moved on through the signal becore SEC has started its 
processing. This is especially likely when there is a considerable computing 
disparity between PRI and SEC (for example, a 16Mhz clock compared to 8MHz). 

All protocol message timing is defined in the MD_OnePin_Protocol.h header file. 
The base time slot T is defined as OPT and all other timing parameters are 
derived from this. Changing this single constant value is the correct way to 
fine-tune timing related communications issues.

## Message Processing
This section descibes the detailed implementation and coordination between 
PRI and SEC implementation of the OnePin protocol.

### Reset/Presence Signal
-# The PRI pulls the signal low for OPT_RST_SIGNAL time.
-# At end of signal time, PRI switches the I/O pin to a digital input.
-# The SEC detects the reset signal and switches to digital output.
-# The SEC signals its presence by switching low and holding for OPT_RST_PRESENCE time.
-# The PRI samples the signal at OPT_RST_PRS_SAMPLE time for the slave signal.
-# After sampling presence signal PRI switches I/O to digital output.
-# At end of signal period SEC switches I/O pin to digital input.
-# PRI delays OPT_RST_END time before sending the next part of communications.

### Write Signal
-# PRI resets the link and detects presence of device (see above).
-# PRI pulls the signal LOW for OPT_WRIx_SIGNAL (x=0,1) time.
-# PRI pulls the signal HIGH and waits for the remainder of the time slot (OPT_WRIx_PAUSE).

### Read Signal
-# PRI resets the link and detects presence of device (see above).
-# PRI pulls the signal LOW for OPT_RD_INIT time.
-# At end of signal time, PRI switches the I/O pin to a digital input.
-# The SEC detects the read signal and switches to digital output.
-# The SEC signals a 1/0 by holding signal HIGH/LOW for OPT_RD_SIGNAL0 time.
-# At end of signal period SEC switches I/O pin to digital input.
-# The PRI samples the input at OPT_RD_SAMPLE time to detect line status.
-# After sampling slave write, PRI switches I/O to digital output and delays for OPT_RD_PAUSE time.
 */

/**
 * \file
 * \brief Code file for MD_OnePin library class (Comms protocol timing).
 */

#ifndef OP_DEBUG
#define OP_DEBUG 0      ///< 1 turns library debug output on
#endif
#ifndef OP_DEBUG_DIGITAL
#define OP_DEBUG_DIGITAL 0      ///< 1 turns digital I/O debug output on
#endif

#if OP_DEBUG
#define OPPRINT(s,v)   do { Serial.print(F(s)); Serial.print(v); } while (false)
#define OPPRINTX(s,v)  do { Serial.print(F(s)); Serial.print(F("0x")); Serial.print(v, HEX); } while (false)
#define OPPRINTB(s,v)  do { Serial.print(F(s)); Serial.print(F("0b")); Serial.print(v, BIN); } while (false)
#else
#define OPPRINT(s,v)
#define OPPRINTX(s,v)
#define OPPRINTB(s,v)
#endif

#if OP_DEBUG_DIGITAL
#define DBG_PIN 4
#define DBG_FLIP digitalWrite(DBG_PIN, digitalRead(DBG_PIN) ? LOW : HIGH)
#else
#define DBG_FLIP
#endif

// Define some macros
// These are defined as macros to make the code inline and avoid a functional
// call overhead in time critical sections
#define SET_TO_INPUT  pinMode(_pin, INPUT_PULLUP)
#define SET_TO_OUTPUT pinMode(_pin, OUTPUT)
#define OP_SIGNAL(active, pause) \
  do { \
    digitalWrite(_pin, LOW);  \
    delayMicroseconds(active - _writeTime); \
    digitalWrite(_pin, HIGH); \
    if (pause != 0) delayMicroseconds(pause - _writeTime); \
  } while (false)

void MD_OnePin::begin(void)
{
  uint32_t start = micros();

  OPPRINT("\nBPPri:", _bppPri);
  OPPRINT(" BPPSec:", _bppSec);

  // work out average time for a SET_TO_* and digitalWrite() 
  // micros() is not especially accurate for short times, so accumulate a number of
  // SET_TO_* operations and average out the larger number.
  // Use a power of 2 count so we can easily divide the delta time (below).
  for (uint8_t i = 0; i < 64; i++)
  {
    // Note we are switching 2x in this loop!
    SET_TO_OUTPUT;
    SET_TO_INPUT;
  }
  _switchTime = (micros() - start) >> 7; // divide by 2 * (loop count's power of 2) 
  OPPRINT("\nAvg switch us: ", _switchTime);

  SET_TO_OUTPUT;

  start = micros();
  for (uint8_t i = 0; i < 128; i++)
  {
    // Note we are switching 2x in this loop!
    digitalWrite(_pin, HIGH);
    digitalWrite(_pin, LOW);
  }
  _writeTime = (micros() - start) >> 8; // divide by 2 * (loop count's power of 2)
  OPPRINT("\nAvg write us: ", _writeTime);

  // set up the actual initial config for the comms pin
  digitalWrite(_pin, HIGH);

  // if debugging, initialize the debug output pin
#if OP_DEBUG_DIGITAL
  pinMode(DBG_PIN, OUTPUT);
  digitalWrite(DBG_PIN, LOW);
#endif
}

bool MD_OnePin::write(packet_t data, bool noReset)
{
  if (!noReset) resetComm();

  if (noReset || _presence)
  {
    // send out each bit in turn, LSB first
    for (uint8_t i = 0; i < _bppPri; i++, data >>= 1)
    {
      if (data & 1) OP_SIGNAL(OPT_WR1_SIGNAL, OPT_WR1_PAUSE);
      else          OP_SIGNAL(OPT_WR0_SIGNAL, OPT_WR0_PAUSE);
      DBG_FLIP;   // bit received/sampled
    }
  }

  return(_presence);
}

MD_OnePin::packet_t MD_OnePin::read(bool noReset)
{
  uint32_t packet = 0;
  packet_t mask = 1;

  if (!noReset) resetComm();

  if (!noReset && !_presence) return (0xffffffff);

  // receive each bit in turn, LSB first
  for (uint8_t i = 0; i < _bppSec; i++, mask <<= 1)
  {
    OP_SIGNAL(OPT_RD_INIT, 0);
    SET_TO_INPUT;
    delayMicroseconds(OPT_RD_SAMPLE - _switchTime);
    if (digitalRead(_pin) == HIGH) packet |= mask;
    SET_TO_OUTPUT;
    DBG_FLIP;   // bit received/sampled
    delayMicroseconds(OPT_RD_PAUSE - _switchTime);
  }

  return(packet);
}

bool MD_OnePin::resetComm(void)
{
  SET_TO_OUTPUT;
  OP_SIGNAL(OPT_RST_SIGNAL, 0);
  SET_TO_INPUT;
  delayMicroseconds(OPT_RST_PRS_SAMPLE - _switchTime);
  DBG_FLIP; // sampling point
  _presence = (digitalRead(_pin) == LOW);
  SET_TO_OUTPUT;
  delayMicroseconds(OPT_RST_END - _switchTime);
  DBG_FLIP; // end of signal

  return(_presence);
}
