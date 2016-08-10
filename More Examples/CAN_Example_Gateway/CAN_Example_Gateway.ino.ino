#include <CAN_Acquisition.h>
/**************************************************************************************************************************************************************************
This example is built upon the "CANAcquisition" class cAcquireCAN. 
This example shows how to receive message 0x7E8, modify it and re-send it on CAN1 while acting as a "gateway" and pass through the rest of the ID's between CAN0 and CAN 1

/*************************************************************************************************************************************************************************/

//create the CANport acqisition schedulers
cAcquireCAN CANport0(CAN_PORT_0);
cAcquireCAN CANport1(CAN_PORT_1);
    
/**
 * inheriting this class allows us to implement the RX callback function at the top layer
 */
class cGatewayRXFrameCAN0 : public cCANFrame
{
    bool  CallbackRx(RX_CAN_FRAME *R);
};

class cGatewayRXFrameCAN1 : public cCANFrame
{
    bool  CallbackRx(RX_CAN_FRAME *R);
};

//we are going to gateway messages 101,7DF,7E0,7E1, 0x7E8

/****** CAN 0 messages ******/
cGatewayRXFrameCAN0  CAN0_0x7E8;
cGatewayRXFrameCAN0  CAN0_0x101;
cGatewayRXFrameCAN0  CAN0_0x7E0;
cGatewayRXFrameCAN0  CAN0_0x7E1;
cGatewayRXFrameCAN0  CAN0_0x7DF;

/****** CAN 1 messages ******/
cGatewayRXFrameCAN1  CAN1_0x7E8;
cGatewayRXFrameCAN1  CAN1_0x101;
cGatewayRXFrameCAN1  CAN1_0x7E0;
cGatewayRXFrameCAN1  CAN1_0x7E1;
cGatewayRXFrameCAN1  CAN1_0x7DF;



UINT8 i;

void setup()
{
    //start serial port at 115.2kbps for PC comms
    Serial.begin(115200);

    //debugging message for monitor to indicate CPU resets are occuring
    Serial.println("System Reset");

    //output pin that can be used for debugging purposes
    pinMode(13, OUTPUT);      

    //start CAN ports, set the baud rate here. 
    CANport0.initialize(_500K);
    CANport1.initialize(_500K);

    //initialize raw CAN mesasges
    /****** CAN 0 - Messages ******/
    CAN0_0x7E8.ID = 0x7E8;   
    CAN0_0x101.ID = 0x101;   
    CAN0_0x7E0.ID = 0x7E0;   
    CAN0_0x7E1.ID = 0x7E1;   
    CAN0_0x7DF.ID = 0x7DF;   
  
    /****** CAN 1 - Messages ******/
    CAN1_0x7E8.ID = 0x7E8;
    CAN1_0x101.ID = 0x101;
    CAN1_0x7E0.ID = 0x7E0;
    CAN1_0x7E1.ID = 0x7E1;
    CAN1_0x7DF.ID = 0x7DF;

    //add messages to rx (set RX filters)
    CANport0.addMessage(&CAN0_0x7E8, RECEIVE);
    CANport0.addMessage(&CAN0_0x101, RECEIVE);
    CANport0.addMessage(&CAN0_0x7E0, RECEIVE);
    CANport0.addMessage(&CAN0_0x7E1, RECEIVE);  
    CANport0.addMessage(&CAN0_0x7DF, RECEIVE);  
 
    CANport1.addMessage(&CAN0_0x7E8, RECEIVE);
    CANport1.addMessage(&CAN0_0x101, RECEIVE);
    CANport1.addMessage(&CAN0_0x7E0, RECEIVE);
    CANport1.addMessage(&CAN0_0x7E1, RECEIVE);
    CANport1.addMessage(&CAN0_0x7DF, RECEIVE);
}

//this is our timer interrupt handler, called at XmS interval
void CAN_RxTx()
{
    //receive messages, don't periodically transmit (but you can transmit asynchronously via CANportx.TXmsg(xxx) in our overloaded messages below)	 
    CANport0.run(POLLING_noTx);
    CANport1.run(POLLING_noTx);
}

void loop()
{
    //handle can port RX in tight loop
    CAN_RxTx();
}


//this method is called whenever we receive a registered message on CAN 0
bool cGatewayRXFrameCAN0::CallbackRx(RX_CAN_FRAME *R)
{
    bool retVal = false;
    U8 i;
    if (R)
    {
        //go ahead and immediately stuff message payload from raw can frame
        for (i=0;i<8;i++)
        {
            this->U.b[i] = R->data.bytes[i];
        }
        
        //have a look to see if this is the message we are looking for (0x7E8) on CAN0 to modify and pass through on CAN1
        if(this->ID == 0x7E8)
        {
            //we want to increment the last byte of this message and resend this on CAN1
            this->U.b[7] += 1;
        }
        
        //gateway all registered RX messages from CAN 0 to CAN 1
        CANport1.TXmsg(this);
    }
    return(retVal);
}

//this method is called whenever we receive a registered message on CAN 1
bool cGatewayRXFrameCAN1::CallbackRx(RX_CAN_FRAME *R)
{
    bool retVal = false;
    U8 i;
    if (R)
    {
        //go ahead and immediately stuff message payload from raw can frame
        for (i=0;i<8;i++)
        {
            this->U.b[i] = R->data.bytes[i];
        }

        //gateway all registered RX messages from CAN 1 to CAN 0
        CANport0.TXmsg(this);
    }
    return(retVal);
}
