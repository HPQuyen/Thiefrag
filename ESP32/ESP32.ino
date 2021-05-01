#include <HTTPClient.h>

#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h> // Github: https://github.com/mobizt
#include <WiFiUdp.h>
#include <NTPClient.h>

#include "FS.h"
#include "SPIFFS.h"
#include "esp_camera.h"
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "WiFiManager.h" // https://github.com/tzapu/WiFiManager

/* 1. Define the WiFi credentials */
#define AP_SSID "ESP32 AP"

/* 2. Define the Firebase project host name and API Key */
#define FIREBASE_HOST "thiefrag-65129-default-rtdb.firebaseio.com"
#define API_KEY "AIzaSyAextH1u5uq3LlcZ-5SoMdhuPuwx1jcl6E"

/* 3. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "admin@gmail.com"
#define USER_PASSWORD "password"

/* 4. Define the Firebase storage bucket ID e.g bucket-name.appspot.com */
#define STORAGE_BUCKET_ID "thiefrag-65129.appspot.com"
#define PHOTO_PATH "/photo.jpg"

#define VN_GMT 25200

#define SERVER_NAME "http://thiefrag.ddns.net/mailservice"
#define LED_BUILD_IN 33

// Pin definition for CAMERA_MODEL_AI_THINKER
// Change pin definition if you're using another ESP32 with camera module
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

//*************************************************
//*****         DEFINE REGION FIELDS          *****
//*************************************************

//Define Firebase Data object
FirebaseData fbdo;
FirebaseData fbdoListener;
FirebaseAuth auth;
FirebaseConfig config;

FirebaseJson json;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "vn.pool.ntp.org");

//Stores the camera configuration parameters
camera_config_t camConfig;


//**************************************************
//*****         DEFINE REGION METHODS          *****
//**************************************************
String RandomString(int len);
bool UploadWarningLogs();
void TriggerWarningEvent();
String GetDateTimeString(unsigned long epochTime);


void SetupWifi(){
  while (WiFi.status() != WL_CONNECTED)
  {
      Serial.print(".");
      delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}



void SetupFirebase(){
  /* Assign the project host and api key (required) */
  config.host = FIREBASE_HOST;
  config.api_key = API_KEY;
  
  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  
#if defined(ESP8266)
  //Set the size of WiFi rx/tx buffers in the case where we want to work with large data.
  fbdo.setBSSLBufferSize(1024, 1024);
  fbdoListener.setBSSLBufferSize(1024, 1024);
#endif
  
  //Set the size of HTTP response buffers in the case where we want to work with large data.
  fbdo.setResponseSize(1024);
  fbdoListener.setResponseSize(1024);
  
  //Set database read timeout to 1 minute (max 15 minutes)
  Firebase.RTDB.setReadTimeout(&fbdo, 3000 * 60);
  Firebase.RTDB.setReadTimeout(&fbdoListener, 3000 * 60);
  
  //tiny, small, medium, large and unlimited.
  //Size and its write timeout e.g. tiny (1s), small (10s), medium (30s) and large (60s).
  Firebase.RTDB.setwriteSizeLimit(&fbdo, "medium");
  //optional, set the decimal places for float and double data to be stored in database
  Firebase.setFloatDigits(2);
  Firebase.setDoubleDigits(6);

  if (!Firebase.RTDB.beginStream(&fbdoListener, "/signalWarning"))
  {
    //Could not begin stream connection, then print out the error detail
    Serial.println(fbdo.errorReason());
    ESP.restart();
  }
  Firebase.RTDB.setStreamCallback(&fbdoListener, streamCallback, streamTimeoutCallback);
}



void SetupInitCamera(){
  camConfig.ledc_channel = LEDC_CHANNEL_0;
  camConfig.ledc_timer = LEDC_TIMER_0;
  camConfig.pin_d0 = Y2_GPIO_NUM;
  camConfig.pin_d1 = Y3_GPIO_NUM;
  camConfig.pin_d2 = Y4_GPIO_NUM;
  camConfig.pin_d3 = Y5_GPIO_NUM;
  camConfig.pin_d4 = Y6_GPIO_NUM;
  camConfig.pin_d5 = Y7_GPIO_NUM;
  camConfig.pin_d6 = Y8_GPIO_NUM;
  camConfig.pin_d7 = Y9_GPIO_NUM;
  camConfig.pin_xclk = XCLK_GPIO_NUM;
  camConfig.pin_pclk = PCLK_GPIO_NUM;
  camConfig.pin_vsync = VSYNC_GPIO_NUM;
  camConfig.pin_href = HREF_GPIO_NUM;
  camConfig.pin_sscb_sda = SIOD_GPIO_NUM;
  camConfig.pin_sscb_scl = SIOC_GPIO_NUM;
  camConfig.pin_pwdn = PWDN_GPIO_NUM;
  camConfig.pin_reset = RESET_GPIO_NUM;
  camConfig.xclk_freq_hz = 20000000;
  camConfig.pixel_format = PIXFORMAT_JPEG; //YUV422,GRAYSCALE,RGB565,JPEG

  
  // Select lower framesize if the camera doesn't support PSRAM
  if(psramFound()){
    camConfig.frame_size = FRAMESIZE_SVGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    camConfig.jpeg_quality = 10; //10-63 lower number means higher quality
    camConfig.fb_count = 2;
  } else {
    camConfig.frame_size = FRAMESIZE_CIF;
    camConfig.jpeg_quality = 12;
    camConfig.fb_count = 1;
  }
  
  // Initialize the Camera
  esp_err_t err = esp_camera_init(&camConfig);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
    return;
  }
  sensor_t * s = esp_camera_sensor_get();
  s->set_brightness(s, 0);     // -2 to 2
  s->set_contrast(s, 0);       // -2 to 2
  s->set_saturation(s, 0);     // -2 to 2
  s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
}



void streamCallback(FirebaseStream data)
{
  //Print out all information
  Serial.println("Stream Data...");
  Serial.println(data.streamPath());
  Serial.println(data.dataPath());
  Serial.println(data.dataType());
  Serial.println(data.intData());
  if (data.dataType() == "int" && data.intData() == 1){
    TriggerWarningEvent();
  } 

}



//Global function that notifies when stream connection lost
//The library will resume the stream connection automatically
void streamTimeoutCallback(bool timeout)
{
  Serial.println("Stream timeout");
  if(timeout){
    //Stream timeout occurred
    Serial.println("Stream timeout, resume streaming...");
  }  
}



String RandomString(int len){
  String letters;
  for(int i = 0; i < len; i++){
    byte randomValue = random(0, 37);
    char letter = randomValue + 'a';
    if(randomValue > 26)
      letter = (randomValue - 26) + '0';
    letters += letter;
  }
  return letters;
}


  const char htmlMsg[] = R"=====(<h2 style="text-align: center;"><span style="color: #ff0000;">*** FBI WARNING***</span></h2>
<p style="text-align: left;">A thief is breaking into your house. <br />Photo capture below.</p>
<p style="text-align: left;">Please follow link for more information: <a href="http://thiefrag.ddns.net/login">http://thiefrag.ddns.net</a></p>
<p style="text-align: left;"><strong><span style="text-decoration: underline;">ThiefragDevice_SMTP</span></strong></p>)=====";


bool UploadWarningLogs(){
  String localPath = PHOTO_PATH, remotePath = RandomString(32)+".jpg";
  Serial.println("------------------------------------");
  Serial.println("Upload file...");
  
  //Upload large file over fews hundreds KiB is not allowable in Firebase Storage by using this method.
  //To upload the large file, please use the Firebase.GCStorage.upload instead.
  //The following upload will be error 503 service unavailable (upload rejected due to the large file size)
  //MIME type should be valid to avoid the download problem.
  //The file systems for flash and SD/SDMMC can be changed in FirebaseFS.h.
  if (Firebase.Storage.upload(&fbdo, STORAGE_BUCKET_ID /* Firebase Storage bucket id */, localPath.c_str() /* path to local file */, mem_storage_type_flash /* memory storage type, mem_storage_type_flash and mem_storage_type_sd */, remotePath.c_str() /* path of remote file stored in the bucket */, "image/jpeg" /* mime type */))
  {
    unsigned long timestamp = timeClient.getEpochTime() + VN_GMT;
    Serial.printf("Download URL: %s\n", fbdo.downloadURL().c_str());
    Serial.println("------------------------------------");
    String pathDatetime = "/logs/" + String(timestamp) + "/datetime";
    String pathSnapshot = "/logs/" + String(timestamp) + "/snapshotUrl";
    String downloadURL = fbdo.downloadURL();
    if (Firebase.RTDB.set(&fbdo, pathDatetime.c_str(), GetDateTimeString(timestamp)) && Firebase.RTDB.set(&fbdo, pathSnapshot.c_str(), downloadURL) && Firebase.RTDB.set(&fbdo, "/signalWarning", 0))
    {
      Serial.println("PASSED");
      return true;
    }
    Serial.println("FAILED");
    Serial.println("REASON: " + fbdo.errorReason());
    Serial.println("------------------------------------");
    return false;
  }
  Serial.println("FAILED");
  Serial.println("REASON: " + fbdo.errorReason());
  Serial.println("------------------------------------");
  return false;
}

String GetDateTimeString(unsigned long epochTime){
 time_t t_now = epochTime;
 tm *ltm = localtime(&t_now);
 char buf[25];
 strftime(buf, 25, "%d/%m/%Y %T", ltm);
 return String(buf);
}

bool TriggerTakeSnapshot(){

  camera_fb_t * fb = NULL; // pointer

  // Take a photo with the camera
  Serial.println("Taking a photo...");

  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return false;
  }

  Serial.printf("Picture file name: %s\n", PHOTO_PATH);
  File file = SPIFFS.open(PHOTO_PATH, FILE_WRITE);
  
  // Insert the data in the photo file
  if (!file) {
    Serial.println("Failed to open file in writing mode");
    return false;
  }
  else {
    file.write(fb->buf, fb->len); // payload (image), payload length
    
    Serial.print("The picture has been saved in ");
    Serial.print(PHOTO_PATH);
    Serial.print(" - Size: ");
    Serial.print(file.size());
    Serial.println(" bytes");
  }
  file.close();
  esp_camera_fb_return(fb);
  return true;
}



void TriggerSendEmail(){
  Serial.println(ESP.getFreeHeap());
  // Preparing email
  Serial.println("Sending email...");
  HTTPClient httpClient;
  httpClient.begin(SERVER_NAME);
  httpClient.addHeader("Content-Type", "Content-Type: application/json");
  int responceHttpCode = httpClient.POST("{ \"API_KEY\" : \"5E884898DA28047151D0E56F8DC6292773603D0D6AABBDD62A11EF721D1542D8\" }");
  httpClient.end();
}



void TriggerWarningEvent(){
  // Take Camera -> snapshot photo by ESP32-CAM and save to Flash memory photo.jpg
  if(!TriggerTakeSnapshot())
    return;
  // Send email or sms for user
  TriggerSendEmail();
  if(!UploadWarningLogs()){
    Firebase.RTDB.set(&fbdo, "/signalWarning", 0);
    Serial.println("FAILED");
    Serial.println("REASON: " + fbdo.errorReason());
    Serial.println("------------------------------------");
  }
}



void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  Serial.begin(115200);

  WiFi.disconnect(false,true);
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  WiFiManager wm;
  bool res;
  res = wm.autoConnect(AP_SSID); // anonymous ap
  if(!res) {
    Serial.println("Failed to connect");
    ESP.restart();
  }
  //if you get here you have connected to the WiFi    
  Serial.println("connected...yeey :)");
  Serial.println(ESP.getFreeHeap());
  // Setup every thing here
  SetupWifi();
  SetupFirebase();
  SetupInitCamera();
  if(!SPIFFS.begin()){
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  timeClient.begin();
  pinMode(LED_BUILD_IN, OUTPUT);
}

unsigned long prevTime = 0;
unsigned long currentTime = 0;
int state = LOW;
void loop()
{
  currentTime = millis();
  if(currentTime >= prevTime + 500){
    if(state == LOW){
      digitalWrite(LED_BUILD_IN, HIGH);
      state = HIGH;
    }else{
      digitalWrite(LED_BUILD_IN, LOW);
      state = LOW;
    }
    prevTime = currentTime;
  }
  timeClient.update();
}
