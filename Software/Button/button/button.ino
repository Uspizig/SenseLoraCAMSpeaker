#include "Arduino.h"
#include <Adafruit_NeoPixel.h>
#include "USB.h"
const int BUTTON1 = 3;
const int BUTTON2 = 13;
#define PIN        5 // On Trinket or Gemma, suggest changing this to 1#define NUMPIXELS 4 // Popular NeoPixel ring size
#define NUMPIXELS  4 // Popular NeoPixel ring size
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
#define DELAYVAL 1000 // Time (in milliseconds) to pause between pixels

USBCDC USBSerial;

int buttonState = 0;  // variable for reading the pushbutton status
int buttonState2 = 0;  // variable for reading the pushbutton status

void setup() {
  delay(2000);
  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);
  USBSerial.begin();USB.begin();
  Serial.begin(115200);
  /*pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.clear(); // Set all pixel colors to 'off'
  for(int i=0; i<NUMPIXELS; i++) { // For each pixel...
        // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
        // Here we're using a moderately bright green color:
        pixels.setPixelColor(i, pixels.Color(0, 10, 0));
        pixels.show();   // Send the updated pixel colors to the hardware.
        delay(DELAYVAL); // Pause before next pass through loop
        USBSerial.println("Booting");
  }*/
  USBSerial.println("SETUP finished");
  Serial.println("SETUP finished");

}

void loop() {
  //buttonState = analogRead(BUTTON1);
  buttonState = digitalRead(BUTTON1);
  buttonState2 = digitalRead(BUTTON2);
  USBSerial.print(buttonState);
  USBSerial.println(buttonState2);
  Serial.print(buttonState);Serial.println("nixUSB");
  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  /*if (buttonState == 1) {
    // turn LED BLUE:
    pixels.setPixelColor(1, pixels.Color(0, 0, 50));
    
  } else {
    // turn LED RED:
    pixels.setPixelColor(1, pixels.Color(50, 0, 0));
  }
  pixels.show();   // Send the updated pixel colors to the hardware.*/
  delay(500);
}
