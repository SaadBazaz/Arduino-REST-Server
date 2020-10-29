#include <Ethernet.h> //Standard Library for Ethernet Server (contains EthernetUDP)
#include <SPI.h>      //Standard Library for networking
#include <SD.h>       //Standard Library for SD card I/O
#include <avr/wdt.h>  //Library for watchdog timer

#define MAXLENGTH_FIRSTLINE 30          //Max line length for the first line of the request
#define MAXLENGTH 150                   //Max line length for other lines of the request
#define MAXLINES 18                     //Max possible lines for the whole request (only used if you try to retrieve data)




/*
 * Handle generic responses
 * 1- Adds the appropriate header
 * 2- Adds the status code you've passed
 * 3- Adds the message (optional)
 */
void handleResponse(EthernetClient& client, char* status_code, char* message = ""){
    if(client.connected()) {
      client.print(F("HTTP/1.1 "));
      client.print(status_code);
      client.println(F("\nContent-Type: text/html\n"));
      client.println(message);
    }
    client.stop();
    Serial.print("\nClient disconnected with ");
    Serial.print(status_code);
}




/*
 * Extract data from the HTTP request
 */
bool getData (EthernetClient& client, char* information, byte& information_letter_count){
      information_letter_count = 0;

      byte wordCount = 0;
      byte letterCount = 0;
      byte lineCount = 0;

      bool dataAhead = false;
      
      while(client.available()) {             
        
          char c = client.read();
    
          // Extract Request Data 
          if (dataAhead == true){
            if (information_letter_count < 30)
              information[information_letter_count++] = c;
            else{
              handleResponse (client, "413 Payload Too Large");
              return false;              
            }
          }
    
          // Check Endlines
          if (c == '\n'){
            lineCount ++;
            if (letterCount == 2){  //In HTTP, data is ahead of the line containing only a newline
              dataAhead = true;              
            }
            letterCount = 0;
          }     
    
          letterCount ++;          
    
          if (letterCount >= MAXLENGTH or lineCount >= MAXLINES){
            handleResponse (client, "413 Payload Too Large");
            return false;
          }
      }
      information[information_letter_count] = '\0'; 
      return true; 
}


/*
 * Instead of making entirely new strings, which we would do when splitting strings,
 * We simply save the "starting position" of the split string and its length. That way,
 * we save space and time.
 * 
 * We can extract the whole string by just doing:
 *
 * ```
 * //assuming "buffer" is a Lite_String 
 * 
 * char * my_extracted_string;
 * my_extracted_string = new char [buffer.length];
 * for (byte i=0; i<buffer.length; i++){
 *  my_extracted_string[i] = buffer.start[i];
 * }
 * 
 * 
 * ```
 * 
 */
struct Lite_String {
  char * start;
  byte length;  
};


/*
 * Take a char array of data, and tokenize it into Lite_Strings
 * "Buffer" behaves like a simple vector class, expanding itself according to its needs
 * Returns the total number of Lite_Strings in "buffer"
 */
byte parseData(char* data, struct Lite_String* &buffer, byte& length){

  byte index = 0;

  length = 4;
  buffer = new Lite_String [length];


  buffer[index].start = &data[0];
  buffer[index].length = 1;  
  for (byte i=0; data[i]!='\0'; i++){
      
      if (data[i] == '=' or data[i] == '&'){
        buffer[++index].start = &data[++i];
        buffer[index].length = 1;
        if (index>=length){
          length*=2;
          Lite_String* temp_buffer = new Lite_String [length];
          for (byte i=0; i<index; i++){
            temp_buffer[i] = buffer[i]; 
          }
          delete buffer;
          buffer = temp_buffer;
        }

                
      }
      else{
         buffer[index].length++;
      }
  }


//  for (byte i=0; i<index+1; i++){
//    for (byte j=0; j<buffer[i].length; j++)
//       Serial.print(buffer[i].start[j]);
//  }


  return index+1;
}




// The milliseconds which we retrieved from the timeserver when the Arduino booted
unsigned long actualMillisFromBoot;


/*
 * Log the provided information to a relevant file
 */
void logger (IPAddress IPaddr, char* username, char* requestMethod, char* route, char* message = ""){
  
    auto logFile = SD.open("actions.log", FILE_WRITE);        // Open log file (from SD card)

    if (logFile) {
        // Calculate timestamp by adding the current timestamp from boot,
        // to the time we got from the server at boot
        unsigned long timeStamp = actualMillisFromBoot + millis();

        logFile.write((byte*)&timeStamp, sizeof(long));
        logFile.write(',');

        // Technique to extract IPAddress into char*
        char IP[16]; // 3*"nnn." + 1*"nnn" + NUL
        snprintf(IP, sizeof(IP), "%d.%d.%d.%d", IPaddr[0], IPaddr[1], IPaddr[2], IPaddr[3]);
                
        logFile.write(IP);
        logFile.write(',');
        logFile.write(username);
        logFile.write(',');
        logFile.write(requestMethod);
        logFile.write(',');
        logFile.write(route);
        if (message){
          logFile.write(',');
          logFile.write(message);
        }
        logFile.write('\n');
    }
    logFile.close();
      
}





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






// Mac Address -> 00:aa:bb:cc:de:06
byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x06};

// Port -> 12345
EthernetServer server = EthernetServer(12345);

// WebFile declared pre-emptively to prevent on-time delay
File webFile;




/*
 * Runs on every boot
 */
void setup() {

  //Enable Watchdog Timer.
  //This will reset the Arduino if it hangs up for 8 seconds
  //In the loop, as we also delay the loop for 2 seconds, we have 7 seconds left to work
  //Which, in hindsight of the Arduino's walnut memory, may be less as well.
  wdt_enable(WDTO_8S);

  Serial.begin(9600);
    
  // initialize SD card
  Serial.println("Init SD card...");
  if (!SD.begin(4)) {
      Serial.println("ERROR - SD init");
      return;    // init failed
  }
  Serial.println("SUCCESS - SD init");

  
  if(Ethernet.begin(mac) == 0) { 
    Serial.println("Failed to Configure");
    return;
  }
  else {
    Serial.print("IP address of Arduino:");
    Serial.println(Ethernet.localIP());
  }
  
  server.begin();
  Serial.println("Server Started");

  // Making all from 1-13 trigger pins
//  for (int i=1; i<9; i++){
    pinMode(1, OUTPUT);    // sets the digital pin as output
    digitalWrite(1, LOW);  // sets the digital pin off
//  }
  Serial.println("Set pin 1 as output");



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
    // or two words, long. First, esxtract the two words:

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




 
//Global data as that seemed like the most efficient option to pass around data,
//as these variables are used in every single request

byte information_letter_count = 0;
byte wordCount = 0;
byte letterCount = 0;
byte lineCount = 0;



/* 
Main looping function
*/
void loop() {

  // The program is alive...for now. 
  wdt_reset();
  
  Serial.println(".");
  EthernetClient client = server.available();

if(client) {

  char* route = "";
  char* requestMethod = "";
  char information [30];

  information_letter_count = 0;

  wordCount = 0;
  letterCount = 0;
  lineCount = 0;
  

  while(client.available()) {             
    
      char c = client.read();
      Serial.print(c);

      // Data in the first line is important to determine which route we'll take, and how
      if (c == ' '){
        information[information_letter_count] = '\0';

        if (wordCount == 0){
                requestMethod = new char [information_letter_count+1]; 
                for (byte i=0; i<information_letter_count; i++)
                  requestMethod[i] = information [i];
                requestMethod [information_letter_count] = '\0'; 
        }
        else if (wordCount == 1){
                route = new char [information_letter_count+1]; 
                for (byte i=0; i<information_letter_count; i++)
                  route[i] = information [i];
                route [information_letter_count] = '\0';
                break;
        }
        wordCount++;
        information_letter_count = 0;
      }
      else{
          information[information_letter_count++] = c;
      }


      if (c == '\n'){
        lineCount ++;
        break;
      }     

      letterCount ++;

      if (letterCount >= MAXLENGTH_FIRSTLINE){
        handleResponse (client, "413 Payload Too Large");
        if (route)
          delete route;
        if (requestMethod)
          delete requestMethod;
        return;
      }
  }


  Serial.print ("Req Mthd: ");
  Serial.print (requestMethod);
  Serial.print ("\nRoute: ");

  route = &route[1];
  
  Serial.print (route);




  /* 
   * ============================= Define routes here ============================= 
   * Simply create a route (e.g. "/login", "/home") by creating an if/else statement containing
   * the parent route. Subroutes can be checked by adding another if/else inside.
   * 
   * You can check the request method as well, with if conditionals.
   * 
   * Be creative! Almost anything a computer server can do, can be done in this Arduino server.
   * Just be careful not to use Strings or any heavy computing. Simple stuff works amazing.
   */
   


    
  /*
  @route: HIGH
  Turns the pin 1 high
  Returns a statement (text/html)
  */
  if (strncmp("H/", route, 2) == 0){

    Serial.println("\nActivating pin 1...");
    digitalWrite(1, HIGH);                   // sets the digital pin 1 on

    if(client.connected()){
      handleResponse(client, "200 OK");      
    }
  }

    

  /*
  @route: PULSE
  Pulses the pin 1 for 500ms
  Returns a statement (text/html)
  */
  else if (strncmp("P/", route, 2) == 0){
    
    Serial.println("\nPulsing pin 1...");
    digitalWrite(1, HIGH);                   // sets the digital pin 1 on
    delay(1000);                               // waits for a second
    digitalWrite(1, LOW);                    // sets the digital pin 1 off

    if(client.connected()){
      handleResponse(client, "200 OK");      
    }
  }

    

  /*
   *  @route: LOW 
   *  Turns the pin 1 low
   *  Returns a statement (text/html)
  */
  else if (strncmp("L/", route, 2) == 0){

    Serial.println("\nDeactivating pin 1...");
    digitalWrite(1, LOW);                    // sets the digital pin 1 off

    if(client.connected()){
      handleResponse(client, "200 OK");      
    }
  }

   
  
  /* 
   * @route: LOGIN
   * GET: Serves a login page from the SD card
   * POST: Receives and validates login data
  */
  else if (strncmp("login", route, 5) == 0){

    if(strcmp(requestMethod, "POST") == 0){   // Handle POST Request

      if ( getData(client, information, information_letter_count) ){

        byte length;
        Lite_String * buffer;
        byte index = parseData (information, buffer, length);

        if (buffer[0].start[0] == 'u'){
          
          char * username = new char [buffer[1].length+1];        //+1 for NULL ending character
          strncpy(username, buffer[1].start, buffer[1].length);
          username[buffer[1].length] = '\0';
          
          logger(client.remoteIP(), username, requestMethod, route);

          // For now, return the username which the client entered
          handleResponse(client, "200 OK", username);       

        }
        else {
          // Was some other format
          handleResponse(client, "422 Unprocessable Entity");                 
        }


      }
      else{
        return;
      }
       
    }
    
    else if(client.connected()) {             // Handle any other Request Method (GET, DELETE, HEAD, etc)
        Serial.println("\nResponse Sent to Client: A HTML Page");
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html\n");
        // Send web page
        webFile = SD.open("index.htm");        // Open web page file (from SD card)
        if (webFile) {
            while(webFile.available() && client.available()) {
                client.write(webFile.read()); // Send web page to client
            }
            webFile.close();
        } 
        client.stop();
        Serial.println("Client is disconnected");
    }    
  }    



    
  /*
   * @route: DELAY
   * Delay the Arduino for 8 seconds. Used to reboot the Arduino forcefully.
   * Returns OK
  */
  else if (strcmp("dl", route)==0){
      delay(8000);
      handleResponse(client, "200 OK");  
  }




 /*
   * @route: LOGS
   * Retrieve the log file
   * Returns OK
  */
  else if (strcmp("logs", route)==0){
     if(client.connected()) {
        Serial.println("\nResponse Sent to Client: Text");
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html\n");
        // send web page
        webFile = SD.open("actions.log");        // open log file (from SD card)
        if (webFile) {
            while(webFile.available() && client.available()) {
                client.write(webFile.read());    // send log file to client
            }
            webFile.close();
        } 
        client.stop();
        Serial.println("Client is disconnected");
      }  
  }
    


    
  /*
   * @route: NOT_FOUND
   * If route doesn't exist above, land here
   * Returns a statement (text/html)
  */
  else {
    handleResponse(client, "404 Not Found");
  }


  // free dynamically allocated memory
  delete route;
  delete requestMethod;

}

delay(3000); // Wait for 3 seconds before starting to listen again
}
