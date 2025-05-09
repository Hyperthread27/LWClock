#ifndef GLOBAL_H
#define GLOBAL_H

const String nName = "LWClock_"; //"LWScreen_"; "LWClock_"; 
const String nVersion = "v2.1";
#define USE_RTC false //USE RTC chip DS3231 
#define USE_BME280 false //USE sensor BME280 (5V) http://got.by/40d52x
#define USE_DHT false //USE sensor DHT
#define MAX_DEVICES 4  //Number of indicator modules MAX7219
#include <ESP8266WiFi.h>     
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ESP8266SSDP.h>
#include <Updater.h>//for update firmware and SPIFFS
#include <FS.h>
#include <SPI.h>
#include <Wire.h> // must be included here so that Arduino library object file references work              
#include <ArduinoJson.h>        //https://github.com/bblanchon/ArduinoJson.git
#include <TimeLib.h>            //https://github.com/PaulStoffregen/Time Michael Margolis
#include <Timezone.h>           //https://github.com/JChristensen/Timezone
#include <MD_Parola.h>          //https://github.com/MajicDesigns/MD_Parola
#include <MD_MAX72xx.h>         //https://github.com/MajicDesigns/MD_MAX72XX
#include <MD_MAXPanel.h>        //https://github.com/MajicDesigns/MD_MAXPanel
#include <PubSubClient.h> //"https://github.com/knolleary/pubsubclient.git"
#include "NetCrtESP.h" //https://github.com/Lightwell-bg/NetCrtESP
//#include "ESPTimeFunc.h" //https://github.com/Lightwell-bg/ESPTimeFunc
#if USE_RTC == false
  #include "RTClib.h" //https://github.com/adafruit/RTClib
  RTC_DS3231 rtc;
#endif
#if USE_DHT == true
  #include <Adafruit_Sensor.h>                           // https://github.com/adafruit/Adafruit_Sensor
  #include "DHT.h"                                      //https://github.com/adafruit/DHT-sensor-library
  //#define DHTTYPE DHT21   // DHT 21 (AM2301)
  #define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
  #define TEMPHUM_PIN 2    // D4 выберите входной контакт,к которому подключен датчик
  DHT dht(TEMPHUM_PIN, DHTTYPE);
#endif //DHT
#if USE_BME280 == true // I2C D1, D2
  #include "BlueDot_BME280.h"                         //https://github.com/BlueDot-Arduino/BlueDot_BME280
  BlueDot_BME280 bme280 = BlueDot_BME280();  
#endif //BME280
#define ALARM_PIN 12  
#define BUZ_PIN 15 

#include "fonts_rus.h"
#include "font_5bite_rus.h" 
#include "font_6bite_rus.h" 

#define CLK_PIN   13
#define DATA_PIN  11
#define CS_PIN    16

ESP8266WebServer HTTP; 
File fsUploadFile; 
NetCrtESP myWIFI;
WiFiClient ESPclient;
PubSubClient mqttClient(ESPclient);

// Ensure MQTTLine is initialized properly
String MQTTLine = "";

//For dispalay name of device
#if USE_RTC == true
  const String urts = " +RTC";
  bool useRTC = true; 
#else  
  const String urts = " noRTC";
  bool useRTC = false; 
#endif

#if USE_DHT == true
  String cVersion = nName + nVersion + urts + " DHT22";
#elif USE_BME280 == true 
  String cVersion = nName + nVersion + urts + " BME280";
#else 
  String cVersion = nName + nVersion + urts + " no sensor";  
#endif //BME280 

#if (USE_BME280 == true || USE_DHT == true) //Show T, H, (P) indoor if have sensor
  const uint8_t maxModeShow = 11; //Number of programs
#else 
  const uint8_t maxModeShow = 10; //Number of programs
#endif

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
MD_MAXPanel MP = MD_MAXPanel(P.getGraphicObject(), MAX_DEVICES, 1);

// Global data
typedef struct {
  textEffect_t  effect;   // text effect to display
  String        psz;      // text string nul terminated
  uint16_t      speed;    // speed multiplier of library default
  uint16_t      pause;    // pause multiplier for library default
} sCatalog;

sCatalog  catalog[] = {
  { PA_PRINT, "Eффект PRINT", 1, 5 }, //0
  { PA_SLICE, "Eффект SLICE", 1, 5 }, //1
  { PA_MESH, "Eффект MESH", 20, 5 },  //2
  { PA_FADE, "Eффект FADE", 20, 4 },  //3
  { PA_WIPE, "Eффект WIPE", 5, 4 },   //4
  { PA_WIPE_CURSOR, "Eффект  WPE_C", 4, 4 },  //5 //////////////
  { PA_OPENING, "Eффект OPEN", 3, 4 },   //6
  { PA_OPENING_CURSOR , "Eффект OPN_C", 4, 4 },   //7
  { PA_CLOSING, "Eффект CLOSE", 3, 4 },   //8
  { PA_CLOSING_CURSOR, "Eффект CLS_C", 4, 4 },  //9
  { PA_RANDOM, "Eффект RAND", 3, 4 },  //10
  { PA_BLINDS, "Eффект BLIND", 7, 4 },  //11
  { PA_DISSOLVE, "Eффект DSLVE", 7, 4 },  //12
  { PA_SCROLL_UP, "Eффект SC_U", 5, 4 },  //13
  { PA_SCROLL_DOWN, "Eффект SC_D", 5, 4 },//14
  { PA_SCROLL_LEFT, "Eффект SC_L", 5, 4 },  //15
  { PA_SCROLL_RIGHT, "Eффект SC_R", 5, 4 },//16
  { PA_SCROLL_UP_LEFT, "Eффект SC_UL", 7, 4 }, //17
  { PA_SCROLL_UP_RIGHT, "Eффект SC_UR", 7, 4 }, //18  
  { PA_SCROLL_DOWN_LEFT, "Eффект SC_DL", 7, 4 }, //19
  { PA_SCROLL_DOWN_RIGHT, "Eффект SC_DR", 7, 4 },//20
  { PA_SCAN_HORIZ, "Eффект SCANH", 4, 4 },  //21
  { PA_SCAN_VERT, "Eффект SCANV", 3, 4 }, //22
  { PA_GROW_UP, "Eффект GRW_U", 7, 4 }, //23
  { PA_GROW_DOWN, "Eффект GRW_D", 7, 4 }, //24
};

char buf[256]; //For MD PAROLA
String logo = "Hello!";
const char* day_ru[] PROGMEM = {"Воскресенье", "Понедельник", "Вторник", "Среда", "Четверг", "Пятница", "Суббота"};
const char* day_bg[] PROGMEM = {"Неделя", "Понеделник", "Вторник", "Сряда", "Четвъртък", "Петък", "Събота"};
const char* day_en[] PROGMEM = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const char** day_table[] PROGMEM = {day_ru, day_bg, day_en};
const char* month_ru[] PROGMEM = {"Января", "Февраля", "Марта", "Апреля", "Мая", "Июня", "Июля", "Августа", "Сентября", "Октября", "Ноября", "Декабря"};
const char* month_bg[] PROGMEM = {"Януари", "Февруари", "Март", "Април", "Май", "Юни", "Юли", "Август", "Септември", "Октомври", "Ноември", "Декември"};
const char* month_en[] PROGMEM = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
const char** month_table[] PROGMEM = {month_ru, month_bg, month_en};

// Wi-Fi setting
String ssid = "SSID";
String password = "SSIDPASSWORD";
String ssidAP = "LWClock";   // SSID AP 
String passwordAP = "LWClock"; // AP password

// Generate a unique device name using the last 3 bytes of the MAC address
String getUniqueDeviceName() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char uniqueSuffix[8];
  sprintf(uniqueSuffix, "_%02X%02X%02X", mac[3], mac[4], mac[5]); // Use the last 3 bytes of the MAC address
  return String("LWClock") + uniqueSuffix;
}

// Update the global MQTT name and SSDP name
String mqtt_name = getUniqueDeviceName();
String SSDP_Name = mqtt_name;

String apIP = "192.168.4.1";


String jsonConfig = "{}";
uint8_t lang = 2; //0-RU, 1-BG, 2 -EN, 3-UA
unsigned long timeCount = 0; //Time counter for All
//Time
int8_t timezone = -5;              // Time zone GMT
bool isDayLightSaving = true; //Summer time use
String sNtpServerName = "us.pool.ntp.org";
bool statusUpdateNtpTime = 1; //
const unsigned long TIME_UPDATE = 1000*60*10; //Time update interval
const unsigned long NTP_UPDATE = 1000*30;
unsigned long lastNtpTime = 0;
// US Central Time (Chicago)
TimeChangeRule CDT = {"CDT", Second, Sun, Mar, 2, -300};  // Central Daylight Time (UTC-5)
TimeChangeRule CST = {"CST", First, Sun, Nov, 2, -360};   // Central Standard Time (UTC-6)
Timezone CT(CDT, CST);
TimeChangeRule *tcr;

//Display TEXT
uint32_t  displayClockCount = 900000; //counter for time of display CLOCK (15 minutes)
bool displayClockFirst = true; //Flag for entering to display CLOCK first 
char  szTime[9];    // hh:mm\0
String strText0="", strText1="Konnichiwa", strText2="", strText3="";
bool isTxtOn0 = false, isTxtOn1 = false, isTxtOn2 = false, isTxtOn3 = false;
bool isCrLine0 = false, isCrLine1 = true, isCrLine2 = false, isCrLine3 = false;
float txtFrom0=0, txtFrom1=0, txtFrom2=0, txtFrom3=0; 
float txtTo0=24, txtTo1=24, txtTo2=24, txtTo3=24;
float weathFrom=0, fcastFrom=0, clockFrom=0, dateFrom=0, seaFrom=0;
float weathTo=24, fcastTo=24, clockTo=24, dateTo=24, seaTo=24; 
//Indoor T,H,P
bool isLedTHP = true; 
float thpFrom=0, thpTo=24; 
String strTHP; //String temp, hum, press

uint8_t modeShow = 1; //Mode of display 1 - clock, 2 - date, 3 - run text etc

//Setup for LED
uint8_t fontUsed = 0; //fonts
uint8_t dmodefrom = 8, dmodeto = 20; //DAY MODE
uint8_t brightd = 3, brightn = 0; //brightness day and night
uint16_t speedTicker = 5; // speed of creeping line
bool isLedClock = true, isLedDate = true, isLedWeather = false, isLedForecast = false, isLedSea = false;
float global_start = 0, global_stop = 24; //Working time

// Weather server setting
String  W_URL  = "api.openweathermap.org";
String  W_API    = "************************************"; 
String  CITY_ID     = "*******"; // Other city code http://bulk.openweathermap.org/sample/city.list.json.gz
String strWeather, strWeatherFcast, strSea; //strSea - for future use
const unsigned long PER_GET_WEATHER = 1000*60*6;
const unsigned long PER_GET_WEATHER_FCAST = 1000*60*11;
const unsigned long PER_GET_SEA = 1000*60*15;
unsigned long lastTimeWeather = 0; unsigned long lastTimeWeatherFcast = 0, lastTimeSea=0;
const char* overboard[] PROGMEM = {"За бортом ", "Извън борда ", "Weather: "};
const char* overboardfcast[] PROGMEM = {"Завтра за бортом ", "Утре извън борда ", "Weather Tomorrow: "};
const char* temper[] PROGMEM = {". \xC5 ", ". \xC5 ", ". \xC5 "};
const char* hum[] PROGMEM = {"°F. Æ", "°F. Æ", "°F. Æ"};
const char* pres[] PROGMEM = {"% \xC8 ", "% \xC8 ", "% \xC8 "};
const char* wind[] PROGMEM = {"hPa \xC7 ", "hPa \xC7 ", "hPa \xC7 "};
const char* windsp[] PROGMEM = {" mph ", " mph ", " mph "};
const char* windir_ru[] PROGMEM = {"С-В ", "В ", "Ю-В ", "Ю ", "Ю-З ", "З ", "С.-З ", "С "};
const char* windir_bg[] PROGMEM = {"С-И ", "И ", "Ю-И ", "Ю ", "Ю-З ", "З ", "С-З ", "С "};
const char* windir_en[] PROGMEM = {"N-E ", "E ", "S-E ", "S ", "S-W ", "W ", "N-W ", "N "};
const char** windir_table[] PROGMEM = {windir_ru, windir_bg, windir_en};
const char* cloudstxt[] PROGMEM = {" \xC9 ", " \xC9 ", " \xC9 "};
const char* forecast[] PROGMEM = {"Завтра  ", "Утре ", "Tomorrow "};
const char* tempermin[] PROGMEM = {". \xC5.мин ", ". \xC5.мин ", ". \xC5.min "};
const char* tempermax[] PROGMEM = {"°F макс ", "°F макс ", "°F max "};
const char* tempersea[] PROGMEM = {"Темп.моря ", "Морската вода ", "Sea temp "};
const char* onboard[] PROGMEM = {"На борту темп ", "На борда темп ", "Onboard air "};
const char* temperIn[] PROGMEM = {"Темп", "Темп ", "Temp"};
const char* humIn[] PROGMEM = {"Влаж", "Влаж", "Hum"};
const char* presIn[] PROGMEM = {"Давл", "Нал", "Pres"};  

//sea
String WSea_URL = "worldseatemp.com";
String SEA_ID = "59";

//alarm
const uint8_t day_byte[] = {1, 2, 4, 8, 16, 32, 64}; //1 - Sunday, 2 - Monday .... etc.
typedef struct {
  uint8_t alarm_h;   
  uint8_t alarm_m;     
  uint8_t alarm_stat;     
} sAlarmSet;
sAlarmSet myAlarm[] = {{6,30,0},{7,30,0}};
bool stopAlarm = false; //Alarm of flag (Флаг кнопки выкл. ALARM)
unsigned long timeStopAlarm = 0; //Отсчет времени активности кнопки (1 мин)  

//sensor
bool dataCorrect = false; //use correction for temp and hum depending brightness 
bool hpa = true; //Pressure hPa or mm for MQTT

//mqtt
bool mqttOn = true;
char mqttData[80]; //array for send to  MQTT
String mqtt_server = "127.0.0.1"; // Name of MQTT server
int mqtt_port = 1883; //  MQTT port
String mqtt_user = "mqtt-user"; // MQTT login
String mqtt_pass = "mqtt-user"; // MQTT pass
String mqtt_sub_crline = "LWClock/MQTTLine"; //Topic for subcribe to creeping line
unsigned long ReconnectTime = 0;  const unsigned long MQTT_CONNECT = 1000*60*3;
String mqtt_pub_temp = "LWClock/temp"; 
String mqtt_pub_hum = "LWClock/hum";
String mqtt_pub_press = "LWClock/press";
unsigned long mqttDhtTime = 0;  const unsigned long MQTT_SEND_INT = 1000*60*7; //MQTT send interval
unsigned long lastTimePHT = 0; const unsigned long PER_GET_THP = 1000*60*3; //get sensor data Period 

float mqttFrom = 0.0; // Start time for MQTT display mode
float mqttTo = 24.0;  // End time for MQTT display mode

//thingspeak.com
unsigned long tspeakDhtTime = 0;  const unsigned long TSPEAK_SEND_INT = 1000*60*10; //TSPEAK send interval
bool tspeakOn = false;
String tspeak_server = "";
unsigned long tspeak_channal = 1;
String tspeak_wapi = "";  

String mqttMessage = ""; // Holds the latest MQTT message
bool mqttMessageReceived = false; // Flag to indicate a new MQTT message was received

//predefine functions
String GetTime(bool s=false); //s - показывать секунды
void getTimeDisp(char *psz, bool f = true);
String GetDate();
void WIFI_init();
void FS_init(void);
bool loadConfig();
void SSDP_init(void);
void Display_init(void);
void Time_init();
void timeSynch();
void workAlarms();
void displayTime(bool lastShow);
bool showText(String sText, textPosition_t t_pos, uint16_t t_speed, uint16_t t_pause, textEffect_t  t_effectBegin, textEffect_t  t_effectEnd);
uint8_t showTextUP(String sText, uint16_t t_speed, uint16_t t_pause, textEffect_t  t_effectBegin, textEffect_t  t_effectEnd, bool last);
void server_httpinit(void);
String GetWeather();
String GetWeatherForecast();
String GetSea();
bool compTimeInt(float tFrom, float tTo, float tNow);
void init_mqtt();
void init_sensor();
boolean reconnect();
void sendMQTT();
void sendTspeak();
String getTempDHT();
String getHumDHT();
String getTempBME280(bool correct = false, uint8_t pr = 1);
String getHumBME280(bool correct = false, uint8_t pr = 0);
String getPressBME280(bool hpa = false, uint8_t pr = 0);
#endif
