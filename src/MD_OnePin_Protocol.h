#pragma once
/**
\page pageLinkSignals Link Signaling Protocol
## Overview
OnePin is a communications link is between a Primary device (PRI) and a Secondary 
device (SEC). The PRI is assumed to have more computational power be more capable 
than the SEC, which is envisaged as a small embedded processor providing local 
control for a sensor or actuator.

OnePin operates using just one I/O pin and electrical ground per device pair, 
switching the direction of signal flow according to a timer based protocol 
implemented in both the PRI and SEC. The electrical connections is dependent 
on the hardware implementation, which primarily needs to ensure that the digital 
HIGH and LOW signal voltages are are compatible between devices.

Electrically, the link is idle HIGH and communications is effected by pulling 
the link LOW. When the I/O pins are set to input mode in the PRI or SEC they 
are be pulled HIGH using the processor's internal pullup resistors (INPUT_PULLUP).
However, a external pullup is also possible and compatible withe the oprations of 
the OnePin library.

The communications link timing is divided into fixed time slots (T) and all 
protocol timing is expressed as as multiples of T. There is no system clock - 
communications are synchronized to the falling edge of PRI. This means that 
PRI timing should be more accurate than SEC timing, which only responds to PRI.

PRI initiates every communication between the devices, down to bit level 
transfers for both directions. OnePin is also highly tolerant of SEC device 
failure, reestablishing communications as soon as both devices are available - 
when the PRI restarts communications SEC will respond and signal its presence.

Each protocol data transaction occurs in packet, with transfers occurring as 
sequential bits starting with the LSB (2<sup>0</sup>). As all activity occurs 
on the same wire, communications are necessarily half duplex.

## Link Transactions

Data is transmitted in packets up to 32 bits in length. The contents of these 
data packets is arbitrary determined by the application.

There are 5 basic signals used for communications (Write0, Write1, Read,
Reset/Presence) described below. The timing (length) of the first part of the 
signal (header or preamble) identifies the type of signal to follow.

Transmission of a packet begins with PRI initiating a Reset Signal, then:
- To write data to SEC, PRI sends a packet using the Write Signal for each bit.
- To read data from SEC, PRI initiates a Read Signal for each bit until a packet 
has been received.

### Write 1 Signal
-# PRI pulls the low LOW for 0.5T and then HIGH for the rest of the time slot.
-# On the rising edge SEC determines a 1 by the timing value.

![Write 1 Timing Diagram] (Write1.png "Write 1 Timing Diagram")

### Write 0 Signal
-# PRI pulls the link LOW for 1.5T and then HIGH for the remainder of the time slot.
-# On the rising edge SEC determines a 0 by the timing value.

![Write 0 Timing Diagram] (Write0.png "Write 0 Timing Diagram")

### Read Signal
-# PRI pulls the link LOW for 2.5T and then HIGH for the remainder of the time slot.
-# PRI reads the link 0.5T after the end of the time slot and waits T before further
signaling if it detects a presence.
-# On the PRI rising edge, SEC sets link to HIGH/LOW for 1/0 bit value for T time. 

![Read Timing Diagram] (Read.png "Read Timing Diagram")

### Reset/Presence Signal
Note: The reset signal is the start of every transaction. 
-# PRI sets the link LOW for 5T and then sets it HIGH.
-# PRI reads the link T after setting the rising edge to detect the presence of SEC
-# PRI then waits 1.5T before starting further signaling if SEC was detected.
-# On the PRI rising edge, SEC sets the link LOW for 1.5T to signal its presence.

![Reset/Presence Timing Diagram] (Reset_Presence.png "Reset/Presence Timing Diagram")
*/

/**
 * \file
 * \brief Header for MD_OnePin library link protocol timing parameters.
 */

/**
\def BPP_PRI
The size in bits of the link packet from PRI to SEC.
The bits per packet (bpp) can be between 1 and 32 bits.

\sa \ref BPP_SEC
*/
#ifndef BPP_PRI
#define BPP_PRI 32
#endif

/**
\def BPP_SEC
The size in bits of the link packet from SEC to PRI.
The bits per packet (bpp) can be between 1 and 32 bits.

\sa \ref BPP_PRI
*/
#ifndef BPP_SEC
#define BPP_SEC 8 
#endif

/**
Defines the packet type for PRI node. As PRI is much less 
resource constrained than SEC, we just define the largest 
data type only that will hold all packet sizes (defined as 
BPP_PRI bits).
*/
typedef uint32_t opPriPacket_t;

/**
Defines packet typedef for the SEC node. As SEC is probably resource 
constrained the definition will generate the smallest size that fits
BPP_SEC number of bits.
*/
#if (BPP_SEC > 0 && BPP_SEC <= 8)
typedef uint8_t opSecPacket_t;
#elif (BPP_SEC <= 16)
typedef uint16_t opSecPacket_t;
#elif (BPP_SEC <= 32)
typedef uint32_t opSecPacket_t;
#else
#error opSecPacket_t size not properly defined
#endif

// One Wire timing values in microseconds
const uint16_t OPT = 80;     ///< OnePin Timesleot in microseconds - all timing is multiples/fractions of this

//-- Reset
const uint16_t OPT_RST_SIGNAL = 5 * OPT;                  ///< Reset the device for a new command
const uint16_t OPT_RST_PRESENCE = (3 * OPT) / 2;          ///< Duration of SEC presence signal
const uint16_t OPT_RST_PRS_SAMPLE = OPT;                  ///< PRI presence sampling time after setting rising edge
const uint16_t OPT_RST_END = (3 * OPT) / 2;               ///< Delay presence sampling before the next comms

//-- Write 1
const uint16_t OPT_WR1_SIGNAL = OPT / 2;                  ///< Write a 1 line active time (low signal)
const uint16_t OPT_WR1_PAUSE = OPT - OPT_WR1_SIGNAL;      ///< Write a 1 line delay time (high signal after active)
const uint16_t OPT_WR1_DETECT = OPT;                      ///< Write a 1 SEC read detection threshold

//-- Write 0
const uint16_t OPT_WR0_SIGNAL = (3 * OPT) / 2;            ///< Write a 0 line active time (low signal)
const uint16_t OPT_WR0_DETECT = 2 * OPT;                  ///< Write a 0 SEC read detection threshold
const uint16_t OPT_WR0_PAUSE = (2 * OPT) - OPT_WR0_SIGNAL;///< Write a 0 line delay time (high signal after active)

//-- Read
const uint16_t OPT_RD_INIT = (5 * OPT) / 2;               ///< Read activation signal
const uint16_t OPT_RD_DETECT = 3 * OPT;                   ///< Read bit SEC detection threshold
const uint16_t OPT_RD0_SIGNAL = OPT;                      ///< SEC hold time to signal a 0
const uint16_t OPT_RD_SAMPLE = OPT_RD0_SIGNAL / 2;        ///< PRI read signal sampling time after read signal
const uint16_t OPT_RD_PAUSE = OPT;                        ///< PRI read pause before next read

