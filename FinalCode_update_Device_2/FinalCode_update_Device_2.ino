
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include <SoftwareSerial.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <String.h>

int snr1[2]={15,12}, snr2[2]={15,14};
int object, totalObject;
int counter = 0, hour, preHour, fail = 0;
String date, preDate;
float d1, d2, sonardist = 27400;


//Time
const long utcOffsetInSeconds = 3600;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

// Set these to run example.
#define FIREBASE_HOST "toll-df5a2.firebaseio.com"
#define FIREBASE_AUTH "ecHpFUsq9GqIL7CMcCWQFiRiW0hrtjwjJYfjqcbS"
#define WIFI_SSID "Toll"
#define WIFI_PASSWORD "toll12341234"


void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  timeClient.begin();

  pinMode(snr1[0], OUTPUT);
  pinMode(snr1[1], INPUT);
  pinMode(snr2[1], INPUT);
  
}


//////////////////Sonar Read/////////////////////////
float sonarRead(int pin[2]) {
  float duration, lengthCM = 0;
  digitalWrite(pin[0], LOW);
  delayMicroseconds(5);
  digitalWrite(pin[0], HIGH);
  delayMicroseconds(10);
  digitalWrite(pin[0], LOW);
  duration = pulseIn(pin[1], HIGH, sonardist);
  lengthCM = duration / 58.2;
  return lengthCM;
}


void sensor()
{
  float s1 = 0, s2 = 0;
  int n = 10;
  for(int i=0; i<n; i++)
  {
    if(sonarRead(snr1) < 5)
      s1++;
    if(sonarRead(snr2) < 5)
      s2++; 
  }
  if(s1 > 5)
    d1 = 0;
  else 
    d1 = 1;
  if(s2 > 5)
    d2 = 0;
  else 
    d2 = 1;
}


//Main loop
void loop() {
  // Extract time
  timeClient.update();
  hour = timeClient.getHours()+5;
  if(hour >= 24)
    hour = hour - 24;
  Serial.println(hour);
  
  // Date calculation
  unsigned long epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime); 
  int monthDay = ptm->tm_mday;
  int currentMonth = ptm->tm_mon+1;
  int currentYear = ptm->tm_year+1900;
  date = String(monthDay) + "_" + String(currentMonth) + "_" + String(currentYear);
  Serial.println(date);
  // soner read 
  sensor(); 
  if(d1 == 0 || d2 == 0){
     delay(50);
     sensor();
     if(d1 == 0 || d2 == 0){
       delay(100);
       sensor();
    }
  }

  Firebase.setFloat("Sonardata/Soner3", d1);
  if (Firebase.failed()) {
      Serial.print("Set data /logs failed:");
      Serial.println(Firebase.error());  
      fail = 1;
  }
  if(fail == 0)
  {
    Firebase.setFloat("Sonardata/Soner4", d2);
    sonardist = Firebase.getInt("Sonardata/Soner");
    Serial.println(sonardist);
  }
   

  if(d1 != 0 && d2 != 0 && counter == 0){
    object = object + 1;
    totalObject = totalObject + 1;
    counter = 1;
  }
  else if((d1 == 0 || d2 == 0) && counter == 1){
    counter = 0;
    return;
  }
  else 
    return;
  
  Serial.print("object = ");
  Serial.println(object);
  if(fail == 1){
    fail = 0;
    return;
  }

  
//Object calculation  
  if(date == "1_1_1970")
    return;
    
  if(hour != preHour)
  {
    preHour = hour;
    object = 1;
  }

  if(date != preDate)
  {
    preDate = date;
    totalObject = 1;
  }

// Path calculation
  String today = "device2/today/" + String(hour) + "/number";
  String todayTime = "device2/today/" + String(hour) + "/time";
  String todayDate = "device2/today/" + String(hour) + "/date";
  
  String historyNumber = "device2/history/" + date + "/number";
  String historyTime = "device2/history/" + date + "/time";
  String historyDate = "device2/history/" + date + "/date";
  Serial.println(today);
  
  // History date update
  int getNumber = Firebase.getInt(historyNumber);
  if (Firebase.failed()) {
    Firebase.setInt(historyTime, hour);
    Firebase.setString(historyDate, date);
    Firebase.setInt(historyNumber, totalObject);
  }
  else if(totalObject == 0)
    return;
  else{
    if(totalObject == 1 || (totalObject <= getNumber))
      totalObject = totalObject + getNumber; 
    Firebase.setInt(historyTime, hour);
    Firebase.setString(historyDate, date);
    Firebase.setInt(historyNumber, totalObject);
  }

  //today data update
  String getDate = Firebase.getString(todayDate);
  if (Firebase.failed()) {
    Firebase.setInt(today, object);
    Firebase.setInt(todayTime, hour);
    Firebase.setString(todayDate, date);
  }
  else
  {
    if(getDate == date)
    {
      getNumber = Firebase.getInt(today);
      if(object == 1 || (object <= getNumber))
        object = object + getNumber;
      else if(object == 0)
        return;
      Firebase.setInt(today, object);
      Firebase.setInt(todayTime, hour);
      Firebase.setString(todayDate, date);
    }
    else
    {
      Firebase.setInt(today, object);
      Firebase.setString(todayDate, date);
      Firebase.setInt(todayTime, hour);
    }
  }
 
  if (Firebase.failed()) {
      Serial.print("Set data /logs failed:");
      Serial.println(Firebase.error());  
      return;
  }
}



