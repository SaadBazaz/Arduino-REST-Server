#include <Ethernet.h>
#include <SPI.h>
#include <SD.h>

#define MAXLENGTH_FIRSTLINE 30

void handleResponse(EthernetClient& client, char* status_code, char* message){
    if(client.connected()) {
      client.print("HTTP/1.1 ");
      client.print(status_code);
      client.println("\nContent-Type: text/html\n");
      client.println(message);
    }
    client.stop();
    Serial.print("\nClient disconnected with ");
    Serial.print(status_code);
}

// Mac Address -> 00:aa:bb:cc:de:06
byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x06};

// Port -> 12345
EthernetServer server = EthernetServer(12345);

File webFile;


void setup() {
  
  Serial.begin(9600);
  boolean receiving = false;

    
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

  // Making a trigger pin
  pinMode(13, OUTPUT);    // sets the digital pin 8 as output
  digitalWrite(13, LOW);  // sets the digital pin 8 off
  Serial.println("Set pin 13 as output");

  
}

void loop() {
  
  Serial.println(".");
  EthernetClient client = server.available();

if(client) {

  char* route = "";
  char* requestMethod = "";
  char information [15];
  int information_letter_count = 0;

  int wordCount = 0;
  int letterCount = 0;
  int lineCount = 0;
  
  while(client.available()) {             
    
      char c = client.read();


      // Data in the first line is important to determine which route we'll take, and how
      if (c == ' '){
        information[information_letter_count] = '\0';

        if (wordCount == 0){
                requestMethod = new char [information_letter_count+1]; 
                for (int i=0; i<information_letter_count; i++)
                  requestMethod[i] = information [i];
                requestMethod [information_letter_count] = '\0'; 
        }
        else if (wordCount == 1){
                route = new char [information_letter_count+1]; 
                for (int i=0; i<information_letter_count; i++)
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
        handleResponse (client, "400 Bad Request", "The request's first line was too large.");
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


  // @route: HIGH
  // Turns the pin 13 high
  // Returns a statement (text/html)
  if (strncmp("H/", route, 2) == 0){
      Serial.println("\nActivating pin 13...");
    // trigger the pin 13
      digitalWrite(13, HIGH);                   // sets the digital pin 13 on

    if(client.connected()){
      handleResponse(client, "200 OK", "Pin 13 activated");      
    }
  }

  // @route: PULSE
  // Pulses the pin 13 for 500ms
  // Returns a statement (text/html)
  else if (strncmp("P/", route, 2) == 0){
    Serial.println("\nPulsing pin 13...");
  // trigger the pin 13
    digitalWrite(13, HIGH);                   // sets the digital pin 13 on
    delay(500);                               // waits for a second
    digitalWrite(13, LOW);                    // sets the digital pin 13 off

    if(client.connected()){
      handleResponse(client, "200 OK", "Pin 13 pulsed");      
    }
  }

  // @route: LOW
  // Turns the pin 13 low
  // Returns a statement (text/html)
  else if (strncmp("L/", route, 2) == 0){
    Serial.println("\nDeactivating pin 13...");
    digitalWrite(13, LOW);                    // sets the digital pin 133 off

    if(client.connected()){
      handleResponse(client, "200 OK", "Pin 13 turned off");      
    }
  }

  // @route: LOGIN
  // Serves a login page from the SD card
  // Returns a login page (text/html)
  else if (strncmp("login", route, 5) == 0){
    if(client.connected()) {
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

  // @route: NOT_FOUND
  // If route doesn't exist above, land here
  // Returns a statement (text/html)
  else {
    handleResponse(client, "404 Not Found", "This route does not exist.");
  }

  // free memory
  delete route;
  delete requestMethod;

}

delay(3000); // Wait for 3 seconds before starting to listen again
}
