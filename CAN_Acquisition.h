/*
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#ifndef ACQ_H
#define ACQ_H
#include "variant.h"
#include <due_can.h>

//typedefs for clarity
typedef unsigned int       UINT16;
typedef signed int         SINT16;
typedef unsigned char      UINT8;
typedef signed long        SINT32;
typedef unsigned long      UINT32;
typedef signed long long   SINT64;
typedef unsigned long long UINT64;


//defines current max numer of messages allowed in rx/tx arrays
#define  MAX_NUM_TX_MSGS 20  
#define  MAX_NUM_RX_MSGS 30  

//if implementing a query-response protocol (such as OBD2), delay this long (mS) in between requests
#define  QUERY_MS 100  

/**
 *	forward declare the OBD class to the base class to support circular reference
 */
class cOBDParameter;

/**
 * This enum represents the physical port number
 */
enum ACQ_CAN_PORT
{
    CAN_PORT_0,
    CAN_PORT_1
};

/**
 * This ENUM represents the CAN baud rate values
 */
enum ACQ_BAUD_RATE
{
    _1000K = 1000,
    _800K  = 800,
    _500K  = 500,
    _250K  = 250,
    _125K  = 125,
    _50K   = 50,
	_33k   = 33,
    _25K   = 25,
    _10K   = 10,
    _5K    = 5,
    NONE   = 0
};

/**
 * This enum represents the periodic transmission rate for messages 
 */
enum ACQ_RATE_CAN
{
    _1Hz_Rate       = 1000,
    _5Hz_Rate       = 200,
    _10Hz_Rate      = 100,
    _100Hz_Rate     = 10,
    QUERY_MSG       = 0xFFFF
};

/**
 * This enum represents the position in the CAN payload where a Parameter ID or multi-message identifier page number might be
 */
enum ACQ_PID_IDX
{
    NO_PID        = 0xFF ,
    PID_IN_BYTE0  = 0x00 ,
    PID_IN_BYTE1  = 0x01 ,
    PID_IN_BYTE2  = 0x02 ,
    PID_IN_BYTE3  = 0x03 
};

/**
 * This enum represents the position in the CAN payload where a Parameter ID or multi-message identifier page number might be
 */
enum ACQ_FRAME_TYPE
{
    TRANSMIT,
    RECEIVE
};

/**
 * This enum represents mode that the CAN acquisition methond will be run: Timer or polled
 */
enum ACQ_MODE
{
    POLLING_noTx,
	POLLING,
    TIMER_2mS
};


/**
 * This sturct represents a full CAN frame. The ID, payload (message), transmission rate, and CAN ID index are represented.
 * The 8 byte payload can be represented as two U32's OR tow 4 byte arrays (upper and lower).
 * A union is used to memory map them together. The structure of the LSB and MSB of the payload is:
 *
 *  ----------Upper payload-----------  --------------Lower payload-----------
 * 
 * BYTE_0 | BYTE_1 | BYTE_2 | BYTE_3 |  BYTE_4 | BYTE_5 | BYTE_6 | BYTE_7 |
 *
 *
 * @author D.Kasamis - dan@togglebit.net
 * @version 1.0
 */
class cCANFrame
{
public:
    /**
     * CAN message ID
     */
    UINT32 ID;      

    /**
    * This struct/union combo provides multiple ways to access the payload of the message 
    * Can be referenced by byte or by U32 "upper" and "lower" payloads or by a simple byte array
    */
    union
    {
        /**
          * This array simply represents the entire payload in an 8 byte array
          */
        UINT8 b[8];

        struct
        {
            UINT32 lowerPayload;
            UINT32 upperPayload;
        }P;
    }U;

    /**
     * This is the periodic transmission rate for this message
     */
    ACQ_RATE_CAN rate;

    /**
     * This method provides for writing the payload of the CAN frame. This is required for proper byte ordering in memory.
     * 
     * @param payload U32 representing the payload, in desired order (MSB-LSB)
     */
    void setUpperU32(U32 payload);

    /**
     * This method provides for reading the payload of the CAN frame. This is required for proper byte ordering in memory.
     * 
     * @return U32 represeting payload (MSB-LSB)
     */
    U32  getUpperU32();

    /**
     * This method provides for writing the payload of the CAN frame. This is required for proper byte ordering in memory.
     * 
     * @param payload U32 representing the payload, in desired order (MSB-LSB)
     */
    void setLowerU32(U32 payload);
   
    /**
     * This method provides for writing the payload of the CAN frame. This is required for proper byte ordering in memory.
     * 
     * @return U32 represeting payload (MSB-LSB)
     */
    U32 getLowerU32();



    /**
     * This is a function that is called by the acquisition scheduler when a receive message matching this CAN ID has been received.
     * This was primarily designed for higher-layer protocols that might use a single CAN ID to transmit multiple channels/messages (OBD2).
     *
     * @param  RX_CAN_FRAME *R - pass along a pointer to RX frame  
     *
     * @return - a flag to accept or reject this CAN frame  
     */
    virtual bool CallbackRx(RX_CAN_FRAME *R)
    {
        return(true);
    };

    /**
     * This is a  function that is called by the acquisition scheduler when a message is going to be transmitted.
     * This is designed for higher-layer protocols to overload sucht that  that might use a single CAN ID to transmit multiple channels/messages (OBD2).
     *
     *
     * @return - a flag to accept or reject this CAN frame  
     */
    virtual bool CallbackTx()
    {
        return(true);
    }
};


/**
 * The acquire class is intended to act as a simple perodic acquisition scheduler for all created CAN objects. It is therefore implemented as a static base class.
 * The addMessage() method is responsible for "registering" a pair of RX & TX frames which will be polled for RX/TX during the "runRates" method
 * If written for polling, the AcquireCAN class's "run" method assumes a tight loop call (while(1)) within which it tracks elapsed time (in uSecs)
 * 
 * @author D.Kasamis - dan@togglebit.net
 * @version 1.0
 */
class cAcquireCAN 
{
public:

    /**
     * constructor definition for Acquire class
     */
    cAcquireCAN(ACQ_CAN_PORT _portNumber);

    /**
     * this is the initializtion call that sets up the hardware ports: enable mailboxes, interrupts, sets baud rates etc
     * 
     * @param baud baud rate, NONE = = no attempt is made to initialize this port
     */
    void initialize(ACQ_BAUD_RATE baud);

    /**
     * this is the master scheduler should be run in a periodic timer interrupt or a "loop(), or task() function. If run in pollng
     * you should have a tight execution in the loop to keep on schedule
     */
    void run(ACQ_MODE mode);

    /**
     * diagnostic method. Retrieves acquisition execution time in uS, diagnostics.
     * 
     * @param max - "true" specifies maximum seen value (latched), otherwise last measured value returned
     * @return - number of uSecs elapsed during "run" method 
     */
    UINT32 getTimeSlice(bool max);

    /**
     * resets "max" capture time returned by getTimeSlice. This is used for debugging
     */
    void resetTimeSlice();

    /**
     * Called to add message (pointer) to the acquisition scheduler  
     * 
     * @param frame - refrence to a CAN message that will be periodically transmitted or received
     * @param type  - determines if this frame is to be received or transmitted
     *
     */
    void addMessage(cCANFrame *frame, ACQ_FRAME_TYPE type);

    /**
      * This method transmits a single frame using the low-level driver code
      * Made public such that sending of a "one-shot" message in applicaiton code is possible.
      * @param *I  - pointer to cCANFrame object to be transmitted
      */
    void TXmsg(cCANFrame *I);

    /**
     * Get the number of messages sent by get scheduler (rolling counter value)
     * 
     * @return number of messages sent by get scheduler (rolling counter value)
     */
    UINT32 getTxCtr();

    /**
     * Get the number of messages sent by get scheduler (rolling counter value)
     * 
     * @return number of messages sent by get scheduler (rolling counter value)
     */
    UINT32 getRxCtr();

private:

    /**
     * physical port number for hw referencing
     */
    ACQ_CAN_PORT portNumber;

    /**
     * pointer to raw CAN object, physical port (from lower-level CAN library)
     */
    CANRaw *C;

    /**
     * counters used to accumulate usTicks for acquisition timing
     */
    UINT32 count, prevCount, ticks, usTicks;

    /**
    * counters used to determine # of mSecs that have elapsed
    */
    UINT16 _1mSCntr, _10mSCntr, _100mSCntr, _200mSCntr, _1000mSCntr, _queryCntr;

    /**
     * diagnostic timing varibles used to track the execution time of the scheduler
     */
    UINT32 usTsliceEnd, usTslice, usTsliceMax;

    /**
     * This is the array of message struct pointers for RX/TX. One entry is created each time an object is created. 
     * bound by #define macro "MAX_NUM_TX_MSGS"
     */
    cCANFrame *rxMsgs[MAX_NUM_RX_MSGS];
    cCANFrame *txMsgs[MAX_NUM_TX_MSGS];
    //NOTE a query message is different to a TX message in that only ONE message is sent at a time in order to allow
    //sufficient time for a node to respond before the next request is made (for query-response protocols such as OBD2)
    cCANFrame *queryMsgs[MAX_NUM_TX_MSGS];

    /**
     * counter that keeps track of the number of RX/TX messages that have been created
     */
    UINT8 msgCntRx;
    UINT8 msgCntTx;
    UINT8 msgCntQuery;

    /**
     * This is the index used for messages that are only transmitted once per iteration 
     */
    UINT8 queryIndex;

    /**
     * counters indicating the number of messages that have been transmitted 
     */
    UINT32 RxCtr;
    UINT32 TxCtr;

    /**
     * these are the masks used by the CAN controller hardware to allow multiple messages to be received by one mailbox
     * MAM mask - all bits set to "1's must match the corresponding value in the MID mask
     * MID mask - sets the "base ID" by which the MAM mask sets which bits must match for a message to be received by the mailbox 
     *(see section 	41.7.2.1 of ST datasheet)
     *
     */
    UINT32 MAM_mask;
    UINT32 MID_mask;                                                     

    /**
     * This method simply scans through collection of references array seeking the messages ready for transmission
     * 
     * @param rate    - run the readSensor() method fall all sensors of this rate
     */
    void runRates(ACQ_RATE_CAN rate);


    /**
     * This method checks for RX messages that have come in the lower level buffer and populates the appropriate cCANFrame
     * RX message (via add message method). (note: this depends upon the CAN library interrupt)
     * 
     */
    void RXmsg();

    /**
     * This method is used to build up the MAM mask needed for filtering of RX messages in hardware (see page 1211 of STM datasheet). 
     * Each new message must be bitwise compared (NOR) with previous messages to compile a common bitmask.
     * 
     * @param ID     New ID being added to the RX queue
     * @return MAM mask file per STM hardware requirements 
     */
    U32 buildMAM(U32 ID);
};   
#endif
