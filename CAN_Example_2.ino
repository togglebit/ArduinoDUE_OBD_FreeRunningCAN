#include <OBD2.h>
#include <DueTimer.h>
/********************************************************************
This example is built upon the "CANAcquisition" class cAcquireCAN 
which is a simple scheduler for periodic TX/RX of CAN messages. 

This example shows how to set up a "free-running" CAN bus where you have:
	- Two messages being transmitted at 5Hz and 10Hz 
	- One message being received at 1Hz
/********************************************************************/

//create the CANport acqisition schedulers
cAcquireCAN CANport0(CAN_PORT_0);


/***** DEFINITIONS FOR RAW CAN FRAME *****/      
cCANFrame  RAW_CAN_Frame1;
cCANFrame  RAW_CAN_Frame2;
cCANFrame  RAW_CAN_Frame3;

void setup()
{
	//start serial port at 9600 bps:
	Serial.begin(115200);

	//debugging message for monitor to indicate CPU resets are occuring
	Serial.println("System Reset");

	//start CAN ports, set the baud rate here
	CANport0.initialize(_500K);

	//initialize the items needed to TX/RX raw CAN mesasges
	RAW_CAN_Frame1.ID = 0x100;
	RAW_CAN_Frame1.rate  = _5Hz_Rate;
  
	RAW_CAN_Frame2.ID = 0x200;
	RAW_CAN_Frame2.rate  = _10Hz_Rate;
     
	RAW_CAN_Frame3.ID = 0x400;
	RAW_CAN_Frame3.rate  = _1Hz_Rate;

	//add our raw messages to the scheduler	for CAN port 1
	CANport0.addMessage(&RAW_CAN_Frame1, TRANSMIT);
	CANport0.addMessage(&RAW_CAN_Frame2, TRANSMIT);
	CANport0.addMessage(&RAW_CAN_Frame3, RECEIVE );

	//set up the transmission/reception of messages to occur at 500Hz (2mS) timer interrupt
	Timer3.attachInterrupt(CAN_RxTx).setFrequency(500).start();

	//output pin that can be used for debugging purposes
	pinMode(13, OUTPUT);      
}


UINT8 i;
UINT32 maxTime;

void loop()
{
	// this single byte should continously change in our raw CAN transmissions
	RAW_CAN_Frame1.U.b[0] = i;
	RAW_CAN_Frame2.U.b[7] = i;
	i = i++;

	//print byte from the CAN message we are receiving
	Serial.println(RAW_CAN_Frame3.U.b[0],HEX); 

	//pass control to other task
	delay(1000);
}

//this is our timer interrupt handler, called at XmS interval
void CAN_RxTx()
{
	//run CAN acquisition schedulers on both ports including OBD and RAW CAN mesages (RX/TX)		 
	CANport0.run(TIMER_2mS);
}
