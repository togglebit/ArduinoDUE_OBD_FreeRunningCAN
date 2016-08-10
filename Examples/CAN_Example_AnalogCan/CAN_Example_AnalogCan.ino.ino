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

void setup()
{
	//start serial port
	Serial.begin(115200);

	//debugging message for monitor to indicate CPU resets are occuring
	Serial.println("System Reset");

	//start CAN ports, set the baud rate here
	CANport0.initialize(_1000K);

	//initialize the items needed to TX/RX raw CAN mesasges
	RAW_CAN_Frame1.ID = 0x100;
	RAW_CAN_Frame1.rate  = _10Hz_Rate;

	RAW_CAN_Frame2.ID = 0x101;
	RAW_CAN_Frame2.rate  = _5Hz_Rate;
       
	//add our raw messages to the scheduler	1
	CANport0.addMessage(&RAW_CAN_Frame1, TRANSMIT);
	CANport0.addMessage(&RAW_CAN_Frame2, TRANSMIT);

	//set up the transmission/reception of messages to occur at 500Hz (2mS) timer interrupt
	Timer3.attachInterrupt(CAN_RxTx).setFrequency(500).start();

	//output pin that can be used for debugging purposes
	pinMode(13, OUTPUT);      
}


UINT8 i;
UINT16 analogInputs[8];
UINT32 maxTime;

void loop()
{
	// read all raw ADC values for the CAN transmissions
        for(i=0; i<8; i++)
        {
          analogInputs[i] = analogRead(i);
        }
        
        RAW_CAN_Frame1.U.P.upperPayload = analogInputs[0] << 16;
        RAW_CAN_Frame1.U.P.upperPayload |= analogInputs[1] & 0xFFFF;

        RAW_CAN_Frame1.U.P.lowerPayload = analogInputs[2] << 16;
        RAW_CAN_Frame1.U.P.lowerPayload |= analogInputs[3] & 0xFFFF;

        RAW_CAN_Frame2.U.P.upperPayload = analogInputs[4] << 16;
        RAW_CAN_Frame2.U.P.upperPayload |= analogInputs[5] & 0xFFFF;

        RAW_CAN_Frame2.U.P.lowerPayload = analogInputs[6] << 16;
        RAW_CAN_Frame2.U.P.lowerPayload |= analogInputs[7] & 0xFFFF;

	//pass control to other task
	delay(100);
}

//this is our timer interrupt handler, called at XmS interval
void CAN_RxTx()
{
	//run CAN acquisition schedulers on both ports including OBD and RAW CAN mesages (RX/TX)		 
	CANport0.run(TIMER_2mS);
}
