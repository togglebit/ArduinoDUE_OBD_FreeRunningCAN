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
#include "CAN_Acquisition.h" 
/**
 * Constructor definition for Acquisition class, initialize variables
 * 
 * @param _portNumber - This is the physical port number that this object belongs to
 */
cAcquireCAN::cAcquireCAN(ACQ_CAN_PORT _portNumber)
{

	//initialize variables
	ticks        = 0;
	usTicks      = 0;
	prevCount    = 0;
	count        = 0;
	_1mSCntr     = 0;
	_10mSCntr    = 0;
	_100mSCntr   = 0;
	_1000mSCntr  = 0;
	usTsliceMax  = 0;
	usTslice     = 0;
	msgCntRx     = 0;
	msgCntTx     = 0;
	msgCntQuery  = 0;
	queryIndex   = 0;
	RxCtr        = 0;
	TxCtr        = 0;

	//set pointer reference to proper object for that physical port
	portNumber = _portNumber;
	C = (portNumber == CAN_PORT_0) ? &CAN : &CAN2;
}
/**
 * This is the initializtion call that sets up the hardware ports: enable mailboxes, interrupts, sets baud rates etc. 
 * This can be called by applicaiton code directly, should be called last after messages have been added to the scheduler 
 * i.e. right at the end of the setup(), this will allow for the masks to be set properly.
 * 
 * @param -  baud rate for CAN0, NONE = no attempt is made to initialize this port
 */
void cAcquireCAN::initialize(ACQ_BAUD_RATE baud)
{
	if (portNumber == CAN_PORT_0)
	{
		//setup port0 hardware
		if (CAN.init(baud*1000))
		{
			// Disable all CAN0 & CAN1 interrupts
			CAN.disable_interrupt(CAN_DISABLE_ALL_INTERRUPT_MASK);
			//DEBUG THIS TO FIGURE OUT PROPER IRQ
			NVIC_EnableIRQ(CAN0_IRQn);

			//reset mailboxes
			CAN.reset_all_mailbox();

            //setup the masks for the recieve mailbox
            CAN.mailbox_set_accept_mask(0, MAM_mask, MID_mask > 0x7FF ? true : false);
            CAN.mailbox_set_id         (0, MID_mask, MID_mask > 0x7FF ? true : false);



			//setup the receive mailbox for CAN 0 to mailbox 0
			CAN.mailbox_set_mode(0, CAN_MB_RX_MODE); 

			//setup the transmit mailbox for CAN 0 to mailbox 1
			CAN.mailbox_set_mode(1, CAN_MB_TX_MODE);
			CAN.mailbox_set_priority(1, 15);
			CAN.mailbox_set_datalen(1, 8);



			//enable RX interrupt for mailbox0
			CAN.enable_interrupt(CAN_IER_MB0);
		}
	}


	if (portNumber == CAN_PORT_1)
	{
		//set physical can1 port reference
		if (CAN2.init(baud*1000))
		{
			// Disable all CAN0 & CAN1 interrupts
			CAN2.disable_interrupt(CAN_DISABLE_ALL_INTERRUPT_MASK);
			//DEBUG THIS TO FIGURE OUT PROPER IRQ
			NVIC_EnableIRQ(CAN1_IRQn);

			//reset mailboxes
			CAN2.reset_all_mailbox();
            
            //setup the masks for the recieve mailbox
            CAN2.mailbox_set_accept_mask(0, MAM_mask, MID_mask > 0x7FF ? true : false);
            CAN2.mailbox_set_id         (0, MID_mask, MID_mask > 0x7FF ? true : false);

			//setup the receive mailbox for CAN 1 to mailbox 2
			CAN2.mailbox_set_mode(0, CAN_MB_RX_MODE); 

			//setup the transmit mailbox for CAN 1 to mailbox 2
			CAN2.mailbox_set_mode(1, CAN_MB_TX_MODE);
			CAN2.mailbox_set_priority(1, 15);
			CAN2.mailbox_set_datalen(1, 8);

			//enable RX interrupt for mailbox1
			CAN2.enable_interrupt(CAN_IER_MB0);
		}
	}
}

/**
 * This method adds message reference to the collection of rx/tx references,
 * increments counter. Bound by "MAX_NUM_TX_MSGS" macro.
 * 
 * @param frame  pointer reference to a message (object) that is intended for reception or transmission
 * @param type   identifies if this message is to be received or transmitted
 */
void cAcquireCAN::addMessage(cCANFrame *frame, ACQ_FRAME_TYPE type)
{
	if (type == TRANSMIT)
	{
		if (frame->rate == QUERY_MSG)
		{
			//add this to the pointer queue for query messages
			queryMsgs[msgCntQuery] = frame;
			msgCntQuery = (msgCntQuery < MAX_NUM_TX_MSGS)? msgCntQuery + 1 : MAX_NUM_TX_MSGS - 1;

		} else
		{
			//add message into collection of pointers to free-running CAN messages, bounds check
			txMsgs[msgCntTx] = frame;       
			msgCntTx = (msgCntTx < MAX_NUM_TX_MSGS)? msgCntTx + 1 : MAX_NUM_TX_MSGS - 1;
		}
	}

	if (type == RECEIVE)
	{
		//add messge into collection of pointers, bounds check
		rxMsgs[msgCntRx] = frame;       
		msgCntRx = (msgCntRx < MAX_NUM_RX_MSGS)? msgCntRx + 1 : MAX_NUM_RX_MSGS - 1; 

		//we need to setup the masks for the ID's in the receive mailbox
		//receive mailbox is mailbox #0
		MAM_mask = buildMAM(frame->ID);
		MID_mask |= frame->ID;
	}

}

/**
 * This method is used to build up the MAM mask needed for filtering of RX messages in hardware (see page 1211 of STM datasheet). 
 * Each new message must be bitwise compared (NOR) with previous messages to compile a common bitmask.
 * 
 * @param ID     New ID being added to the RX queue
 * @return MAM mask file per STM hardware requirements 
 */
U32 cAcquireCAN::buildMAM(U32 ID)
{
	U32 XOR_mask;
	U32 retVal;
	U8  i;

	for (i=0; i < msgCntRx; i++)
	{
		//XOR new ID with all messages, all bits that do not match are set to 1 in XOR mask
		XOR_mask |= rxMsgs[i]->ID^ID; 
	}
	//per page 1211 of datasheet, invert all non-matching bits to '0'
	retVal = ~XOR_mask;

	return(retVal);
}

/**
 * This method simply scans through collection of references array seeking the messages ready for transmission/update
 * 
 * @param rate - transmit all messages scheduled at this rate
 */
void cAcquireCAN::runRates(ACQ_RATE_CAN rate)
{
	UINT8 i ;

	if (rate == QUERY_MSG)
	{
		//check to see if we have any query message entries
		if (msgCntQuery)
		{
			//transmit the next message in the "query-response" queue,
			//only send a single message to allow time for the node to respond before making another request
			TXmsg(queryMsgs[queryIndex]);
			queryIndex = (queryIndex == (msgCntQuery - 1)) ?  0 : queryIndex + 1;
		}

	} else
	{
		//scan through message list and transmit all free-running messages scheduled at this rate
		for (i=0; i < msgCntTx; i++)
		{
			//look for a valid pointer entry for this rate        
			if (txMsgs[i]->rate == rate)
			{
				TXmsg(txMsgs[i]);
			}
		}   
	}
}


/**
 * This method transmits a single frame using the low-level driver code
 * 
 * @param *I  - pointer to cCANFrame object to be transmitted
 */
void cAcquireCAN::TXmsg(cCANFrame *I)
{
	UINT32 mbStatus, status;
	bool validFrame = false;

	// transmit a message here, set up CAN hardware
	//wait until our mailbox is ready to accept a new message and we are not in a bus error state
	do
	{
		mbStatus = C->mailbox_get_status(1);
		status = C->get_status();

	}while (!(mbStatus & CAN_MSR_MRDY) && !(status & CAN_SR_ERRP) && !(status & CAN_SR_BOFF));

	//if a higher level protocol is used, fire callback to handle any modificaiton of the message or abort message
	validFrame = I->CallbackTx();

	//if no abort, stuff the frame and payload 
	if (validFrame)
	{
        
		//set CAN ID  for mailbox,check for extended ID
		C->mailbox_set_id(1, I->ID, I->ID > 0x7FF ? true : false);

		//load payloads	for this mailbox 
		C->mailbox_set_datal(1,I->U.P.lowerPayload);
		C->mailbox_set_datah(1, I->U.P.upperPayload);

		// send this mailbox
		C->global_send_transfer_cmd(CAN_TCR_MB1);

		//increment transmit counter
		TxCtr += 1; 
	}
}

/**
 * This method checks for RX messages that have come into the lower-level buffer
 * and populates the appropriate RX message ID's accordingly (via add message method).
 *   
 */
void cAcquireCAN::RXmsg()
{
	UINT8 i;
	bool validFrame = false;

	//temporary frame we'll use to figure out which CAN ID has been received and where to move data to
	RX_CAN_FRAME newFrame;

	//based upon the lower-level CAN library the get_rx_buff method appears to be critical 
	noInterrupts();

	//pull all data frames out of the buffer 
	while (C->read(newFrame))
	{
		//scan through message list and read the corresponding header ID
		for (i=0; i < msgCntRx; i++)
		{
			//look for a valid entry for the CAN ID we just received        
			if (rxMsgs[i]->ID == newFrame.id)
			{
                
				//fire callback to higher-level protocol (e.g. check PID parameter ID)
				validFrame = rxMsgs[i]->CallbackRx(&newFrame);                
                                
				//stuff the received  payload 
				if (validFrame)
				{
					///either NO PID OR PID's match so stuff it
					rxMsgs[i]->U.b[0] = newFrame.data.byte[0];
					rxMsgs[i]->U.b[1] = newFrame.data.byte[1];
					rxMsgs[i]->U.b[2] = newFrame.data.byte[2];
					rxMsgs[i]->U.b[3] = newFrame.data.byte[3];
					rxMsgs[i]->U.b[4] = newFrame.data.byte[4];
					rxMsgs[i]->U.b[5] = newFrame.data.byte[5];
					rxMsgs[i]->U.b[6] = newFrame.data.byte[6];
					rxMsgs[i]->U.b[7] = newFrame.data.byte[7];
					//NOTE: re-write in future versions for U32 transfers

					//increment receive counter
					RxCtr += 1;
				}
			}
		}
	}
	interrupts();
}

/**
 * This is the scheduler routine that should be run at a periodic rate to keep up with specified transmission rates and to pull received messages 
 * from the lower level driver buffers.If run in "loop() / while() / task()" ,it assumes tight execution to keep on schedule. 
 * This method keeps track of the number of uSeconds elapsed and calls the "runRates" method for a given rate, allowing for the entire 
 * list of CAN messages to be transmitted or received (runRates) at it's scheduled rate. This method should be called from a tightly executed 
 * loop in polling mode OR for more deterministic operation from a XmS timer interrupt. This method first looks to read out any received 
 * messages and then does the transmission. The rate at which messages are received and updated is equal to that of which this method is 
 * called (1mS timer interrupt = messages pulled from low-level RX buffer and updated at ~1mS). In polling mode, this method uses the math 
 * that refrences the DUE micros() function to execute at a approx 1mS rate.
 * 
 * @param mode   - how this method is called, from a polling mode or periodic timer interrupt
 */
void cAcquireCAN::run(ACQ_MODE mode)
{
	//sample clock to determine elapsed number of microseconds
	count = micros();

	//this is the method that looks for message receptions. 
	//As such, RX packet data will only be updated as often as this is called (CAN reception is updated at the interrupt level)
	RXmsg();         

	if (mode == POLLING)
	{
		//compensate for rollover
		ticks = (prevCount < count) ? (count - prevCount) : (0xFFFFFFFF - prevCount) + count; 

		//capture new previous count
		prevCount = count;

		//accumulate uS ticks
		usTicks += ticks;
		//detect if 1mS passed? clear uSec count if we've determined 1ms has passed
		usTicks = (usTicks >= 1000) ? 0 : usTicks;
	}

	//The followoing transmission rate tasks are driven by a XmS raster period. (timer or tight polling loop)
	if (mode == TIMER_2mS || ((mode == POLLING) && !usTicks))
	{

		 //increment 1mS tick counter
		_1mSCntr = (mode == TIMER_2mS) ? _1mSCntr + 2 : _1mSCntr + 1;

		//after Xms has passed, run all xHz transmissions based upon XmS tick counter		

		//transmit the next message in the "query-response" queue
		if ((_1mSCntr - _queryCntr) >= QUERY_MS)
		{
			//periodic requests
			runRates(QUERY_MSG);
			_queryCntr = _1mSCntr;
		}

		//transmit the "free-running" CAN messages
		//100Hz
		if ((_1mSCntr - _10mSCntr) >= 10)
		{
			runRates(_100Hz_Rate);
			_10mSCntr = _1mSCntr;
		}

		//10Hz
		if ((_1mSCntr - _100mSCntr) >= 100)
		{
			runRates(_10Hz_Rate);
			_100mSCntr = _1mSCntr;
		}

		//5Hz
		if ((_1mSCntr - _200mSCntr) >=  200)
		{
			runRates(_5Hz_Rate);
			_200mSCntr = _1mSCntr;
		}

		//1Hz
		if (_1mSCntr >= 1000)
		{
			runRates(_1Hz_Rate);
			_queryCntr  = 0;
			_1mSCntr    = 0;
			_10mSCntr   = 0;
			_100mSCntr  = 0;
			_200mSCntr  = 0;
		}

		//perform diagnostic timer, provides service routine timing in uSec
		usTsliceEnd = micros();
		usTslice = (count < usTsliceEnd) ? usTsliceEnd - count : (0xFFFFFFFF - count) + usTsliceEnd; 

		//latch maximum value
		usTsliceMax = usTslice > usTsliceMax ? usTslice : usTsliceMax;
	}
}

/**
* diagnostic method. Retrieves acquisition execution time in uS, diagnostics.
* 
* @param max - "true" specifies maximum seen value (latched), otherwise last measured value returned
* @return - number of uSecs elapsed during "run" method 
*/
UINT32 cAcquireCAN::getTimeSlice(bool max)
{
	return( max ? usTsliceMax : usTslice );
}

/**
 * resets "max" capture time returned by getTimeSlice. This is used for debugging
 */
void cAcquireCAN::resetTimeSlice()
{
	usTsliceMax = 0;
}

/**
 * Get the total number of messages that have been transmitted (rolling)
 * 
 * @return - U32 rolling counter of number of messages transmitted 
 */
UINT32 cAcquireCAN::getTxCtr()
{
	return(TxCtr);
}

/**
 * Get the total number of messages that have been received (rolling)
 * 
 * @return - U32 rolling counter of number of messages received 
 */
UINT32 cAcquireCAN::getRxCtr()
{
	return(RxCtr);
}

/**
 * This method provides for writing the payload of the CAN frame. This is required for proper byte ordering in memory.
 * 
 * @param payload U32 representing the payload, in desired order (MSB-LSB)
 */
void cCANFrame::setUpperU32(U32 payload)
{
    
	//swap byte ordering
	U.b[0] = (payload >> 24) & 0xFF;
	U.b[1] = (payload >> 16) & 0xFF;
	U.b[2] = (payload >> 8)  & 0xFF;
	U.b[3] = payload & 0xFF;
}

/**
 * This method provides for writing the payload of the CAN frame. This is required for proper byte ordering in memory.
 * 
 * @param payload U32 representing the payload, in desired order (MSB-LSB)
 */
void cCANFrame::setLowerU32(U32 payload)
{
	//swap byte ordering
	U.b[4] = (payload >> 24) & 0xFF;
	U.b[5] = (payload >> 16) & 0xFF;
	U.b[6] = (payload >> 8)  & 0xFF;
	U.b[7] = payload & 0xFF;
}

/**
 * This method provides for reading the payload of the CAN frame. This is required for proper byte ordering in memory.
 * 
 * @return U32 represeting payload (MSB-LSB)
 */
U32  cCANFrame::getUpperU32()
{
	return((U32)(U.b[0] << 24) | (U.b[1] << 16) | (U.b[2] << 8) | U.b[3]);   
}


/**
 * This method provides for writing the payload of the CAN frame. This is required for proper byte ordering in memory.
 * 
 * @return U32 represeting payload (MSB-LSB)
 */
U32  cCANFrame::getLowerU32()
{
	return((U32)(U.b[4] << 24) | (U.b[5] << 16) | (U.b[6] << 8) | U.b[7] );   
}





