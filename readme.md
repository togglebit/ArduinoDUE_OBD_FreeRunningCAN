# OBD2 and CAN Acquisition Libraries

These libraries were designed with continuous message TX/RX (DAQ applications) in mind by implementing a periodic scheduler.
The scheduler supports both native DUE CAN ports and allows for one to easily implement a "free-running raw" CAN protocol
or perhaps something a bit more layered such as OBD2. An OBD2 implementation is provided.  Sample, logged CAN traffic and screen data are provided in the `extras` directory (the `driveHome*.*` files).

Thank you for actually reading the readme and contributing!

Dan  
dan@togglebit.net


## Installation
What you (probably) need to do to get it working (assuming a Windows installation):

1. Install Arduino IDE 1.6.x or newer.
2. Download all of the files from the [`due_can` repository](https://github.com/collin80/due_can) using the "Download ZIP" option.
2. Download all of the files from this repository using the "Download ZIP" option.
3. In the Arduino IDE, go to _Sketch_ -> _Include Library_ -> _Install .ZIP Library..._ and choose the ZIP file for `due_can`.
3. Do the above again, but choosing the ZIP file for this library.
5. Go to _Tools_ -> _Board: [...]_ -> _Boards Manager..._ and download the **Adruino SAM Boards (32-bit ARM Cortex-M3)** hardware support kit.
6. Once installed, go to _Tools_ -> _Board: [...]_ and select **Arduino Due**.
7. Go to _File_ -> _Examples_ -> _OBD2 and CAN Acquisition_ under the _Examples from Custom Libraries_ section to open one of the examples for this library.
8. You will now be able to verify, upload etc.

## Getting Started with Free Running CAN

To transmit a message five times a second:        

```c++
//create the CANport acqisition schedulers
cAcquireCAN CANport0(CAN_PORT_0);

//define a CANFrame object 
cCANFrame  RAW_CAN_Frame1;

//then set the ID and transmission rate 
RAW_CAN_Frame1.ID = 0x100;
RAW_CAN_Frame1.rate  = _5Hz_Rate;

//add to the scheduler 
CANport0.addMessage(&RAW_CAN_Frame1, TRANSMIT);

//start CAN ports, set the baud rate here
CANport0.initialize(_500K);


// call the scheduler in a 2mS timer iterrupt
CANport0.run(TIMER_2mS);

// manipulate CAN payload in loop() or wherever appropriate
RAW_CAN_Frame1.U.b[0] = i;
```

## Getting Started with OBD2 PID's
(reference http://en.wikipedia.org/wiki/OBD-II_PIDs)

To get engine RPM:        

```c++
//create the CANport acqisition schedulers
cAcquireCAN CANport0(CAN_PORT_0);

//define an OBDParameter object 
cOBDParameter OBD_Speed("Speed ", " KPH" ,  SPEED , _8BITS,  false, CURRENT,  1, 0,  &CANport0, false);

//start CAN ports, set the baud rate here
CANport0.initialize(_500K);

// call the scheduler, preferable in an interrupt
CANport0.run(TIMER_2mS);

//show acquired data in loop() wherever appropriate
Serial.print(OBD_EngineSpeed.getName()); 
Serial.print(OBD_EngineSpeed.getData());
Serial.println(OBD_EngineSpeed.getUnits());
```
### TIPs and Warnings

- The first revision of code was developed for functionality and not speed. For example, there is only one CAN mailbox implemented.
  Many many speed efficiencies are yet to be found and optimized.
- The underlying CAN library provided here "due_can.*" may not be the latest contributions from the DUE forum.
- the OBD2 has only been tested on 11bit ID's with Toyota vehicle and 29bit with Honda vehicle
- To create an OBD PID that does not yet exist see the relevant enums in the OBD2.h file.  
- Thanks to collin80 for developing the original due_can.* libraries and ivanseidel for developing the DUETimer libraries
- When in doubt, check your wiring and termination resistors ;)

### Hardware Information

See: www.togglebit.net for the latest CANshield and protoshield hardware for the Arduino DUE

### Version History

* `1.2 (2017-06-09)`: Unbundle `due_can` library.  Update to support latest version of `due_can`.  Add support for the Arduino IDE Library manager.
* `1.1 (2015-13-09)`:
* `1.0 (2014-18-03)`: Original release
