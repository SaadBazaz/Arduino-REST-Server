# Arduino-Web-Server
Simple, super-compact webserver code for Arduino boards

### Features
- Simple routing
- Time-logging from an actual timeserver
- Extracts and parses request body
- Optimized yet easily editable
- Does not consume enough memory to cause instability (Consuming just ~85% Program memory, ~68% SRAM)

### Notes
For production, comment all Serial.print statements and it'll save ~20% SRAM compared to uncommented.

You can edit the MAXLENGTH_FIRSTLINE preprocessor definition according to your needs. I recommend building very very small routes, preferably 1 character only. This would optimize performance.

You can also remove unnecessary messages being returned to the client. The browser/app should be capable enough to understand the response just with the HTTP response code.

If the Arduino hangs or takes more than 8 seconds, it is automatically rebooted.

Incase of file corruption, please try to delete the files from the SD card. 

##### Tested on
Arduino Uno with Ethernet Shield, and a 16GB (overkill) SD card.


Have fun! If you're able to optimize this or make it better, please make a Pull Request.

