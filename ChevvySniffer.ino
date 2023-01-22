
/*
 * Written by KBeard 19/09/19
 * Code for sniffing the CAN-BUS for throttle position and when it reaches or exceeds the set value, enable a relay.
 * connections:
 * 128x32 OLED screen output on SDA,SCL pins
 * Relay on D2
 * Linear pot on A0
 */


#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Serial_CAN_Module.h>
#include <SoftwareSerial.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Serial_CAN can;

#define relayPin 4 //D4
#define potPin 3 //A3
#define can_tx  5           // tx of serial can module connect to D5
#define can_rx  6           // rx of serial can module connect to D6

#define CAN_ID_PID          0x7DF

int setVal = 80;
int actVal = 50;
bool curr_status;
bool prev_status;

unsigned long mask[4] = 
{
    0, 0x7FC,                // ext, maks 0
    0, 0x7FC,                // ext, mask 1
};

unsigned long filt[12] = 
{
    0, 0x7E8,                // ext, filt 0
    0, 0x7E8,                // ext, filt 1
    0, 0x7E8,                // ext, filt 2
    0, 0x7E8,                // ext, filt 3
    0, 0x7E8,                // ext, filt 4
    0, 0x7E8,                // ext, filt 5
};

void setup() {
  Serial.begin(115200);
  pinMode(relayPin, OUTPUT);
  
  can.begin(can_tx, can_rx, 9600);      // tx, rx
  set_mask_filt();

  dispSetup();
  
  Serial.println("begin");
}

void dispSetup() {
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  //display.display();
  display.setRotation(2);
  //delay(2000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();
}

void loop() {

    sendPid(73);//19.22-81.18 //pedal pos
//    sendPid(74);//9.41-40.39
//    sendPid(75);//nothing
//    sendPid(76);//not linear // thottle pos
    taskCanRecv();
//    taskDbg();

    setVal = map(analogRead(potPin),0,1023,0,100);
    //Serial.println(setVal);
    dispVal();
  
    delay(100);
}

void sendPid(unsigned char __pid)
{
    unsigned char tmp[8] = {0x02, 0x01, __pid, 0, 0, 0, 0, 0};
    Serial.print("SEND PID: 0x");
    Serial.println(__pid, HEX);
    can.send(CAN_ID_PID, 0, 0, 8, tmp);   // SEND TO ID:0X55
}

void taskCanRecv()
{
    unsigned char len = 8;
    unsigned long id  = 0;
    unsigned char buf[8];

    if(can.recv(&id, buf))                   // check if get data
    {
//        Serial.println("\r\n------------------------------------------------------------------");
//        Serial.print("Get Data From id: 0x");
//        Serial.println(id, HEX);
//        for(int i = 0; i<len; i++)          // print the data
//        {
//            Serial.print("0x");
//            Serial.print(buf[i], HEX);
//            Serial.print("\t");
//        }
//        Serial.println();
        Serial.print("HEX:");
        Serial.println(buf[3], HEX);
        Serial.print("DEC:");
        Serial.println(buf[3], DEC);
        actVal = String(buf[3],DEC).toInt() * 0.392156862745098;//100/255
//        Serial.println(buf[3], HEX);
        Serial.print("PCT:");
        Serial.println(actVal);
//        Serial.println(throttlePos.toInt() * 0.392156862745098);
        // * 0.392156862745098
    }
}

void dispVal(void) {
    display.clearDisplay();

    display.setTextSize(1);      // Normal 1:1 pixel scale
    display.setTextColor(WHITE); // Draw white text
    display.cp437(true);         // Use full 256 char 'Code Page 437' font

    display.setCursor(0, 5);
    display.print(F("Set:"));
    display.setCursor(60, 5);
    display.print(setVal); 
    display.setCursor(0, 18);
    display.print(F("Act:"));
    display.setCursor(60, 18);
    display.print(actVal);

    // The INVERSE color is used so rectangles alternate white/black
    display.fillRect(24, 3, setVal, 11, INVERSE);
    display.fillRect(24, 16, actVal, 11, INVERSE);

    engage(actVal >= setVal);
    
    display.display(); // Update screen with each newly-drawn rectangle
    delay(1);

}

void engage(bool curr_status)
    {  if (curr_status != prev_status)

            if (curr_status) {
                display.invertDisplay(true);
                digitalWrite(relayPin, HIGH);
              }
            
            else {
                display.invertDisplay(false);
                digitalWrite(relayPin, LOW);
              }
          
    prev_status = curr_status;
    }

void set_mask_filt()
{
    /*
     * set mask, set both the mask to 0x3ff
     */

    if(can.setMask(mask))
    {
        Serial.println("set mask ok");
    }
    else
    {
        Serial.println("set mask fail");
    }
    
    /*
     * set filter, we can receive id from 0x04 ~ 0x09
     */
    if(can.setFilt(filt))
    {
        Serial.println("set filt ok");
    }
    else 
    {
        Serial.println("set filt fail");
    }
    
}
