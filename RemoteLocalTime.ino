/* NOTE THIS IS HARD-CODED FOR MY TIMEZONE
 *  AND A SECOND TIME-ZONE WITH FAMILY.
 *  Adjust to your world!
 */
#include <WiFi.h>
#include "time.h"

const char* ssid       = "YOUR WIFI SSID";
const char* password   = "YOUR PASSWORD";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -6 * 60 * 60;
const int   daylightOffset_sec = 3600;
int  remote_offset = 14 * 60 * 60;

// For LCD
char ESC = 0xFE; // Used to issue command
const char CLS = 0x51;
const char CURSOR = 0x45;
const char LINE1_POS = 0x00;
const char LINE2_POS = 0x40;
const char LINE3_POS = 0x14;
const char LINE4_POS = 0x54;

//  THIS IS WHAT THE DATE STRING LOOKS LIKE
// "MON 12:13:31 PM"   -
char timeBuffer[21];
String lastLocalTimeStr;
String lastRemoteTimeStr;
int lastMillis = 0;
const int updateTimeFromNetMillis = 24 * 60 * 60 * 1000; // once a day

void dumpTimeInfo(String msg, struct tm* timeinfo) {
  Serial.println(msg);
  Serial.print("tm_sec:");
  Serial.println(timeinfo->tm_sec);
  Serial.print("tm_min:");
  Serial.println(timeinfo->tm_min);
  Serial.print("tm_hour:");
  Serial.println(timeinfo->tm_hour);
  Serial.print("tm_mday:");
  Serial.println(timeinfo->tm_mday);
  Serial.print("tm_mon:");
  Serial.println(timeinfo->tm_mon);
  Serial.print("tm_year:");
  Serial.println(timeinfo->tm_year);
  Serial.print("tm_isdst:");
  Serial.println(timeinfo->tm_isdst);
}

int getUpdatePos(String newDate, String oldDate) { 
  if(newDate.length() != oldDate.length()) {
    return 0; // Update everything
  }
  for(int x = 0; x < newDate.length();x++) {
    if(newDate.charAt(x) != oldDate.charAt(x)) {
      return x;
    }
  }

  return -1; // No change
}

void setupScreen() {
  // Initialize port
  Serial2.begin(9600);
  
  // Initialize LCD module
  Serial2.write(ESC);
  Serial2.write(0x41);
  Serial2.write(ESC);
  Serial2.write(0x51);
  
  // Set Contrast
  Serial2.write(ESC);
  Serial2.write(0x52);
  Serial2.write(40);
  
  // Set Backlight
  Serial2.write(ESC);
  Serial2.write(0x53);
  Serial2.write(8);
  
  Serial2.print("NKC Electronics");
  
  // Set cursor line 2, column 0
  Serial2.write(ESC);
  Serial2.write(CURSOR);
  Serial2.write(LINE2_POS);
  
  Serial2.print("20x4 Serial LCD");
  
  delay(10);
}

void updateScreen(const char updatePos, const char* textToUpdate) {
    Serial2.write(ESC);
    Serial2.write(CURSOR);
    Serial2.write(updatePos);
    Serial2.print(textToUpdate);
}

void printScreen(const char* line1, const char* line2,
  const char* line3, const char* line4) {
  // Clear screen
  Serial2.write(ESC);
  Serial2.write(CLS);
  
  // Print line1 if present
  if(line1 != NULL) {
    Serial2.print(line1);
  }

  // Print line2 if present
  if(line2 != NULL) {
    Serial2.write(ESC);
    Serial2.write(CURSOR);
    Serial2.write(LINE2_POS);
    Serial2.print(line2);
  }

  // Print line3 if present
  if(line3 != NULL) {
    Serial2.write(ESC);
    Serial2.write(CURSOR);
    Serial2.write(LINE3_POS);
    Serial2.print(line3);
  }
  
  // Print line4 if present
  if(line4 != NULL) {
    Serial2.write(ESC);
    Serial2.write(CURSOR);
    Serial2.write(LINE4_POS);
    Serial2.print(line4);
  }
}

String getLocalTimeStr()
{
  struct tm timeinfo;
  
  if(!getLocalTime(&timeinfo)){
    return "";
  }

  //dumpTimeInfo("Local Time: ", &timeinfo);

  strftime(timeBuffer, sizeof(timeBuffer), "%a %I:%M:%S %p", &timeinfo);
  return  "  " + String(timeBuffer);
}


String getRemoteTimeStr()
{
  struct tm timeinfo;
  int dstAdjust = 0;
  
  if(!getLocalTime(&timeinfo)){
    return "";
  }

  if(timeinfo.tm_isdst) {
    dstAdjust = -1;
  }

  // Now add 14 hours
  time_t remoteTime = mktime(&timeinfo) + remote_offset + dstAdjust;

  struct tm* remoteInfo = localtime(&remoteTime);

  //dumpTimeInfo("Local Time: ", remoteInfo);
  
  strftime(timeBuffer, sizeof(timeBuffer), "%a %I:%M:%S %p", remoteInfo);
  return "  " + String(timeBuffer);
}

void updateLocalTimeFromNet() {
  printScreen("Updating from net",NULL,NULL,NULL);
  delay(100);
  
  //connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println(" CONNECTED");
  
  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  firstTimeDisplay();
  
  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

void setup()
{
  Serial.begin(115200);
  timeBuffer[20]='\0';
  setupScreen();

  updateLocalTimeFromNet();
}

void firstTimeDisplay() {
  String localTime = getLocalTimeStr();
  String remoteTime = getRemoteTimeStr();

  lastLocalTimeStr = localTime;
  lastRemoteTimeStr = remoteTime;
  
  printScreen("Bellville: ", localTime.c_str(), "Hagonoy:", remoteTime.c_str());
}

// Only update what has changed to lessen flicker
void updateTimeDisplay() {
  String localTimeStr, remoteTimeStr;
  
  // Get latest time updates
  localTimeStr = getLocalTimeStr();
  remoteTimeStr = getRemoteTimeStr();

  if(localTimeStr == "" || remoteTimeStr == "") {
    Serial.println("Error retrieving time");
    return;
  }

  int localUpdatePos = getUpdatePos(localTimeStr, lastLocalTimeStr);
  int remoteUpdatePos = getUpdatePos(remoteTimeStr, lastRemoteTimeStr);
   
  if(localUpdatePos != -1) {
   
    updateScreen(LINE2_POS + localUpdatePos, localTimeStr.substring(localUpdatePos).c_str());
  }

  if(remoteUpdatePos != -1) {
    updateScreen(LINE4_POS + remoteUpdatePos, remoteTimeStr.substring(remoteUpdatePos).c_str());
  }

  lastLocalTimeStr = localTimeStr;
  lastRemoteTimeStr = remoteTimeStr; 
}

void loop()
{
  int curMillis = millis();

  if( (curMillis < lastMillis) // Millis have rolled over back to 0 
      || ((curMillis - lastMillis) > updateTimeFromNetMillis)) {
    lastMillis = curMillis;
    updateLocalTimeFromNet();
  }
  
  delay(200);
  updateTimeDisplay();
}
