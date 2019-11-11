#include <NTPClient.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <ESP8266WiFi.h>

const char ssid[][10] = {"nigel", "08154711"};
const char password[][20] = {"##IdW16AwdTudB##", "xD9fgLH6"};

volatile unsigned long last_interrupt_time = 0;
volatile int mode = 0;
bool forcesync = true;
unsigned long currentmillis;
unsigned long lastmillis = 0;
const int intervall_0 = 500;
const int intervall_1 = 1000;
const int intervall_2 = 500;
const int utcOffsetInSeconds = 3600;
unsigned long time_lastcon, time_last, syncedtimeinmilliseconds;
unsigned long timeinmilliseconds = 0;
char conssid[10];
String year, month, day, minutes, seconds, sum, sum_old, toprint, formatteddate;
float temperature, humidity, pressure, altitude;
int rawhours;

byte doppelpunktrechts[] = {
  0x00,
  0x01,
  0x01,
  0x00,
  0x01,
  0x01,
  0x00,
  0x00
};
  byte doppelpunktlinks[] = {
  0x00,
  0x10,
  0x10,
  0x00,
  0x10,
  0x10,
  0x00,
  0x00
};
byte grad[] = {
  0x06,
  0x09,
  0x09,
  0x06,
  0x00,
  0x00,
  0x00,
  0x00
};

Adafruit_BME280 bme;
LiquidCrystal_I2C lcd(0x27,16,2);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", utcOffsetInSeconds);
ESP8266WebServer server(80);

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("Started serial");
  lcd.init();
  lcd.backlight();
  lcd.noCursor();
  lcd.createChar(0, doppelpunktrechts);
  lcd.createChar(1, doppelpunktlinks);
  lcd.createChar(2, grad);
  Serial.println("Started lcd");
  bme.begin(0x76);
  Serial.println("Started BME280");
  Serial.println();

  if (connectWIFI(1) == false){
    if (connectWIFI(0) == false){
      lcd.clear();
      lcd.setCursor(0,1);
      lcd.print("restart...");
      Serial.print("restart...");
      delay(500);
      ESP.restart();
    }
  }
  Serial.println("WiFi connected..!");
  lcd.setCursor(0,1);
  lcd.print("connected        ");
  Serial.print("Got IP: ");  Serial.println(WiFi.localIP());
  attachInterrupt(digitalPinToInterrupt(15), interrupt_handler, RISING);
  timeClient.begin();
  server.on("/", handle_OnConnect);
  server.onNotFound(handle_NotFound);
  server.begin();
  Serial.println("HTTP server started");
  delay(700);
}

void loop() {
  // put your main code here, to run repeatedly:
  currentmillis = millis();
  server.handleClient();
  switch (mode){
    case 0:
      if(currentmillis - lastmillis > intervall_0 || forcesync == true){
        forcesync = false;
        lastmillis = currentmillis;
        build_print(0);

      }
      break;
    case 1:
      if(currentmillis - lastmillis > intervall_1 || forcesync == true){
        forcesync = false;
        lastmillis = currentmillis;
        build_print(1);

      }
      break;
    case 2:
      if(currentmillis - lastmillis > intervall_2 || forcesync == true){
        forcesync = false;
        lastmillis = currentmillis;
        build_print(2);

      }
      break;
    case 3:
      if(forcesync == true){
        forcesync = false;
        build_print(3);
      }
      break;
    case 4:
      if(forcesync == true){
        forcesync = false;
        build_print(4);
      }
      break;
  }

}

void build_print(int modus){
  switch(modus){
      case 0:
      {
      get_indoor();
      get_time();
      sum = String(rawhours) + minutes + String(round(temperature)) + String(round(humidity));
      if (sum != sum_old){
        lcd.clear();
        lcd.setCursor(7-String(rawhours).length(), 0);
        lcd.print(rawhours);
        lcd.write(0); lcd.write(1);
        lcd.print(minutes);
  
        if(String(round(temperature)).length() <= 2){
          lcd.setCursor(1,1);lcd.print("T:");
          lcd.print(temperature, 0);
          lcd.print("\2C");
        }
        else{
          lcd.setCursor(0,1);lcd.print("T:");
          lcd.print(temperature, 0);
          lcd.print("\2C");
        }
        
        lcd.setCursor(16 - String(round(humidity)).length() - 4 , 1); lcd.print("H:");
        lcd.print(humidity, 0); lcd.print("%");
        sum_old = sum;
      }
      }
      break;

      case 1:
      {
      get_indoor();
      sum = String(round(temperature*10)/10) + String(round(humidity*10)/10) + String(round(pressure)) + String(round(altitude/10)*10);
      if (sum != sum_old){
        lcd.clear();
        lcd.setCursor(1, 0); lcd.print(temperature, 1); lcd.print("\2C");       
        lcd.setCursor(16 - String(humidity).length() -1 , 0); lcd.print(humidity, 1); lcd.print("%");
        lcd.setCursor(1,1); lcd.print(pressure, 0); lcd.print("hPa");
        lcd.setCursor(16 - String(round(altitude)).length() - 3, 1); lcd.print("~"); 
        lcd.print(round(altitude/10)*10); lcd.print("m");      
        sum_old = sum;
      }
      }
      break;

      case 2:
      {
      get_time();
      sum = String(rawhours) + minutes + seconds;
      if (sum != sum_old){       
        toprint = String(rawhours)+":"+minutes+":"+seconds + "|" + day + "/" + month + "/" + year;
        lcd.clear(); 
        lcd.setCursor(12 - toprint.substring(0, toprint.indexOf("|")).length(), 0);
        lcd.print(toprint.substring(0, toprint.indexOf("|")));
        lcd.setCursor(3,1); lcd.print(toprint.substring(toprint.indexOf("|") +1));
        sum_old = sum;
      }
      }
      break;

      case 3:
      {
      lcd.clear(); lcd.home(); lcd.print(conssid); lcd.setCursor(0,1); lcd.print(WiFi.localIP());
      }
      break;

      case 4:
      {
      lcd.clear(); lcd.home(); lcd.print("utcOffset: "); lcd.print(utcOffsetInSeconds);
      lcd.setCursor(0,1); lcd.print("("); lcd.print(float(intervall_0)/1000, 1); 
      lcd.print("|"); lcd.print(float(intervall_1)/1000, 1); lcd.print("|"); 
      lcd.print(float(intervall_2)/1000, 1); lcd.print(")");
      }
      break;
    }
}

bool connectWIFI(int num){
  char conpassword[20];
  int count = 0;
  bool conStatus = true;
  strcpy(conssid, ssid[num]);
  strcpy(conpassword, password[num]);
  lcd.clear();
  lcd.home();
  lcd.print(conssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(conssid, conpassword);
  Serial.print("Connecting: ");
  Serial.println(conssid);
  lcd.setCursor(0,1);
  while (WiFi.status() != WL_CONNECTED) {
    if (count < 16){
      lcd.print(".");
      Serial.print(".");
      count ++;
      delay(1000);
    }
    else{
      conStatus = false;
      Serial.print("Failed to connect: ");
      Serial.println(conssid);
      break;
    }
  }
  return conStatus;
}

void interrupt_handler()
{
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 100){
    interrupt_func();
 }
 last_interrupt_time = interrupt_time;
}

void interrupt_func(){
  if (mode == 4){
    mode = 0;
  }
  else{
    mode ++;
  }
  Serial.print("Changed Mode to: ");
  Serial.println(mode);
  forcesync = true;
  sum_old = "";
}

void get_time(){
  unsigned long time_current = millis();

  if (time_current -  time_lastcon > 1800000 || time_lastcon == 0){
    sync_time();
  }
  else{
    timeinmilliseconds += time_current - time_last;
    time_last = time_current;
  }
  if(timeinmilliseconds >= 86400000){
    sync_time();
  }
  int temp = 0;
  rawhours = timeinmilliseconds / int(3600000); temp = timeinmilliseconds % 3600000;
  int rawminutes = temp / int(60000); temp = temp % 60000;
  int rawseconds = temp / int(1000); temp = 0;
  minutes = String(rawminutes);
  seconds = String(rawseconds);
  if(rawminutes < 10){
    minutes = "0" + String(rawminutes);
  }
  if(rawseconds < 10){
    seconds = "0" + String(rawseconds);
  }
}

void sync_time(){
  timeClient.update();
  time_lastcon = millis();
  time_last = time_lastcon;
  timeinmilliseconds = 0;
  timeinmilliseconds += timeClient.getHours() * 3600000;
  timeinmilliseconds += timeClient.getMinutes() *60000;
  timeinmilliseconds += timeClient.getSeconds() * 1000;
  String datetime = timeClient.getFormattedDate();
  year = datetime.substring(0, 4);
  month = datetime.substring(5,7);
  day = datetime.substring(8,10);
  Serial.println("synced Time");
  Serial.println(datetime);
}

void get_indoor(){

  temperature = bme.readTemperature();
  humidity = bme.readHumidity();
  pressure = bme.readPressure() / 100;
  altitude = bme.readAltitude(1013.25);

}

void handle_OnConnect() {
  get_indoor();
  server.send(200, "text/html", SendHTML());
}

void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

String SendHTML(){
  String ptr = "<!DOCTYPE html> <html>\n";
  ptr +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\" charset=\"utf-8\">\n";
  ptr +="<title>Tom's Wetterstation</title>\n";
  ptr +="<style>html { font-family: Arial, Helvetica, sans-serif; display: inline-block; margin: 0px auto; text-align: center;}\n";
  ptr +="body{margin-top: 50px;} h1 {margin: 50px auto 30px;}\n";
  ptr +="p {font-size: 24px; margin-bottom: 10px;}\n";
  ptr +="</style>\n"; ptr +="</head>\n"; ptr +="<body>\n"; ptr +="<div id=\"webpage\">\n";
  ptr +="<h1>Tom's Wetterstation</h1>\n"; ptr +="<p>Temperatur: "; ptr +=temperature;
  ptr +="&deg;C</p>"; ptr +="<p>Luftfeuchtigkeit: "; ptr +=humidity; ptr +="%</p>";
  ptr +="<p>Luftdruck: "; ptr +=pressure; ptr +="hPa</p>"; ptr +="<p>Höhe (über NN): ";
  ptr +=altitude; ptr +="m</p>"; ptr +="</div>\n";
  ptr +="</body>\n"; ptr +="</html>\n";
  return ptr;
}
