#include <OBD2.h>
#include <DueTimer.h>
/********************************************************************
This example is built upon the "CANAcquisition" class cAcquireCAN 
which is a simple scheduler for periodic TX/RX of CAN messages. This 
example should be used to test that you have built the hardware properly.

This example shows how to set up a "free-running" CAN bus where you have:
    - Two messages being transmitted at 1Hz 
    - Two messages being received at 1Hz
/********************************************************************/

//create the CANport acqisition schedulers
cAcquireCAN CANport0(CAN_PORT_0);
cAcquireCAN CANport1(CAN_PORT_1);


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
    CANport0.initialize(_500K);
    CANport1.initialize(_500K);

    //initialize the items needed to TX/RX raw CAN mesasges
    RAW_CAN_Frame1.ID    = 0x100;
    RAW_CAN_Frame1.rate  = _1Hz_Rate;
    RAW_CAN_Frame2.ID    = 0x101;
    RAW_CAN_Frame2.rate  = _1Hz_Rate;

    //add our raw messages to the scheduler	1
    CANport0.addMessage(&RAW_CAN_Frame1, TRANSMIT);
    CANport0.addMessage(&RAW_CAN_Frame2, RECEIVE);
    CANport1.addMessage(&RAW_CAN_Frame1, RECEIVE);
    CANport1.addMessage(&RAW_CAN_Frame2, TRANSMIT);


    //set up the transmission/reception of messages to occur at 500Hz (2mS) timer interrupt
    Timer3.attachInterrupt(CAN_RxTx).setFrequency(500).start();

    //output pin that can be used for debugging purposes
    pinMode(13, OUTPUT); 

}

UINT8 i,CAN0_rxCtr,CAN1_rxCtr;
UINT32 maxTime;
UINT8 passFail;

void loop()
{
    //set pass flag
    passFail =  true;

    //first check to see if CAN port 0 is recieving data from port 1
    if (CAN0_rxCtr != CANport0.getRxCtr())
    {
        Serial.println("CAN0 is Receiving Data from CAN1,"); 
    } else
    {
        Serial.println("CAN0 is NOT Receiving Data from CAN1,"); 

        //indicate test failed
        passFail =  false;
    }
    //capture counter
    CAN0_rxCtr = CANport0.getRxCtr();

    //first check to see if CAN port 1 is recieving data from port 0
    if (CAN1_rxCtr != CANport1.getRxCtr())
    {
        Serial.println("CAN1 is Receiving Data from CAN0,"); 
    } else
    {
        Serial.println("CAN1 is NOT Receiving Data from CAN0,"); 

        //indicate test failed
        passFail =  false;
    }
    //capture counter
    CAN1_rxCtr = CANport1.getRxCtr();

    //indicate if board has passed or failed construction
    if (passFail)
    {
        Serial.println("The CAN shield has passed construciton testing!"); 
        Serial.println("****************************************************");  
    } else
    {
        Serial.println("The CAN shield has failed construciton testing. Here are some things to check: "); 
        Serial.println(" - remove all loopback jumpers"); 
        Serial.println(" - install jumper on at least one termination resistor"); 
        Serial.println(" - make sure wiring is CAN0 H -> CAN1 H, CAN0 L -> CAN1 L, GND -> GND"); 
        Serial.println(" - make sure the shield header pins are is properly seated on the Arduino DUE. See webiste www.togglebit.net notes on SPI header"); 
        Serial.println(" - see www.togglebit.net for minimum number of headers to be installed on shield");
        Serial.println("****************************************************");
    }

    //pass control to other task, delay for 2 sec to see if counter has changed
    delay(2000);
}

//this is our timer interrupt handler, called at XmS interval
void CAN_RxTx()
{
    //run CAN acquisition schedulers on both ports including OBD and RAW CAN mesages (RX/TX)		 
    CANport0.run(TIMER_2mS);
    CANport1.run(TIMER_2mS);
}
