
/********** Initialize Modem ********/
#include <HardwareSerial.h>
HardwareSerial SerialAT(1);

// Set Modem type
#define TINY_GSM_MODEM_SIM800

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// Increase RX buffer to capture the entire response
// Chips without internal buffering (A6/A7, ESP8266, M590)
// need enough space in the buffer for the entire response
// else data will be lost (and the http library will fail).
#if !defined(TINY_GSM_RX_BUFFER)
#define TINY_GSM_RX_BUFFER 650
#endif

// See all AT commands, if wanted
// #define DUMP_AT_COMMANDS

// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG SerialMon
// #define LOGGING  // <- Logging is for the HTTP library

// Range to attempt to autobaud
// NOTE:  DO NOT AUTOBAUD in production code.  Once you've established
// communication, set a fixed baud rate using modem.setBaud(#).
#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 115200

// Add a reception delay, if needed.
// This may be needed for a fast processor at a slow baud rate.
// #define TINY_GSM_YIELD() { delay(2); }

// Define how you're planning to connect to the internet
// These defines are only for this example; they are not needed in other code.
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false

// set GSM PIN, if any
#define GSM_PIN ""

// Your GPRS credentials, if any
const char apn[]      = "byu";
const char gprsUser[] = "";
const char gprsPass[] = "";

#include <TinyGsmClient.h>
#include <HTTPClient.h>

// Just in case someone defined the wrong thing..
#if TINY_GSM_USE_GPRS && not defined TINY_GSM_MODEM_HAS_GPRS
#undef TINY_GSM_USE_GPRS
#undef TINY_GSM_USE_WIFI
#define TINY_GSM_USE_GPRS false
#define TINY_GSM_USE_WIFI true
#endif
#if TINY_GSM_USE_WIFI && not defined TINY_GSM_MODEM_HAS_WIFI
#undef TINY_GSM_USE_GPRS
#undef TINY_GSM_USE_WIFI
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false
#endif

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm        modem(debugger);
#else
TinyGsm        modem(SerialAT);
#endif

TinyGsmClient client(modem);

/***************** Initialize camera ******************/

// Camera Model Definition
#define CAMERA_MODEL_AI_THINKER // Has PSRAM

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include "esp_camera.h"
#include "camera_pins.h"

// Deep Sleep Definitions
// #define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
// #define TIME_TO_SLEEP  300        /* Time ESP32 will go to sleep (in seconds) */

// Function Prototypes
void initCamera();
void uploadPhoto();

// Global variables
int counter = 0;

String serverName = "desprokel10.ddns.net";
String serverPath = "/upload.php";
const int serverPort = 1234;

void setup() {
  // Set console baud rate
  SerialMon.begin(115200);
  delay(10);
  SerialMon.println("Wait...");

  // Set GSM module baud rate
  SerialAT.begin(9600, SERIAL_8N1, 12, 13); //RX TX
  delay(6000);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  SerialMon.println("Initializing modem...");
  // modem.restart();
  modem.init();

  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);

  // GPRS connection parameters are usually set after network registration
  SerialMon.print(F("Connecting to "));
  SerialMon.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");
  if (modem.isGprsConnected()) { SerialMon.println("GPRS connected"); }

  delay(1000);

  // Initialize Camera
  SerialMon.println("Initializing Camera");
  initCamera();
  delay(1000);

  // Take picture and upload
  uploadPhoto();

}

void loop() {
  

}

void uploadPhoto(){
  // Take picture
  camera_fb_t * fb = NULL;
  Serial.println("Taking pictures...");
  fb = esp_camera_fb_get();
  if(!fb) {
    Serial.println("Camera capture failed");
    ESP.restart();
  }
  else{
    Serial.println("Camera capture successful!");
  }

  // Upload picture
  String getAll;
  String getBody;

  if (client.connect(serverName.c_str(), serverPort)){

    SerialMon.println("Connection successful!");    
    String head = "--ThisIsABoundary\r\nContent-Disposition: form-data; name=\"imageFile\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--ThisIsABoundary--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;

    SerialMon.println("Data byte length: " + String(totalLen));

    SerialMon.println("Sending header....");
    client.print("POST " + serverPath + " HTTP/1.1\r\n");
    client.print("Host: " + serverName + "\r\n");
    client.print("Content-Length: " + String(totalLen) + "\r\n");
    client.print("Content-Type: multipart/form-data; boundary=ThisIsABoundary\r\n");
    client.print("\r\n");
    client.print(head);

    SerialMon.println("Sending body....");
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    int packet_size = 1024;
    for (size_t n=0; n<fbLen; n=n+packet_size) {
      if (n+packet_size < fbLen) {
        client.write(fbBuf, packet_size);
        fbBuf += packet_size;
      }
      else if (fbLen%packet_size>0) {
        size_t remainder = fbLen%packet_size;
        client.write(fbBuf, remainder);
      }
      SerialMon.println("Body Sent: " + String(n));
      delay(200);
    }
    SerialMon.println("Sending tail....");
    client.print(tail);

    // Waiting for response from the php server
    int timeoutTimer = 10000;
    long startTimer = millis();
    bool state = false;
    bool feedback = false;

    SerialMon.println("\r\nWaiting for server response...");
    while((startTimer + timeoutTimer) > millis()){
      SerialMon.print(".");
      delay(100);
      while(client.available()){
        char c = client.read();
        if (c == '\n'){
          if (getAll.length()==0){
            state = true;
          }

          getAll = "";
        }

        else if (c != '\r'){
          getAll += String(c);
        }

        if (state == true){
          getBody += String(c);
        }

        startTimer = millis();
      }

      if (getBody.length() > 0){
        feedback = true;
        break;
      }

    }
    SerialMon.println();
    client.stop();
    SerialMon.println(getBody);
  }
  else{
    SerialMon.println("Unable to connect to server!");
    return;
  }

}

void initCamera(){
  // Camera setup
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
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  // config.pixel_format = PIXFORMAT_RGB565;

  config.frame_size = FRAMESIZE_SVGA;
  config.jpeg_quality = 35;
  config.fb_count = 10;
  config.grab_mode = CAMERA_GRAB_LATEST;

  SerialMon.println("Intitializing camera");
  // Init Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    SerialMon.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  s -> set_framesize(s, FRAMESIZE_SVGA);

  s->set_brightness(s, 0);     // -2 to 2
  s->set_contrast(s, 2);       // -2 to 2
  s->set_saturation(s, -2);     // -2 to 2
  s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
  s->set_whitebal(s, 1);       // 0 = disable , 1 = enable (White Balance)
  s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable (Auto White Balance Gain)
  s->set_wb_mode(s, 0);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
  s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable (Auto Exposure Control)
  s->set_aec2(s, 1);           // 0 = disable , 1 = enable (Auto Exposure Control 2)
  s->set_ae_level(s, 0);       // -2 to 2 (Auto Exposure Level if exposure ctrl is enabled)
  s->set_aec_value(s, 1200);    // 0 to 1200 
  s->set_gain_ctrl(s, 0);      // 0 = disable , 1 = enable
  s->set_agc_gain(s, 4);       // 0 to 30
  // s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
  s->set_bpc(s, 0);            // 0 = disable , 1 = enable
  s->set_wpc(s, 1);            // 0 = disable , 1 = enable
  s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
  s->set_lenc(s, 1);           // 0 = disable , 1 = enable
  s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
  s->set_vflip(s, 0);          // 0 = disable , 1 = enable
  s->set_dcw(s, 1);            // 0 = disable , 1 = enable
  s->set_colorbar(s, 0);       // 0 = disable , 1 = enable

  SerialMon.println("Camera Successfully Initialized");
}