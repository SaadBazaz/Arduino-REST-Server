
/*
  Web client

This sketch connects to a website (http://www.google.com)
using an Arduino Wiznet Ethernet shield. 

Circuit:
* Ethernet shield attached to pins 10, 11, 12, 13

created 18 Dec 2009
by David A. Mellis

modified for no-ip client example
by Jerry Sy aka doughboy
*/

#include <SPI.h>
#include <Ethernet.h>


bool sendDDNSUpdate(EthernetClient& client){

  // if you get a connection, report back via serial:
  if (client.connect("dynupdate.no-ip.com", 80)) {
    //Serial.println("connected");
    // Make a HTTP request:
    //replace yourhost.no-ip.org with your no-ip hostname
    client.println("GET /nic/update?hostname=yourhhost.no-ip.org HTTP/1.0");
    client.println("Host: dynupdate.no-ip.com");
    //encode your username:password (make sure colon is between username and password) 
    //to base64 at http://www.opinionatedgeek.com/dotnet/tools/base64encode/
    //and replace the string below after Basic with your encoded string
    //clear text username:password is not accepted
    client.println("Authorization: Basic dXNlcm5hbWU6cGFzc3dvcmQ="); 
    client.println("User-Agent: Arduino Sketch/1.0 user@host.com");
    client.println();
return true;
  } 
  else {
    // if you didn't get a connection to the server:
    //Serial.println("connection failed");
return false;
  }

}
