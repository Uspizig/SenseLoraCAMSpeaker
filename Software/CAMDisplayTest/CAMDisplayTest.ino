/*
- Funktioniert nur wenn SpeicherKarte gesteckt
- Nimmt eine Audio Aufnahme auf
- Schiesst Bilder Speichert auf SD Karte

*/

/*
 * This code was adapted from the original, developed by Seeed Studio: 
 * https://wiki.seeedstudio.com/xiao_esp32s3_camera_usage/#project-i-making-a-handheld-camera
 * 
 * Important changes:
 *  - It is not necessary to to cut off J3 on the XIAO ESP32S3 Sense expansion board. 
 *  It is possible to use the XIAO's SD Card Reader. For that you need use SD_CS_PIN as 21.
 * 
 * - The camera buffer data should be captured as RGB565 (raw image) with a 240x240 frame size  
 * to be displayed on the round display. This raw image should be converted to jpeg before save 
 * in the SD card. This can be done with the line: 
 * esp_err_t ret = frame2jpg(fb, 12, &out_buf, &out_len);
 * 
 * - The XCLK_FREQ_HZ should be reduced from 20KHz to 10KHz in order to prevent 
 * the message "no EV-VSYNC-OVF message" that appears on Serial Monitor (probably due the time 
 * added for JPEG conversion. 
 * 
 * Adapted by MRovai @02June23
 * 
 * 
*/
//Features
#define AUDIORECORDER 1
#define BUTTONENABLED 1


#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include "esp_camera.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "USB.h"

USBCDC USBSerial;
#define CAMERA_MODEL_XIAO_ESP32S3 // Has PSRAM

#ifdef AUDIORECORDER
  #include <I2S.h>
  #define RECORD_TIME   20  // seconds, The maximum value is 240
  #define WAV_FILE_NAME "arduino_rec"

// do not change for best
  #define SAMPLE_RATE 16000U
  #define SAMPLE_BITS 16
  #define WAV_HEADER_SIZE 44
  #define VOLUME_GAIN 2
#endif

#ifdef BUTTONENABLED
  #define TOUCH_INT 6 //Version 1 SenseCAM Board
#endif
/* 
 * NOTE: Since the XIAO EPS32S3 Sense is designed with three pull-up resistors 
 * R4~R6 connected to the SD card slot, and the round display also has 
 * pull-up resistors, the Round Display SD card cannot be read when both 
 * are used at the same time. To solve this problem, we need to cut off 
 * J3 on the XIAO ESP32S3 Sense expansion board.
 * https://wiki.seeedstudio.com/xiao_esp32s3_camera_usage/#preliminary-preparation
 */

//#define SD_CS_PIN 3 // ESP32S3 Sense SD Card Reader
#define SD_CS_PIN 21 // ESP32S3 Sense SD Card Reader
//#define SD_CS_PIN D2 // XIAO Round Display SD Card Reader


#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39

#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15
#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13

#define LED_GPIO_NUM      21
#define LORACS_GPIO_NUM      3

// Width and height of round display
const int camera_width = 240;
const int camera_height = 240;

// File Counter
int imageCount = 1;
bool camera_sign = false;          // Check camera status
bool sd_sign = false;              // Check sd status

TFT_eSPI tft = TFT_eSPI();


void setup() {
  // put your setup code here, to run once:
  USBSerial.begin(115200);USB.begin();
  Serial.begin(115200);
  delay(5000);
  pinMode(LORACS_GPIO_NUM, OUTPUT);
  digitalWrite(LORACS_GPIO_NUM, 1); // sonst startet der Kartenleser nicht. Irgendwas blockiert da auf dem SPI BUS
  #ifdef BUTTONENABLED
    pinMode(TOUCH_INT, INPUT_PULLDOWN);
  #endif
  //Serial.begin(115200);
//  while(!Serial);
  
  // Camera pinout
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000; // Reduced XCLK_FREQ_HZ from 20KHz to 10KHz (no EV-VSYNC-OVF message)
  //config.frame_size = FRAMESIZE_UXGA;
  //config.frame_size = FRAMESIZE_SVGA;
  config.frame_size = FRAMESIZE_240X240;
  //config.pixel_format = PIXFORMAT_JPEG; // for streaming
  config.pixel_format = PIXFORMAT_RGB565;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(config.pixel_format == PIXFORMAT_JPEG){
    if(psramFound()){
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    USBSerial.printf("Camera init failed with error 0x%x", err);
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  USBSerial.println("Camera ready");
  Serial.println("Camera ready");
  camera_sign = true; // Camera initialization check passes

  // Display initialization
  //tft.init(); //USPIZIG
  //tft.setRotation(1); //USPIZIG
  //tft.fillScreen(TFT_WHITE); //USPIZIG

  // Initialize SD card - Card Reader under the camera (21)
  // Change to D2 for using Display Card Reader
  if(!SD.begin(SD_CS_PIN)){
    USBSerial.println("Card Mount Failed");
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  // Determine if the type of SD card is available
  if(cardType == CARD_NONE){
    USBSerial.println("No SD card attached");
    Serial.println("No SD card attached");
    return;
  }

  USBSerial.print("SD Card Type: ");
  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
    USBSerial.println("MMC");
    Serial.println("MMC");
  } else if(cardType == CARD_SD){
    USBSerial.println("SDSC");
    Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
    USBSerial.println("SDHC");
    Serial.println("SDHC");
  } else {
    USBSerial.println("UNKNOWN");
    Serial.println("UNKNOWN");
  }

  sd_sign = true; // sd initialization check passes
  
  #ifdef AUDIORECORDER
    I2S.setAllPins(-1, 42, 41, -1, -1);
    if (!I2S.begin(PDM_MONO_MODE, SAMPLE_RATE, SAMPLE_BITS)) {
      Serial.println("Failed to initialize I2S!");
      USBSerial.println("Failed to initialize I2S for Audio Recorder!");
    }
    USBSerial.println("Starte den AudioRecorder");
    record_wav();
  #endif
}

void loop() {
  
  #ifdef BUTTONENABLED
    if(display_is_pressed()){
      saveimagetoSD();
    }
  #endif
    delay(10);
    //delay(3000);
  
}


// SD card write file
void writeFile(fs::FS &fs, const char * path, uint8_t * data, size_t len){
    USBSerial.printf("Writing file: %s\n", path);
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        USBSerial.println("Failed to open file for writing");
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.write(data, len) == len){
        USBSerial.println("File written");
        Serial.println("File written");
    } else {
        USBSerial.println("Write failed");
        Serial.println("Write failed");
    }
    file.close();
}
#ifdef BUTTONENABLED
  bool display_is_pressed(void)
  {
      if(digitalRead(TOUCH_INT) != HIGH) {
          delay(3);
          if(digitalRead(TOUCH_INT) != HIGH)
          return false;
          USBSerial.println("Button Press detected");
      }
      return true;
  }

  void saveimagetoSD(void){
       if( sd_sign && camera_sign){

          // Take a photo
          camera_fb_t *fb = esp_camera_fb_get();
          if (!fb) {
            USBSerial.println("Failed to get camera frame buffer");
            Serial.println("Failed to get camera frame buffer");
            return;
          }
          USBSerial.println("Button is touched");
          Serial.println("Button is touched");
          char filename[32];
          sprintf(filename, "/image%d.jpg", imageCount);
          
          // Save photo to file
          size_t out_len = 0;
          uint8_t* out_buf = NULL;
          esp_err_t ret = frame2jpg(fb, 12, &out_buf, &out_len);
          if (ret == false) {
            USBSerial.printf("JPEG conversion failed");
            Serial.printf("JPEG conversion failed");
          } else {
            // Save photo to file
            writeFile(SD, filename, out_buf, out_len);
            USBSerial.printf("Saved picture: %s\n", filename);
            Serial.printf("Saved picture: %s\n", filename);
            imageCount++;
            free(out_buf);
          }
          //  images
          uint8_t* buf = fb->buf;
          uint32_t len = fb->len;
          //tft.startWrite(); //uspizig
          //tft.setAddrWindow(0, 0, camera_width, camera_height);//uspizig
          //tft.pushColors(buf, len); //uspizig
          //tft.endWrite(); //uspizig
            
          // Release image buffer
          esp_camera_fb_return(fb);
          
      }
  } 
#endif  


#ifdef AUDIORECORDER
      void record_wav()
      {
        uint32_t sample_size = 0;
        uint32_t record_size = (SAMPLE_RATE * SAMPLE_BITS / 8) * RECORD_TIME;
        uint8_t *rec_buffer = NULL;
        Serial.printf("Ready to start recording ...\n");
        USBSerial.printf("Ready to start recording ...\n");

        File file = SD.open("/"WAV_FILE_NAME".wav", FILE_WRITE);
        // Write the header to the WAV file
        uint8_t wav_header[WAV_HEADER_SIZE];
        generate_wav_header(wav_header, record_size, SAMPLE_RATE);
        file.write(wav_header, WAV_HEADER_SIZE);

        // PSRAM malloc for recording
        rec_buffer = (uint8_t *)ps_malloc(record_size);
        if (rec_buffer == NULL) {
          Serial.printf("malloc failed!\n");
          USBSerial.printf("malloc failed!\n");
          while(1) ;
        }
        Serial.printf("Buffer: %d bytes\n", ESP.getPsramSize() - ESP.getFreePsram());
        USBSerial.printf("Buffer: %d bytes\n", ESP.getPsramSize() - ESP.getFreePsram());

        // Start recording
        esp_i2s::i2s_read(esp_i2s::I2S_NUM_0, rec_buffer, record_size, &sample_size, portMAX_DELAY);
        if (sample_size == 0) {
          Serial.printf("Record Failed!\n");
          USBSerial.printf("Record Failed!\n");
        } else {
          Serial.printf("Record %d bytes\n", sample_size);
          USBSerial.printf("Record %d bytes\n", sample_size);
        }

        // Increase volume
        for (uint32_t i = 0; i < sample_size; i += SAMPLE_BITS/8) {
          (*(uint16_t *)(rec_buffer+i)) <<= VOLUME_GAIN;
        }

        // Write data to the WAV file
        Serial.printf("Writing to the file ...\n");
        USBSerial.printf("Writing to the file ...\n");
        if (file.write(rec_buffer, record_size) != record_size)
          Serial.printf("Write file Failed!\n");
          USBSerial.printf("Write file Failed!\n");

        free(rec_buffer);
        file.close();
        Serial.printf("The recording is over.\n");
        USBSerial.printf("The recording is over.\n");
      }

      void generate_wav_header(uint8_t *wav_header, uint32_t wav_size, uint32_t sample_rate)
      {
        // See this for reference: http://soundfile.sapp.org/doc/WaveFormat/
        uint32_t file_size = wav_size + WAV_HEADER_SIZE - 8;
        uint32_t byte_rate = SAMPLE_RATE * SAMPLE_BITS / 8;
        const uint8_t set_wav_header[] = {
          'R', 'I', 'F', 'F', // ChunkID
          file_size, file_size >> 8, file_size >> 16, file_size >> 24, // ChunkSize
          'W', 'A', 'V', 'E', // Format
          'f', 'm', 't', ' ', // Subchunk1ID
          0x10, 0x00, 0x00, 0x00, // Subchunk1Size (16 for PCM)
          0x01, 0x00, // AudioFormat (1 for PCM)
          0x01, 0x00, // NumChannels (1 channel)
          sample_rate, sample_rate >> 8, sample_rate >> 16, sample_rate >> 24, // SampleRate
          byte_rate, byte_rate >> 8, byte_rate >> 16, byte_rate >> 24, // ByteRate
          0x02, 0x00, // BlockAlign
          0x10, 0x00, // BitsPerSample (16 bits)
          'd', 'a', 't', 'a', // Subchunk2ID
          wav_size, wav_size >> 8, wav_size >> 16, wav_size >> 24, // Subchunk2Size
        };
        memcpy(wav_header, set_wav_header, sizeof(set_wav_header));
      }
#endif