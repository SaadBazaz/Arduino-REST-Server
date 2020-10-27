#include <Ethernet.h> //Standard Library for Ethernet Server
#include <SPI.h>      //Standard Library for networking
#include <SD.h>       //Standard Library for SD card I/O
#include <avr/wdt.h>  //Library for watchdog timer

#define MAXLENGTH_FIRSTLINE 30          //Max line length for the first line of the request
#define MAXLENGTH 150                   //Max line length for other lines of the request
#define MAXLINES 18                     //Max possible lines for the whole request (only used if you try to retrieve data)



/*
Function to handle generic responses
1- Adds the appropriate header
2- Adds the status code you've passed
3- Adds the message (optional)
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
Function to extract data from the HTTP request
*/
bool getData (EthernetClient& client, char* information, byte& information_letter_count){
      information_letter_count = 0;

      byte wordCount = 0;
      byte letterCount = 0;
      byte lineCount = 0;

      bool dataAhead = false;
      
      while(client.available()) {             
        
          char c = client.read();
//          Serial.print(c);
    
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


// Mac Address -> 00:aa:bb:cc:de:06
byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x06};

// Port -> 12345
EthernetServer server = EthernetServer(12345);

File webFile;


void setup() {

  //Enable Watchdog Timer.
  //This will reset the Arduino if it hangs up for 8 seconds
  //In the loop, as we also delay the loop for 2 seconds, we have 7 seconds left to work
  //Which, in hindsight of the Arduino's walnut memory, may be less as well.
  wdt_enable(WDTO_8S);
  
  
  Serial.begin(9600);
  boolean receiving = false;

    
  // initialize SD card
  Serial.println("Init SD card...");
  if (!SD.begin(4)) {
      //Serial.println("ERROR - SD init");
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

  
}



/* 
Global data as that seemed like the most efficient option to pass around data,
as these variables are used in every single request
*/
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




  // ============================= Define routes here =============================
  // Simply create a route (e.g. "/login", "/home") by creating an if/else statement containing
  // the parent route. Subroutes can be checked by adding another if/else inside.


    
  /*
  @route: HIGH
  Turns the pin 1 high
  Returns a statement (text/html)
  */
  if (strncmp("H/", route, 2) == 0){

    Serial.println("\nActivating pin 7...");
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
  @route: LOW
  Turns the pin 1 low
  Returns a statement (text/html)
  */
  else if (strncmp("L/", route, 2) == 0){

    Serial.println("\nDeactivating pin 13...");
    digitalWrite(1, LOW);                    // sets the digital pin 1 off

    if(client.connected()){
      handleResponse(client, "200 OK");      
    }
  }

   
  /* 
  @route: LOGIN
  GET: Serves a login page from the SD card
  POST: Receives and validates login data
  */
  else if (strncmp("login", route, 5) == 0){

    if(strcmp(requestMethod, "POST") == 0){
   

      if ( getData(client, information, information_letter_count) ){

        Serial.print("\nInfo is: ");
        Serial.print(information);

        // For now, return the information which the client entered
        handleResponse(client, "200 OK", information);       
      }
      else{
        return;
      }
       
    }
    
    else if(client.connected()) {
        Serial.println("\nResponse Sent to Client: A HTML Page");
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html\n");
        // send web page
        webFile = SD.open("index.htm");        // open web page file
        if (webFile) {
            while(webFile.available() && client.available()) {
                client.write(webFile.read()); // send web page to client
            }
            webFile.close();
        } 
        client.stop();
        Serial.println("Client is disconnected");
    }    
  }    
    
  /*
  @route: DELAY
  Delay the Arduino for 8 seconds. Used to reboot the Arduino forcefully.
  Returns OK
  */
  else if (strcmp("dl", route)==0){
      delay(8000);
      handleResponse(client, "200 OK");  
  }
    
    
  /*
  @route: NOT_FOUND
  If route doesn't exist above, land here
  Returns a statement (text/html)
  */
  else {
    handleResponse(client, "404 Not Found");
  }

  // free memory
  delete route;
  delete requestMethod;

}

delay(3000); // Wait for 3 seconds before starting to listen again
}
