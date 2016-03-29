# OBD2 and CAN Acquisition Libraries

These libraries were designed with contious message TX/RX (DAQ applications) in mind by implementing a periodic scheduler.
The scheduler supports both native DUE CAN ports and allows for one to easily implement a "free-running raw" CAN protocol
or perhaps something a bit more layered such as OBD2. An OBD2 implemenation is provided, and logged CAN traffic and screen 
data are provided in the repository (driveHome*.* files). 

Thank you for actually reading the readme and contributing!
                                                                                      
Dan
dan@togglebit.net


## Installation
What you (probably) need to do to get it working (assuming a windows installation):
        1. download Arduino IDE R 1.6.x
        2. download all of the files from this repository
        3. Create a new folder called "CAN" under a path C:\Users\"XXXX"\Documents\Arduino\libraries\CAN
        4. Drop all files( *.c,*.h and *.ino) into the new folder (C:\Users\"XXXX"\Documents\Arduino\libraries\CAN) 
        5. Go to Tools->Boards->Boards Manager and download the Adruino SAM (32-bit ARM cortex-M3) boards hardware support kit
        6. Once installed, go to boards select the Arduino DUE from.  
        7. Opening any of the examples ( *.ino) files will create a new folder. 
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

## Getting Started with OBD2 PID's (reference http://en.wikipedia.org/wiki/OBD-II_PIDs)

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
        - Thanks to collin80 for developing the original due_can.* libraries and ivanseidel for developoing the DUETimer libraries
        - When in doubt, check your wiring and termination resistors ;)

### Hardware Information

See: www.togglebit.net for the latest CANshield and protoshield hardware for the Arduino DUE

### Version History

* `1.1 (2015-13-09)`:


