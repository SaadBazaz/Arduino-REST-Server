#include <avr/wdt.h>  //Library for watchdog timer


class CurrentTime {
// The milliseconds which we retrieved from the timeserver when the Arduino booted
unsigned long actualMillisFromBoot;


//toDo
unsigned long millis_from_last_update = 0;

/* 
 * Send an NTP request to the time server at the given address 
 */
void sendNTPpacket(EthernetUDP& Udp, byte * packetBuffer, const int& NTP_PACKET_SIZE, const char * address) {

  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:

  Udp.beginPacket(address, 123); // NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}


public:

CurrentTime () {
  /*
   * Get actual Timestamp from a dedicated time server (NTP)
   * ------------------------------------------------------- START
   */

  EthernetUDP Udp;
  Udp.begin(8888);



  /*
   * Time Related variables
   */
  IPAddress timeServer(192, 43, 244, 18); // time.nist.gov NTP server
  
  const int NTP_PACKET_SIZE = 48;         // NTP time stamp is in the first 48 bytes of the message
  
  byte packetBuffer[NTP_PACKET_SIZE];     // Buffer to hold incoming and outgoing packets


  sendNTPpacket(Udp, packetBuffer, NTP_PACKET_SIZE, timeServer); // Send an NTP packet to a time server

  // Wait to see if a reply is available
  delay(1000); 
  if ( Udp.parsePacket() ) { 
    // We've received a packet, read the data from it
    Udp.read(packetBuffer,NTP_PACKET_SIZE);  // Read the packet into the buffer

    // The timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, extract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]); 
    // Combine the four bytes (two words) into a long integer
    // This is NTP time! (seconds since Jan 1 1900):
    actualMillisFromBoot = (highWord << 16 | lowWord) * 60; 
  }

  Udp.stop();

  /*
   * Get actual Timestamp from a dedicated time server (NTP)
   * ------------------------------------------------------- END
   */
}

unsigned long getTimeMillis (){
    // Calculate timestamp by adding the current timestamp from boot,
    // to the time we got from the server at boot
    return actualMillisFromBoot + millis();
}

char* str (unsigned long milli) {
	//toDo
	//3600000 milliseconds in an hour
	long hr = milli / 3600000;
	milli = milli - 3600000 * hr;
	//60000 milliseconds in a minute
	long min = milli / 60000;
	milli = milli - 60000 * min;

	//1000 milliseconds in a second
	long sec = milli / 1000;
	milli = milli - 1000 * sec;	

}

}
