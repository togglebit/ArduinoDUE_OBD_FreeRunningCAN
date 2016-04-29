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
UINT8 i;
UINT32 maxTime;
UINT16 engSpeed;
UINT8  gear;

void setup()
{
	//start serial port
	Serial.begin(115200);

	//debugging message for monitor to indicate CPU resets are occuring
	Serial.println("System Reset");

	//start CAN ports, set the baud rate here
	CANport0.initialize(_500K);

	//initialize the items needed to TX/RX raw CAN mesasges
	RAW_CAN_Frame1.ID = 0xf004;
	RAW_CAN_Frame1.rate  = _1Hz_Rate;
  RAW_CAN_Frame2.ID = 0xf005;
  RAW_CAN_Frame2.rate  = _1Hz_Rate;
  
     
	//add our raw messages to the scheduler	1
	CANport0.addMessage(&RAW_CAN_Frame1, TRANSMIT);
  CANport0.addMessage(&RAW_CAN_Frame2, TRANSMIT);


	//set up the transmission/reception of messages to occur at 500Hz (2mS) timer interrupt
	Timer3.attachInterrupt(CAN_RxTx).setFrequency(500).start();

	//output pin that can be used for debugging purposes
	pinMode(13, OUTPUT);      

  gear =1;
}




void loop()
{
  //20,000 = 2500rpm 0.125rpm per bit, 
  engSpeed = engSpeed < 20000 ? engSpeed + 100 : 0;
  gear = engSpeed == 0 ? gear + 1 : gear;
  gear = gear < 5 ? gear : 1;
  
	RAW_CAN_Frame1.U.b[3] = engSpeed & 0xFF;
  RAW_CAN_Frame1.U.b[4] = ( engSpeed >> 8 ) & 0xFF;
  
  //gear has an offset of 125
  RAW_CAN_Frame2.U.b[3] = gear + 125;

	//pass control to other task
	delay(100);
}

//this is our timer interrupt handler, called at XmS interval
void CAN_RxTx()
{
	//run CAN acquisition schedulers on both ports including OBD and RAW CAN mesages (RX/TX)		 
	CANport0.run(TIMER_2mS);
}
