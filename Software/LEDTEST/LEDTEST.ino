// NeoPixel Ring simple sketch (c) 2013 Shae Erisson
// Released under the GPLv3 license to match the rest of the
// Adafruit NeoPixel library

#include <Adafruit_NeoPixel.h>
#include "USB.h"
USBCDC USBSerial;

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN        15 
#define NUMPIXELS 1 

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#define DELAYVAL 1500 // Time (in milliseconds) to pause between pixels

void setup() {
  USBSerial.begin(115200);USB.begin();
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  USBSerial.println("INIT");
}

void loop() {
  pixels.clear(); // Set all pixel colors to 'off'

  // The first NeoPixel in a strand is #0, second is 1, all the way up
  // to the count of pixels minus one.
  for(int i=0; i<NUMPIXELS; i++) { // For each pixel...

    // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
    // Here we're using a moderately bright green color:
    pixels.setPixelColor(i, pixels.Color(0, 150, 0));

    pixels.show();   // Send the updated pixel colors to the hardware.

    delay(DELAYVAL); // Pause before next pass through loop
    USBSerial.println("LOOP");
  }
}
