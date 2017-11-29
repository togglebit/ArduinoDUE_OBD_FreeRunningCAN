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
#include "variant.h"
#include <due_can.h>
#include <CAN_Acquisition.h>

#ifndef OBD2_H
#define OBD2_H

/**
 * 
 * This macro is used to set the maximum string length for signal name and engineering units
 */
#define STR_LNGTH 		25

/**
 * 
 * This macro is used to set the maximum number of OBD2 Parameter ID's a class can have
 */
#define	MAX_NUM_PIDS	20

/**
 * 
 * This enum represents the Parameter ID field for a particular signal per OBD2 protocol
 */
enum OBD_PID
{

	ENGINE_LOAD  = 0x04,
	COOLANT_TEMP = 0x05,
	ENGINE_RPM   = 0x0C,
	SPEED        = 0x0D,
	ENGINE_IAT   = 0x0F,
	ENGINE_MAF   = 0x10,
	THROTTLE_POS = 0x11,
	FUEL_FLOW    = 0x5E
};

/**
 * 
 * This enum represents the size of the OBD2 signal in bits (8,16,32) per OBD2 protocol
 */
enum OBD_PID_SIZE
{
	_8BITS  = 1,
	_16BITS = 2,
	_32BITS = 4
};

/**
 * 
 * This enum represents the desired mode of the signal requested from the OBD2 port
 */
enum OBD_MODE_REQ
{
	CURRENT  = 1,
	FREEZE   = 2
};

/**
 * this is the receive frame that is used to make the OBD data request to the CAN receiver
 * inheriting this class allows us to implement the RX callback function at the top layer
 */
class cOBDRXFrame : public cCANFrame
{

	bool  CallbackRx(RX_CAN_FRAME *R);
};
/**
 * this is the transmitit frame that is used to make the OBD data request CAN receiver
 * inheriting this class allows us to implement the TX callback function at a higher layer if needed
 */
class cOBDTXFrame : public cCANFrame
{

	bool  CallbackTx();
};



/**
 * OBD class that is used to create a "new" PID parameter. All attributes of the PID message are defined here.
 * Name, slope, offset, parameter number etc. The intention is for the user to define these
 * in the sketch passing a reference to the appropriate acquisition scheduler (physical can port) for periodic RX/TX of mesages.
 * 
 * @author D.Kasamis - dan@togglebit.net
 * @version 1.0
 */
class cOBDParameter 
{
public:
	/**
	* This is the constructor for an OBD parameter. One class should be created and initialized for each parameter ID.
	* If you intend to add parameter ID's that are not in this class already, you'll need to simply add it to the ENUM list "OBD_PID"
	* All other parameters can be dervied from the general OBD2 specficaitons (http://en.wikipedia.org/wiki/OBD-II_PIDs).
	* 
	* 
	* @param _name    -  string describing channel name
	* @param _units   -  string describing engineering units
	* @param _pid     -  parameter ID number
	* @param _size    -  size of the mesage (8, 16 bits etc)
	* @param _signed  -  flag indicating if it is a signed number (sint, uint, etc)
	* @param _mode    -  this is the OBD mode of data we have requested (as provided by the standard) current data, freeze, etc,
	* @param _slope   -  this OBD class assumes a linear relationship between counts and units. This is the slope.
	* @param _offset  -  this sensor class assumes a linear relationship between counts and units. This is the offset.
	* @param _portNum -  physical CAN port to be used for this OBD parameter
	* @param _extended-  indicate we are using OBD2 extended ID's
	*/
	cOBDParameter (char _name[STR_LNGTH],
				   char _units[STR_LNGTH],
				   OBD_PID _pid,
				   OBD_PID_SIZE _size,
				   bool _signed,
				   OBD_MODE_REQ _mode,
				   float _slope,
				   float _offset,
				   cAcquireCAN *_portNum,
                   bool _extended);
	/**
	 * Retreive OBD2 signal data in floating point engineering units. Note the floating point/EU conversion only occurs
	 * when this method is called
	 * 
	 * @return floating point engineering unit representation of OBD2 signal
	 */
	float getData();
	/**
	 * This method is responsible for extracting the data portion of a received CAN frame (OBD message) based upon 8,16,32bit message sizes.
	 * This works in UINT32 data type (calling funciton needs to cast this for sign support).
	 * 
	 * @return UINT32 - integer that represents OBD message data (no scaling or sign applied)
	 */
	UINT32 getIntData();
	/**
	 * this handler is called when a CAN frame ID matching a OBD2 response message is received
	 * 
	 * @param I - pointer to the received CAN frame
	 * @return -  bool this is the proper response for this PID
	 */
	bool receiveFrame(RX_CAN_FRAME *I);
	/**
	 * Retreive the string representing the OBD2 signal name
	 * 
	 * @return - pointer to null-terminated ASCII char string for signal name
	 */
	char* getName();
	/**
	 * Retreive the string representing the OBD2 signal's engineering units
	 * 
	 * @return - pointer to null-terminated ASCII char string for signal units
	 */
	char* getUnits();

protected:
	private:
	/**
	 * string representing parameter name
	 */
	char name[STR_LNGTH];

	/**
	 * string defining units
	 */
	char units[STR_LNGTH];    

	/**
	 * ID number
	 */
	OBD_PID pid;

	/**
	 * size of the mesage (8, 16 bits etc)
	 */
	OBD_PID_SIZE size;

	/**
	 * this is the OBD mode of data we have requested (as provided by the standard) current data, freeze, etc, 
	 */
	OBD_MODE_REQ dataMode;

	/**
	 * The OBD class assumes a linear relationship between digital representation and engineering units. This is the slope and offsett (Y = mX+b)
	 */
	float m; 
	float b;

	/**
	 * flag indicating if it is a signed number (sint, uint, etc)
	 */
	bool sign; 

	/**
	 * periodic scheduler that is to be used aka - physical port to be used
	 */
	cAcquireCAN *portNum;

	/**
	 * this is the transmitit frame that is used to make the OBD data request CAN receiver
	 */
	cOBDTXFrame TXFrame;

	/**
	 * this is the receive frame that is used to make the OBD data request to the CAN receiver
	 */
	cOBDRXFrame RXFrame;

	/**
	 * this is a list that keeps track of all of the OBD2 PID created
	 */
	static cOBDParameter *OBDList[MAX_NUM_PIDS];
	static UINT8  listIdx;
};






#endif
