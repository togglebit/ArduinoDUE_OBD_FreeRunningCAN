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
        1. download Arduino IDE R 1.5.5 r2
        2. download all of the files from this repository
        3. With the exception of variant.h, drop the *.c and *.h files into C:\Program Files (x86)\Arduino\hardware\arduino\sam\cores\arduino 
        4. Copy variant.h into C:\Program Files (x86)\Arduino\hardware\arduino\sam\variants\arduino_due_x
        5. Create a new folder called "OBD2" in C:\Program Files (x86)\Arduino\libraries
        6. Inside the OBD2 folder create a folder called "examples". Inside that folder create folders for each indivdual example with only that 
           example file located inside the folder "OBD_Example_1" 

## Getting Started with Free Running CAN

        To transmit a message five times a second:        

        ```c++
        //create the CANport acqisition schedulers
        cAcquireCAN CANport0(CAN_PORT_0);
        
        //define a CANFrame object 
        cCANFrame  RAW_CAN_Frame1;
        
        //start CAN ports, set the baud rate here
	CANport0.initialize(_500K);
        
        //then set the ID and transmission rate 
        RAW_CAN_Frame1.ID = 0x100;
	RAW_CAN_Frame1.rate  = _5Hz_Rate;
        
        //add to the scheduler 
        CANport0.addMessage(&RAW_CAN_Frame1, TRANSMIT);
        
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
        cOBDParameter OBD_Speed("Speed ", " KPH" ,  SPEED , _8BITS,  false, CURRENT,  1, 0,  &CANport0);
        
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
        - Yes, we've hijacked and modified the variants.h file
        - the OBD2 has only beent tested on one vehicle so far with 11-bit identifiers
        - To create an OBD PID that does not yet exist see the relevant enums in the OBD2.h file.  
        - Thanks to collin80 for developing the original due_can.* libraries and ivanseidel for developoing the DUETimer libraries
        - When in doubt, check your wiring and termination resistors ;)

### Hardware Information

See: www.togglebit.net for the latest CANshield and protoshield hardware for the Arduino DUE

### Version History

* `1.0 (2014-18-03)`: Original release


