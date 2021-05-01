#include "pitches.h"

#if defined(ESP32)
#include "WiFi.h"
#elif defined(ESP8266)
#include "ESP8266WiFi.h"
#endif

#include "Firebase_ESP_Client.h"
#include "WiFiManager.h" // https://github.com/tzapu/WiFiManager

/* 1. Define the WiFi credentials */
#define WIFI_SSID "Sam"
#define WIFI_PASSWORD "5091Nephilim7031"

/* 2. Define the Firebase project host name and API Key */
#define FIREBASE_HOST "thiefrag-65129-default-rtdb.firebaseio.com"
#define API_KEY "AIzaSyAextH1u5uq3LlcZ-5SoMdhuPuwx1jcl6E"

/* 3. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "admin2@gmail.com"
#define USER_PASSWORD "password"

#define MUSIC_PLAY_TIME 5

//*************************************************
//*****         DEFINE REGION FIELDS          *****
//*************************************************
//Define Firebase Data object
FirebaseData fbdo;
FirebaseData fbdoListener;
FirebaseData fbdoListener2;

FirebaseAuth auth;
FirebaseConfig config;

FirebaseJson json;

//*************************************************
//*****         DEFINE PROGRAM FIELDS          ****
//*************************************************
bool isActive = true;
bool isAutomatical = true;

const int pirPin = D1;        // Chọn chân tín hiệu vào cho PIR
bool isSus = false;
int susCount = 0;
const int susThreshhold = 3;

int lastPeriod = 0;
bool isChangePeriod = false;
const int periodFactor = 2;
const int periodThreshhold = 10;

const int buzzerPin = D6;     // Chọn chân tín hiệu cho buzzer
// Giai điệu:
int melody[] = {
  NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4
};

// Thời gian từng nốt: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {
  4, 8, 8, 4, 4, 4, 4, 4
};

const int photoresisterPin = A0;
const int photoresisterThreshhold = 620;
int photoresisterState;

/***************  Timer  ***************/
float oldTime;
float currentTime;
float deltaTime;

float timer;

float ledTimer;
bool isOnLed = false;

void playMusic();
void detectMovement();
void resetStat();


//**************************************************
//*****         DEFINE REGION METHODS          *****
//**************************************************

void SetupWifi(){
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  Serial.print("Connecting to Wi-Fi");
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


void streamCallback(FirebaseStream data)
{
  //Print out all information
  Serial.println("Stream Data...");
  Serial.println(data.streamPath());
  Serial.println(data.dataPath());
  Serial.println(data.dataType());
  Serial.println(data.stringData());
  
  if(data.streamPath() == "/settings/IODevice"){
    if (data.dataType() == "string" && data.stringData() == "on"){
      // Set IO Device on
      isActive = true;
    }else{
      // Set IO Device off
      isActive = false;
    }
    Serial.println("Active state: ");
    Serial.println(isActive);
  }else{
    if (data.dataType() == "string" && data.stringData() == "on"){
      // Set IO Automatic on
      isAutomatical = true;
    }else{
      // Set IO Automatic off
      isAutomatical = false;
    }
    Serial.println("isAutomatical state: ");
    Serial.println(isAutomatical);
  }

  
}

//Global function that notifies when stream connection lost
//The library will resume the stream connection automatically
void streamTimeoutCallback(bool timeout)
{
  if(timeout){
    //Stream timeout occurred
    Serial.println("Stream timeout, resume streaming...");
  }
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
  #endif
  
  //Set the size of HTTP response buffers in the case where we want to work with large data.
  fbdo.setResponseSize(1024);
  //Set database read timeout to 1 minute (max 15 minutes)
  Firebase.RTDB.setReadTimeout(&fbdo, 1000 * 60);
  //tiny, small, medium, large and unlimited.
  //Size and its write timeout e.g. tiny (1s), small (10s), medium (30s) and large (60s).
  Firebase.RTDB.setwriteSizeLimit(&fbdo, "tiny");
  //optional, set the decimal places for float and double data to be stored in database
  Firebase.setFloatDigits(2);
  Firebase.setDoubleDigits(6);

  // Set up stream
  if (!Firebase.RTDB.beginStream(&fbdoListener, "/settings/IODevice"))
  {
    //Could not begin stream connection, then print out the error detail
    Serial.println(fbdoListener.errorReason());
  }
  if (!Firebase.RTDB.beginStream(&fbdoListener2, "/settings/IOAutomatic"))
  {
    //Could not begin stream connection, then print out the error detail
    Serial.println(fbdoListener2.errorReason());
  }
  Firebase.RTDB.setStreamCallback(&fbdoListener, streamCallback, streamTimeoutCallback);
  Firebase.RTDB.setStreamCallback(&fbdoListener2, streamCallback, streamTimeoutCallback);
}

bool TriggerESP32(){
  if (Firebase.RTDB.set(&fbdo, "/signalWarning", 1))
  {
    Serial.println("PASSED");
    return true;
  }
  Serial.println("FAILED");
  Serial.println("REASON: " + fbdo.errorReason());
  Serial.println("------------------------------------");
  return false;
}

void setup()
{
  pinMode(pirPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(photoresisterPin, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  WiFiManager wm;
  bool res;
  res = wm.autoConnect("ESP8266 AP"); // anonymous ap
  if(!res) {
      Serial.println("Failed to connect");
      ESP.restart();
  } 
  else {
    //if you get here you have connected to the WiFi    
    Serial.println("connected...yeey :)");
    // Setup any thing here
    SetupWifi();
    SetupFirebase();
  }
}
 
void loop()
{  
  oldTime = currentTime;
  currentTime = millis();
  deltaTime = (currentTime - oldTime) / 1000;

  ledTimer += deltaTime;
  if (ledTimer >= 1.0)
  {
    if (!isOnLed)
    {
      digitalWrite(LED_BUILTIN, HIGH);
      isOnLed = true;
    }
    else
    {
      digitalWrite(LED_BUILTIN, LOW);
      isOnLed = false;
    }

    ledTimer = 0;
  }
  
  // Bật thiết bị
  if (isActive)
  {
    if (isAutomatical)
    {
      photoresisterState = analogRead(photoresisterPin);
      
      // Tối
      if (photoresisterState > photoresisterThreshhold)      
      {
        detectMovement();
      }
      else
      {
        resetStat();
      }
    }
    else
    {
      detectMovement();
    }
  }
}

void playMusic() {
  // Iterate over the notes of the melody:
  for (int thisNote = 0; thisNote < 8; thisNote++) {

    // To calculate the note duration, take one second
    // divided by the note type.
    // e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(buzzerPin, melody[thisNote], noteDuration);

    // To distinguish the notes, set a minimum time between them.
    // The note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // Stop the tone playing:
    noTone(buzzerPin);
  }
}

void detectMovement() {
  int pirState = digitalRead(pirPin);

  Serial.println(pirState);

  // Có chuyển động
  if (isSus == false && pirState == HIGH)                                     
  {
    isSus = true;
  }

  if (isSus)
  {
    timer += deltaTime;
    Serial.printf("Timer: %f\n", timer);

    int period = timer / periodFactor;
    if (period != lastPeriod)
    {
      lastPeriod = period;
      isChangePeriod = true;
    }

    Serial.printf("Period: %d; Last period: %d; Flag: %d\n", period, lastPeriod, isChangePeriod);

    if (pirState == HIGH && isChangePeriod == true)                                     
    {
      susCount++;
      isChangePeriod = false;
    }

    if (timer >= periodThreshhold && susCount >= susThreshhold) {
      TriggerESP32();
      for(int i = 0; i < MUSIC_PLAY_TIME; i++) {
        playMusic();
        delay(500);
      }
      
      Serial.printf("Sus count: %d\n", susCount);

      resetStat();
    }
    else if (timer >= periodThreshhold && susCount < susThreshhold)
    {
      Serial.printf("Sus count: %d\n", susCount);
      
      resetStat();
    }
  }

}

void resetStat() {
  isSus = false;
  susCount = 0;
  isChangePeriod = false;
  timer = 0;
  lastPeriod = 0;
}
