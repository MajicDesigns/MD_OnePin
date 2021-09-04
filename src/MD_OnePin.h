#pragma once
/**
\mainpage OnePin Lighweight Comms link

This library provides a lightweight software-only implementation 
for a single wire serial point-to-point link between two devices.

OnePin uses a single digital I/O line and ground to communicate 
between a primary and a secondary device. The primary always initiates 
and controls the communication. 

This link interface is useful to implement simple message transfers 
between a primary device and a less capable secondary device, such as 
custom sensors or actuators with simple processor control.

This implementation borrows heavily from, and simplifies, the Dallas 
Semiconductor/Maxim 1-Wire&reg; specification. Differences include:
- Links are point-to-point (one Primary and one Secondary) rather than
  bus based.
- Omitting a bus also eliminates the need for device identification 
  infrastructure.
- Timing parameters and handshaking protocol.

See the links below for more information on this signaling protocol and how it 
is implemented using this library.
- \subpage pageLinkSignals
- \subpage pageImplementation

See Also
- \subpage pageRevisionHistory
- \subpage pageDonation
- \subpage pageCopyright

\page pageDonation Support the Library
If you like and use this library please consider making a small donation
using [PayPal](https://paypal.me/MajicDesigns/4USD)

\page pageCopyright Copyright
Copyright (C) 2021 Marco Colli. All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

\page pageRevisionHistory Revision History
Sep 2021 ver 1.0.0
- Initial release
*/

#include <Arduino.h>
#include <MD_OnePin_Protocol.h>

/**
 * \file
 * \brief Main header file and class definition for the MD_OnePin library.
 */

/**
 * Core object for the MD_OnePin library
 */
class MD_OnePin
{
public:

  typedef opPriPacket_t packet_t;    ///< The Primary side comms packet. Defined as max size to fit all supported types.

  //--------------------------------------------------------------
  /** \name Class constructor and destructor.
   * @{
   */
  /**
   * Class Constructor
   *
   * Instantiate a new instance of the class.
   *
   * The main function for the core object is to set the internal
   * shared variables to default or passed values.
   *
   * \param pin pin used for OnePin comms.
   * \param bppPri the number of bits per packet (bpp) sent by PRI
   * \param bppSec the number of bits per packet (bpp) received from SEC
   */
    MD_OnePin(uint8_t pin, uint8_t bppPri = BPP_PRI, uint8_t bppSec = BPP_SEC) :
        _pin(pin), _presence(false), _bppPri(bppPri), _bppSec(bppSec)
        {};
  
   /**
    * Class Destructor.
    *
    * Release any allocated memory and clean up anything else.
    */
     ~MD_OnePin() {};

     /** @} */

  //--------------------------------------------------------------
  /** \name Methods for core object control.
   * @{
   */
  /**
   * Initialize the object.
   *
   * Initialize the object data. This needs to be called during setup()
   * to set items that cannot be done during object creation.
   */
    void begin(void);

  /**
   * Write a PRI data packet.
   *
   * The PRI initiates a Reset/Presence signal with SEC and then sequentially
   * writes (Write 0/1 Signals) as many bits as are configured for the PRI packet
   * size.
   *
   * \sa MD_OnePin(), \ref pageLinkSignals, \ref pageImplementation
   *
   * \param data    the data to sent to the SEC. Only the configured number of bits will be transmitted.
   * \param noReset set true to omit the Comms Reset signal before sending data. Defaults to false (ie, reset).
   * \return true if the SEC device was present.
   */
    bool write(packet_t data, bool noReset = false);

  /**
   * Request a SEC data packet.
   *
   * The PRI initiates a Reset/Presence signal with SEC and then sequentially 
   * requests (Read Signal) as many bits as are configured for the SEC packet 
   * size.
   *
   * \sa MD_OnePin(), \ref pageLinkSignals, \ref pageImplementation
   *
   * \param noReset set true to omit the Comms Reset signal before reading data. Defaults to false (ie, reset).
   * \return Data packet received from the SEC. Only the configured number of bits will be valid.
   */
    packet_t read(bool noReset = false);

  /**
   * Secondary device presence status.
   *
   * Every time a comms packet is sent to the secondary device the library
   * determines whether the secondary device is 'present'. This method returns
   * the status of the last write().
   * 
   * Note the status is never re-evaluated until another call to write(), so
   * this method should not be used to wait for a device to 'come online'.
   *
   * \sa read(), write(), \ref pageLinkSignals
   * 
   * \return true if the SEC was present at the last attempted comms.
   */
    inline bool isPresent(void) { return(_presence); }

  /** @} */

private:
  uint8_t _pin;      ///< comms pin number
  bool    _presence; ///< result of the last presence check (true if present)
  uint8_t _bppPri;   ///< number of bits per packet for primary send
  uint8_t _bppSec;   ///< number of bits per pack for primary receive (secondary send)

  uint16_t _switchTime; ///< average microseconds for a SET_TO_* operation; calculate in begin()
  uint16_t _writeTime;  ///< average microseconds for a digitalWrite() operation; calculate in begin()

  bool resetComm(void); ///< Send a reset command and detect a presence response
                        ///< Returns false if there is no device present
};
