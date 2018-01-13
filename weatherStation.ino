
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#include <Ethernet.h>
#include <SdFat.h>
#include <DS1302.h>

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(10,0,0,111);
EthernetServer server(80);

// ATTN pin 10 reserved to ethernet trx
//#define CSN_PIN 8

RF24 radio(7,8);

const int chipSelect = 4;
const int ledPin=A0;

SdFat sd;

// Init the DS1302
// DS1302: RST / CE pin -> Arduino Digital 3
// I/O / DAT pin -> Arduino Digital 5
// SCLK pin -> Arduino Digital 6
//DS1302(ce, data, clock);
DS1302 rtc(3,5,6);

Time t(2014, 2, 15, 17, 38, 00, Time::kSaturday);

void setup(){
  pinMode(ledPin, OUTPUT);
  pinMode(chipSelect, OUTPUT);
  //digitalWrite(ledPin, LOW); 
  Serial.begin(57600);
  printf_begin();

  radio.begin();
  radio.enableDynamicPayloads();
  radio.setAutoAck(1);
  radio.setRetries(15,15);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.setChannel(76);
  radio.openReadingPipe(1,0xF0F0F0F0E1LL);
  radio.startListening();

  radio.printDetails();

  // Initialize SdFat or print a detailed error message and halt
  // Use half speed like the native library.
  // change to SPI_FULL_SPEED for more performance.
  if (!sd.begin(chipSelect, SPI_HALF_SPEED)) sd.initErrorHalt();

  if (!sd.chdir("/data",false)) {
    sd.errorHalt("move to data folder failed");
  }


  Ethernet.begin(mac,ip);
  server.begin();

  //rtc.halt(false);
  //rtc.writeProtect(false);
  //Serial.println("go!");

  //set_time(); //do once!!
  //rtc.time(t);
  //rtc.writeProtect(true);

  printTime();

  blink(5); 

}

char* printTime() {
  // Get the current time and date from the chip.
  Time t = rtc.time();

  // Name the day of the week.
  //const String day = dayAsString(t.day);

  // Format the time and date and insert into the temporary buffer.
  char buf[20];
  snprintf(buf, sizeof(buf), "%d,%d,%d,%d,%d,%d",
  t.yr,t.mon-1, t.date, 
  t.hr, t.min, t.sec);

  // Print the formatted string to serial so we can see the time.
  Serial.println(buf);

  return buf;
}

void blink(int n) {
  for(int i=0;i<n;i++){
    digitalWrite(ledPin, HIGH); 
    delay(100);
    digitalWrite(ledPin, LOW); 
    delay(100);

  }
}

void sdCard(int p){
      digitalWrite(chipSelect, p);
};

void loop(){
  char data[32]="";

  digitalWrite(ledPin, LOW); 

  EthernetClient client = server.available();

  if(client) {
  
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {   // client data available to read
        blink(2);

        char c = client.read(); // read 1 byte (character) from client
        // last line of client request is blank and ends with \n
        // respond to client only after last line received
        if (c == '\n' && currentLineIsBlank) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");
          client.println();
          // send web page
          SdFile webFile;
          if (!webFile.open("index.html", O_RDONLY)) {
            sd.errorHalt("opening index.html");
          } 
          else {
            while(webFile.available()) {
              client.write(webFile.read()); // send web page to client
            }
            webFile.close();
          }
          break;
        }
        // every line of text received from the client ends with \r\n
        if (c == '\n') {
          // last character on line of received text
          // starting new line with next character read
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          // a text character was received from client
          currentLineIsBlank = false;
        }
      } // end if (client.available())

    } // end while (client.connected())
    delay(1);      // give the web browser time to receive the data
    client.stop(); // close the connection

  } 
  else {
    if ( radio.available() ){ 
      blink(3);
      // Dump the payloads until we've gotten everything
      bool done = false;
      int len = radio.getDynamicPayloadSize();

      while(!done){
        done = radio.read(&data, len);
        printf("data received %s\n\r",data);
        delay(20);
      }

      SdFile myFile;
      
      sdCard(LOW);
      delay(100);


      // open the file for write at end like the Native SD library
      if (!myFile.open("sensor0.csv", O_RDWR | O_CREAT | O_AT_END)) {
        sd.errorHalt("opening sensor0.csv for write failed");
      }
      // if the file opened okay, write to it:

      char line[52]="";
      strcat(line,printTime());
      strcat(line, ",");
      strcat(line,data);

      //printf(line);
      printf("Writing to sensor0.csv...%s\n",line);

      myFile.println(line);

      // close the file:
      myFile.close();
      Serial.println("done.");
      sdCard(HIGH);
      
      delay(100);
    }
  }
}






