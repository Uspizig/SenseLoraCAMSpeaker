#include "Arduino.h"
#include "WiFi.h"
#include "Audio.h"
#include "SD.h"
#include "FS.h"
#include "USB.h"
#include <Adafruit_NeoPixel.h>

// Digital I/O used
#define SD_CS         21
#define SPI_MOSI      9
#define SPI_MISO      8
#define SPI_SCK       7
#define I2S_DOUT      4
#define I2S_BCLK      1
#define I2S_LRC       2
#define BUTTON        6
#define PIN_LMIC_NSS  3

#define PIN        43//2 // On Trinket or Gemma, suggest changing this to 1  PIN 2 bei Platine V1 PIN 43 bei JP1 gebrückt
#define NUMPIXELS 1 // Popular NeoPixel ring size
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
#define DELAYVAL 500 // Time (in milliseconds) to pause between pixels

Audio audio;
USBCDC USBSerial;
String ssid =     "xxxxxx";
String password = "xxxxx";
String hostname = "ESP32 Node Temperature";

int buttonState = 0;  // variable for reading the pushbutton status

void setup() {
    delay(2000); 
    USBSerial.begin(115200);USB.begin();
    delay(2000); 
    USBSerial.println("BOOT UP");
    pinMode(BUTTON, INPUT_PULLDOWN);
  
  
    pinMode(PIN_LMIC_NSS, OUTPUT); digitalWrite(PIN_LMIC_NSS, HIGH); // sonst startet der Kartenleser nicht. Irgendwas blockiert da auf dem SPI BUS
    pinMode(SD_CS, OUTPUT);      digitalWrite(SD_CS, HIGH);
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    
    SD.begin(SD_CS);
    USBSerial.println("SD BEGIN");
    WiFi.disconnect();
    delay(2000); 
    WiFi.mode(WIFI_STA);
    delay(2000); 
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    //WiFi.setHostname(hostname.c_str()); //define hostname
    WiFi.begin(ssid.c_str(), password.c_str());
    USBSerial.println("WIFI START");
    //while (WiFi.status() != WL_CONNECTED){USBSerial.println("."); delay(1500);} 
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(8); // default 0...21
    pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
    pixels.clear(); // Set all pixel colors to 'off'
    for(int i=0; i<NUMPIXELS; i++) { // For each pixel...
        // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
        // Here we're using a moderately bright green color:
        pixels.setPixelColor(i, pixels.Color(0, 10, 0));
        pixels.show();   // Send the updated pixel colors to the hardware.
        delay(DELAYVAL); // Pause before next pass through loop
        USBSerial.println("Booting");
  }
  USBSerial.println("LEDON");
//  or alternative
//  audio.setVolumeSteps(64); // max 255
//  audio.setVolume(63);    
//
//  *** radio streams ***
//    audio.connecttohost("http://stream.antennethueringen.de/live/aac-64/stream.antennethueringen.de/"); // aac
//  audio.connecttohost("http://mcrscast.mcr.iol.pt/cidadefm");                                         // mp3
//  audio.connecttohost("http://www.wdr.de/wdrlive/media/einslive.m3u");                                // m3u
//  audio.connecttohost("https://stream.srg-ssr.ch/rsp/aacp_48.asx");                                   // asx
//  audio.connecttohost("http://tuner.classical102.com/listen.pls");                                    // pls
//  audio.connecttohost("http://stream.radioparadise.com/flac");                                        // flac
//  audio.connecttohost("http://stream.sing-sing-bis.org:8000/singsingFlac");                           // flac (ogg)
//  audio.connecttohost("http://s1.knixx.fm:5347/dein_webradio_vbr.opus");                              // opus (ogg)
//  audio.connecttohost("http://stream2.dancewave.online:8080/dance.ogg");                              // vorbis (ogg)
//  audio.connecttohost("http://26373.live.streamtheworld.com:3690/XHQQ_FMAAC/HLSTS/playlist.m3u8");    // HLS
//  audio.connecttohost("http://eldoradolive02.akamaized.net/hls/live/2043453/eldorado/master.m3u8");   // HLS (ts)
//  *** web files ***
//  audio.connecttohost("https://github.com/schreibfaul1/ESP32-audioI2S/raw/master/additional_info/Testfiles/Pink-Panther.wav");        // wav
//  audio.connecttohost("https://github.com/schreibfaul1/ESP32-audioI2S/raw/master/additional_info/Testfiles/Santiano-Wellerman.flac"); // flac
//  audio.connecttohost("https://github.com/schreibfaul1/ESP32-audioI2S/raw/master/additional_info/Testfiles/Olsen-Banden.mp3");        // mp3
//  audio.connecttohost("https://github.com/schreibfaul1/ESP32-audioI2S/raw/master/additional_info/Testfiles/Miss-Marple.m4a");         // m4a (aac)
//  audio.connecttohost("https://github.com/schreibfaul1/ESP32-audioI2S/raw/master/additional_info/Testfiles/Collide.ogg");             // vorbis
//  audio.connecttohost("https://github.com/schreibfaul1/ESP32-audioI2S/raw/master/additional_info/Testfiles/sample.opus");             // opus
//  *** local files ***
  audio.connecttoFS(SD, "/test4.wav");     // SD //RICHTIG
//  audio.connecttoFS(SD_MMC, "/test4.wav"); // SD_MMC //FALSCH
//  audio.connecttoFS(SPIFFS, "/test.wav"); // SPIFFS

//  audio.connecttospeech("Wenn die Hunde schlafen, kann der Wolf gut Schafe stehlen.", "de"); // Google TTS
USBSerial.print("AudioActive");
}

void loop()
{
    audio.loop();
    buttonState = digitalRead(BUTTON);

  // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
  if (buttonState == HIGH) {
    // turn LED BLUE:
    pixels.setPixelColor(0, pixels.Color(0, 0, 10));
  } else {
    // turn LED RED:
    pixels.setPixelColor(0, pixels.Color(10, 0, 0));
  }
  pixels.show();   // Send the updated pixel colors to the hardware.
}

// optional
void audio_info(const char *info){
    USBSerial.print("info        "); USBSerial.println(info);
}
void audio_id3data(const char *info){  //id3 metadata
    USBSerial.print("id3data     ");USBSerial.println(info);
}
void audio_eof_mp3(const char *info){  //end of file
    USBSerial.print("eof_mp3     ");USBSerial.println(info);
}
void audio_showstation(const char *info){
    USBSerial.print("station     ");USBSerial.println(info);
}
void audio_showstreamtitle(const char *info){
    USBSerial.print("streamtitle ");USBSerial.println(info);
}
void audio_bitrate(const char *info){
    USBSerial.print("bitrate     ");USBSerial.println(info);
}
void audio_commercial(const char *info){  //duration in sec
    USBSerial.print("commercial  ");USBSerial.println(info);
}
void audio_icyurl(const char *info){  //homepage
    USBSerial.print("icyurl      ");USBSerial.println(info);
}
void audio_lasthost(const char *info){  //stream URL played
    USBSerial.print("lasthost    ");USBSerial.println(info);
}
void audio_eof_speech(const char *info){
    USBSerial.print("eof_speech  ");USBSerial.println(info);
}
