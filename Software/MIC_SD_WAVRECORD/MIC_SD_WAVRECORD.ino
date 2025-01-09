/* Ungetestet...vllt I2s Port 1 nehmen statt 0
FUNKTIONIERT NICHT MALLOC FAILED i2S Driver install error

 https://forum.seeedstudio.com/t/xiao-esp32s3-sense-arduino-i2s-mic-init-question/271251/3
 * DI2S Version, 
 * WAV Recorder for Seeed XIAO ESP32S3 Sense
 * 
 * Original: https://wiki.seeedstudio.com/xiao_esp32s3_sense_mic/
 */

#include "driver/i2s.h"
#include "Arduino.h"
#include "WiFi.h"
#include "Audio.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "USB.h"

// make changes as needed
#define RECORD_TIME   20  // seconds, The maximum value is 240
#define WAV_FILE_NAME "arduino_rec_DI2S"
#define I2S_DOUT      4
#define I2S_BCLK      1
#define I2S_LRC       2
#define MIC_CLK       42
#define MIC_DAT       41
#define PIN_LMIC_NSS  3
#define SD_CS         21

// do not change for best
#define SAMPLE_RATE 16000U
#define SAMPLE_BITS 16
#define WAV_HEADER_SIZE 44
#define VOLUME_GAIN 2


Audio audio;
USBCDC USBSerial;

String ssid =     "wallyweb2";
String password = "FS4burgberg";

void setup() {
  
  pinMode(PIN_LMIC_NSS, OUTPUT); digitalWrite(PIN_LMIC_NSS, HIGH); // sonst startet der Kartenleser nicht. Irgendwas blockiert da auf dem SPI BUS
  pinMode(SD_CS, OUTPUT);      digitalWrite(SD_CS, HIGH);
  
  USBSerial.begin(115200);USB.begin();
  //Serial.begin(115200);
  delay(2000); 
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
  //while (WiFi.status() != WL_CONNECTED){USBSerial.println("."); delay(1500);} 

  i2s_init();
  
  if(!SD.begin(SD_CS)){
    USBSerial.println("Failed to mount SD Card!");
    while (1) ;
  }
  record_wav();
  audio_setup();
  //i2s_deinit();
  

}

void loop() {
  audio.loop();
  delay(1000);
  USBSerial.printf(".");
}

void record_wav() {
  //delay(2000); 
  
  size_t  sample_size = 0;
  //uint32_t sample_size = 0;
  uint32_t record_size = (SAMPLE_RATE * SAMPLE_BITS / 8) * RECORD_TIME;
  uint8_t *rec_buffer = NULL;
  USBSerial.printf("Ready to start recording ...\n");

  File file = SD.open("/"WAV_FILE_NAME".wav", FILE_WRITE);
  // Write the header to the WAV file
  uint8_t wav_header[WAV_HEADER_SIZE];
  generate_wav_header(wav_header, record_size, SAMPLE_RATE);
  file.write(wav_header, WAV_HEADER_SIZE);

  // PSRAM malloc for recording
  rec_buffer = (uint8_t *)ps_malloc(record_size);
  if (rec_buffer == NULL) {
    USBSerial.printf("malloc failed!\n");
    while(1) ;
  }
  USBSerial.printf("Buffer: %d bytes\n", ESP.getPsramSize() - ESP.getFreePsram());

  // Start recording
  i2s_read((i2s_port_t)0, (void*)rec_buffer, record_size, &sample_size, portMAX_DELAY);
  
  if (sample_size == 0) {
    USBSerial.printf("Record Failed!\n");
  } else {
    USBSerial.printf("Record %d bytes\n", sample_size);
  }

  // Increase volume
  for (uint32_t i = 0; i < sample_size; i += SAMPLE_BITS/8) {
    (*(uint16_t *)(rec_buffer+i)) <<= VOLUME_GAIN;
  }

  // Write data to the WAV file
  USBSerial.printf("Writing to the file ...\n");
  if (file.write(rec_buffer, record_size) != record_size)
    USBSerial.printf("Write file Failed!\n");

  free(rec_buffer);
  file.close();
  USBSerial.printf("The recording is over.\n");
}

void generate_wav_header(uint8_t *wav_header, uint32_t wav_size, uint32_t sample_rate) {
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

static int i2s_init() {
  // Start listening for audio: MONO @ 16/16KHz
  i2s_config_t i2s_config = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM ),
    .sample_rate          = 16000U,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
    //.channel_format       = I2S_CHANNEL_FMT_ONLY_RIGHT,     // Also works
    .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
    //.communication_format = I2S_COMM_FORMAT_PCM,            // Also works
    .communication_format = I2S_COMM_FORMAT_I2S,   
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 8,
    .dma_buf_len          = 512,    
    .use_apll             = false,
    .tx_desc_auto_clear   = false,
    .fixed_mclk           = 0
  };
  
  i2s_pin_config_t pin_config = {
      .bck_io_num = -1,    // IIS_SCLK
      .ws_io_num = MIC_CLK,     // IIS_LCLK
      .data_out_num = -1,  // IIS_DSIN
      .data_in_num = MIC_DAT,   // IIS_DOUT
  
  };
  esp_err_t ret = 0;

  ret = i2s_driver_install((i2s_port_t)0, &i2s_config, 0, NULL);
  if (ret != ESP_OK) {
    USBSerial.printf("Error in i2s_driver_install");
  }

  ret = i2s_set_pin((i2s_port_t)0, &pin_config);
  if (ret != ESP_OK) {
    USBSerial.printf("Error in i2s_set_pin");
  }

  ret = i2s_zero_dma_buffer((i2s_port_t)0);
  if (ret != ESP_OK) {
    USBSerial.printf("Error in initializing dma buffer with 0");
  }

  return int(ret);
}

static int i2s_deinit(void) {
    i2s_driver_uninstall((i2s_port_t)0); //stop & destroy i2s driver
    return 0;
}


void audio_setup(void){
	audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
	audio.setVolume(8); // default 0...21
}


bool audio_play(int number){
  //audio.connecttoFS(SD, "/test4.wav");     // SD
  //audio.connecttohost("http://stream.antennethueringen.de/live/aac-64/stream.antennethueringen.de/"); // aac
  audio.connecttoFS(SD, "/test4.wav");     // SD //RICHTIG
	return 1;
}


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

