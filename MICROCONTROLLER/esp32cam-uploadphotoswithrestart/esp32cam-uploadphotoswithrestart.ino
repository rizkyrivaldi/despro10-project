// Camera Snapper

// FRAMESIZE_UXGA (1600 x 1200)
// FRAMESIZE_QVGA (320 x 240)
// FRAMESIZE_CIF (352 x 288)
// FRAMESIZE_VGA (640 x 480)
// FRAMESIZE_SVGA (800 x 600)
// FRAMESIZE_XGA (1024 x 768)
// FRAMESIZE_SXGA (1280 x 1024)

// https://randomnerdtutorials.com/esp32-cam-ov2640-camera-settings/

// Camera Model Definition
#define CAMERA_MODEL_AI_THINKER // Has PSRAM

#include <FS.h>
#include <SD_MMC.h>
#include <WiFi.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include "esp_camera.h"
#include "camera_pins.h"

// Deep Sleep Definitions
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  300        /* Time ESP32 will go to sleep (in seconds) */

// Function Prototypes
int readCounter();
void writeCounter(int n);
void initCamera();
void savePicture(int counter);
void initWifi();
void uploadPhoto();

// Global variables
int counter = 0;
bool night_mode = false;

// Wifi Setup
const char* ssid = "DFortress_Micro";
const char* password = "darkwingxmod";
String serverName = "192.168.2.10";
String serverPath = "/upload.php";
const int serverPort = 8888;

// const char* ssid = "RPortable";
// const char* password = "darkwingxmod";
// String serverName = "118.136.215.134";
// String serverPath = "/upload.php";
// const int serverPort = 1234;

WiFiClient client;

void setup() {
  // WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  // SD_MMC.begin("/sdcard", true) -> for disabling led flickers when sd card is present

  Serial.begin(115200);

  initCamera();
  initWifi();

  uploadPhoto();

  // Enable hibernation mode
  esp_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);

  // Enters hibernation
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void loop() {
  // put your main code here, to run repeatedly:
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

  config.frame_size = FRAMESIZE_UXGA;
  config.jpeg_quality = 35;
  config.fb_count = 2;
  config.grab_mode = CAMERA_GRAB_LATEST;

  Serial.println("Intitializing camera");
  // Init Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  s -> set_framesize(s, FRAMESIZE_UXGA);
  // Mode for day light environment
  if(!night_mode == true){
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
  }

  // Mode for low light environment
  else{
    s -> set_framesize(s, FRAMESIZE_UXGA);
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
    s->set_aec_value(s, 600);    // 0 to 1200 
    s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
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
  }
}

void initWifi(){
  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("ESP32-CAM IP Address: ");
  Serial.println(WiFi.localIP());
}

void uploadPhoto(){
  String getAll;
  String getBody;
  camera_fb_t * fb = NULL;
  Serial.println("Taking pictures...");

  if(night_mode == true){
    digitalWrite(4, HIGH);
  }
  else{
    digitalWrite(4, LOW);
  }
  
  // Take Picture with Camera
  fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Camera capture failed");
    ESP.restart();
  }
  else{
    Serial.println("Camera capture successful!");
  }

  digitalWrite(4, LOW);

  Serial.println("Connecting to server: " + serverName);
  if (client.connect(serverName.c_str(), serverPort)){

    Serial.println("Connection successful!");    
    String head = "--ThisIsABoundary\r\nContent-Disposition: form-data; name=\"imageFile\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--ThisIsABoundary--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;
  
    client.println("POST " + serverPath + " HTTP/1.1");
    client.println("Host: " + serverName);
    client.println("Content-Length: " + String(totalLen));
    client.println("Content-Type: multipart/form-data; boundary=ThisIsABoundary");
    client.println();
    client.print(head);
  
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n=0; n<fbLen; n=n+1024) {
      if (n+1024 < fbLen) {
        client.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen%1024>0) {
        size_t remainder = fbLen%1024;
        client.write(fbBuf, remainder);
      }
    }   
    client.print(tail);

    esp_camera_fb_return(fb);

    // Waiting for response from the php server
    int timeoutTimer = 10000;
    long startTimer = millis();
    bool state = false;
    bool feedback = false;

    Serial.println("\r\nWaiting for server response...");
    while((startTimer + timeoutTimer) > millis()){
      Serial.print(".");
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
    Serial.println();
    client.stop();
    Serial.println(getBody);
    if(!feedback){
      ESP.restart();
    }
  }
  else{
    ESP.restart();
  }
}
