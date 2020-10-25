# Arduino-Web-Server
Simple, super-compact webserver code for Arduino boards

### Features
- Simple routing
- Optimized yet easily editable
- Does not consume enough memory to cause instability

### Notes
For production, comment all Serial.print statements and it'll run faster than before.
You can edit the MAXLENGTH_FIRSTLINE preprocessor according to your needs. I recommend building very very small routes, preferably 1 character only. This would optimize performance.

You can also remove unnecessary messages being returned to the client. The browser/app should be capable enough to understand the response just with the HTTP response code.

##### Tested on
Arduino Uno with Ethernet Shield, and a 16GB (overkill) SD card.


Have fun! If you're able to optimize this or make it better, please make a Pull Request.

