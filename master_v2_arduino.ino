
#include <math.h>



String old_longitude = "";  //tracking longitude changes
String old_latitude = "";   //tracking latitude changes
bool blocked = true;        //scooter locked/unlocked
int counter = 0;            //counts loop cycles - to avoid sendign too many http requests


// Function to extract a field from a string based on a delimiter
String getField(String data, char separator, int index) {
  int fieldCount = 0;
  int i = 0;
  String field = "";
  
  while (i < data.length()) {
    if (data[i] == separator) {
      fieldCount++;
    }
    else if (fieldCount == index) {
      field += data[i];
    }
    if (fieldCount > index) {
      break;
    }
    i++;
  }
  
  return field;
}


float roundToDecimalPlaces(float value, int decimalPlaces) {
  float multiplier = pow(10, decimalPlaces);
  return round(value * multiplier) / multiplier;
}


bool sendCommandAndWaitForReply(const char *command, const char *expectedReply, unsigned int timeout = 2000)
{
  Serial.println(command);
  Serial1.println(command);
  unsigned long startTime = millis();
  String reply = "";

  while (millis() - startTime < timeout)
  {
    if (Serial1.available())
    {
      char c = Serial1.read();
      reply += c;
      if (reply.endsWith(expectedReply))
      {
        return true;
      }
    }
  }
  Serial.println(reply);
  return false;
}



void sendCoordinatesViaGsm(float longitude, float latitude){

  
      while (!sendCommandAndWaitForReply("AT+HTTPPARA=\"URL\",\" https://fb87-87-116-163-22.ngrok-free.app/api/scooters/gps\"", "OK")){}
      delay(1000);
      while (!sendCommandAndWaitForReply("AT+HTTPPARA=\"CONTENT\",\"application/json\"", "OK")){}
      delay(1000);
      while (!sendCommandAndWaitForReply("AT+HTTPDATA=33,10000", "DOWNLOAD"))
      delay(1000);
      while (!sendCommandAndWaitForReply("{\"id\":1,\"lat\":latitude,\"long\":longitude}", "OK")){}
      delay(1000);
      if (sendCommandAndWaitForReply("AT+HTTPACTION=2,1", "+HTTPACTION: 0,200", 10000))
      {
          // HTTP GET request successful (response code 200)
          Serial.println("HTTP GET request successful!");
          if (sendCommandAndWaitForReply("AT+HTTPREAD", "OK"))
          {
            while (Serial1.available())
            {
              Serial.write(Serial1.read());
            }
          }
      }
      else
      {
        // HTTP GET request failed or received a non-200 response code
        Serial.println("HTTP GET request failed!");
      }
      
  }



String getToken(){
  String response = "";
  
      while (!sendCommandAndWaitForReply("AT+HTTPPARA=\"URL\",\" https://fb87-87-116-163-22.ngrok-free.app/api/scooters/token/1\"", "OK")){}
      delay(1000);
      if (sendCommandAndWaitForReply("AT+HTTPACTION=0", "+HTTPACTION: 0,200", 10000))
      {
          // HTTP GET request successful (response code 200)
          Serial.println("HTTP GET request successful!");
          if (sendCommandAndWaitForReply("AT+HTTPREAD", "OK"))
          {
            while (Serial1.available())
            {
              response = response + Serial1.read();
              Serial.write(Serial1.read());
            }
            return response;
          }
      }
      else
      {
        // HTTP GET request failed or received a non-200 response code
        Serial.println("HTTP GET request failed!");
        return "";
      }
      
  }



  bool deleteToken(){
    while (!sendCommandAndWaitForReply("AT+HTTPPARA=\"URL\",\" https://fb87-87-116-163-22.ngrok-free.app/api/scooters/token/1\"", "OK")){}
      delay(1000);
      while (!sendCommandAndWaitForReply("AT+HTTPPARA=\"CONTENT\",\"application/json\"", "OK")){}
      delay(1000);
      while (!sendCommandAndWaitForReply("AT+HTTPDATA=12,10000", "DOWNLOAD"))
      delay(1000);
      while (!sendCommandAndWaitForReply("{\"token\":\"\"}", "OK")){}
      delay(1000);
      if (sendCommandAndWaitForReply("AT+HTTPACTION=2,1", "+HTTPACTION: 0,200", 10000))
      {
          // HTTP GET request successful (response code 200)
          Serial.println("HTTP GET request successful!");
          if (sendCommandAndWaitForReply("AT+HTTPREAD", "OK"))
          {
            while (Serial1.available())
            {
              Serial.write(Serial1.read());
            }
            return true;
          }
      }
      else
      {
        // HTTP GET request failed or received a non-200 response code
        Serial.println("HTTP GET request failed!");
        return false;
      }
  }



// Function to check if a point is inside a polygon
bool checkForBlackZone(float longitude, float latitude){
  //Tasmajdan coords
  float pentagonVerticesX[] = {44.8055, 44.8091, 44.8091, 44.8055, 44.8038};  //latitude
  float pentagonVerticesY[] = {20.4652, 20.4652, 20.4842, 20.4842, 20.4805};  // longitude
  int numVertices = 5;                                                        // Number of vertices in the pentagon

  //Zeleno brdo coords
  //float pentagonVerticesX[] = {44.7676992, 44.795, 44.801, 44.794, 44.788};     //latitude
  //float pentagonVerticesY[] = {20.5317853333, 20.530, 20.510, 20.495, 20.500};  // longitude



  bool isInside = false;
  int j = numVertices - 1;

  for (int i = 0; i < numVertices; i++) {
    if ((pentagonVerticesY[i] < longitude && pentagonVerticesY[j] >= longitude) ||
        (pentagonVerticesY[j] < longitude && pentagonVerticesY[i] >= longitude)) {
      if (pentagonVerticesX[i] + (longitude - pentagonVerticesY[i]) /
              (pentagonVerticesY[j] - pentagonVerticesY[i]) *
              (pentagonVerticesX[j] - pentagonVerticesX[i]) <
          latitude) {
        isInside = !isInside;
      }
    }
    j = i;
  }
  return isInside;
}


float calculateDecimalCoords(String coord){
  if(coord.length()==10){
    int degrees = coord.substring(0, 2).toInt();
    float minutes = coord.substring(2).toFloat();
    return degrees + minutes / 60.0;
  }
  else{
    if(coord.length()==11){
      int degrees = coord.substring(1, 3).toInt();
    float minutes = coord.substring(2).toFloat();
    return degrees + minutes / 60.0;
    }
  }
}






  

void setup() {
  Serial.println("Hello");
  pinMode(6, INPUT_PULLUP);   //button
  pinMode(5, OUTPUT);         //motor controller
  Serial1.begin(9600);        // GPRS interface
  Serial.begin(9600);         // GPS interface
  Serial2.begin(115200);      //interfaace with Arduino Uno
  blocked = true;
  counter = 0;




  //--------------------------------------------------------------------------------------------------
  //------------------------------------------ GSM -----------------------------------------------
  //--------------------------------------------------------------------------------------------------

  if (Serial1.available()){
    Serial.write(Serial1.read());
  }

  //Reset GSM module
  sendCommandAndWaitForReply("AT+CIPSHUT", "OK");
  sendCommandAndWaitForReply("AT+HTTPTERM", "OK");

  // Function to send AT command and wait for the expected reply
  while (!sendCommandAndWaitForReply("AT+SAPBR=3,1,\"Contype\",\"GPRS\"", "")){}
  delay(1000);
  while (!sendCommandAndWaitForReply("AT+CSTT=\"internet\",\"internet\",\"internet\"", "OK")){}
  delay(1000);
  while (!sendCommandAndWaitForReply("AT+SAPBR=1,2", "OK")){}
  delay(1000);
  while (!sendCommandAndWaitForReply("AT+HTTPINIT", "OK")){}
  delay(1000);
  while (!sendCommandAndWaitForReply("AT+HTTPSSL=1", "OK"))
  delay(1000);
  while (!sendCommandAndWaitForReply("AT+HTTPPARA=\"CID\",1", "OK"))
  delay(1000);
  Serial.println("Gsm configured properly");
           




  //--------------------------------------------------------------------------------------------------
  //------------------------------------------ END GSM -----------------------------------------------
  //--------------------------------------------------------------------------------------------------



}

void loop() {


  //--------------------------------------------------------------------------------------------------
  //---------------------------------------------- BUTTON AND MOTOR ----------------------------------
  //--------------------------------------------------------------------------------------------------

  int pinValue = digitalRead(6);
  if(pinValue == 0 && blocked==false){
    digitalWrite(5, HIGH);
  } else {
    digitalWrite(5, LOW);
  }

  //--------------------------------------------------------------------------------------------------
  //------------------------------------------ END BUTTON AND MOTOR ----------------------------------
  //--------------------------------------------------------------------------------------------------
  

  //--------------------------------------------------------------------------------------------------
  //---------------------------------------------- GPS -----------------------------------------------
  //--------------------------------------------------------------------------------------------------
  if (Serial.available()) {
    // Example sentence: $GPGGA,092750.000,5321.6802,N,00630.3372,W,1,8,1.03,61.7,M,55.2,M,,*76
    String gpsData = Serial.readStringUntil('\n'); // Read a line of GPS data
    if (gpsData.startsWith("$GPRMC")) {
      // Parse GPS data to extract latitude and longitude
      String latitude = getField(gpsData, ',', 3);
      String longitude = getField(gpsData, ',', 5);

      float latitude_decimal = calculateDecimalCoords(latitude);
      float longitude_decimal = calculateDecimalCoords(longitude);

      if(roundToDecimalPlaces(latitude.toFloat() , 2) != roundToDecimalPlaces(old_latitude.toFloat() , 2) || roundToDecimalPlaces(longitude.toFloat() , 2) != roundToDecimalPlaces(old_longitude.toFloat() , 2)){
        Serial.println("Sending coordinates...");
        
       
        sendCoordinatesViaGsm(roundToDecimalPlaces(latitude_decimal , 4), roundToDecimalPlaces(longitude_decimal , 4));
        if(checkForBlackZone(roundToDecimalPlaces(longitude_decimal , 4), roundToDecimalPlaces(latitude_decimal , 4))==true){
          Serial.println("In the black zone");
          blocked=true;
        }
        else{
          Serial.println("Outside of the black zone");
          blocked=false;  
        }
        
      }

      old_latitude = latitude;
      old_longitude = longitude;
      Serial.println(gpsData);
      Serial.print("Latitude: ");
      Serial.println(latitude);
      Serial.print("Longitude: ");
      Serial.println(longitude);
    }
  }

  //--------------------------------------------------------------------------------------------------
  //------------------------------------------ END GPS -----------------------------------------------
  //--------------------------------------------------------------------------------------------------




  //--------------------------------------------------------------------------------------------------
  //------------------------------------------ NFC unlocking -----------------------------------------------
  //--------------------------------------------------------------------------------------------------

  if(Serial2.available()){
    String nfcMessage = Serial2.readStringUntil('\n');
    nfcMessage.trim();
    Serial.println(nfcMessage);

    /*if(nfcMessage == "NFC mobile!"){
      Serial.println("Unblocking...");
      blocked = false;
    }*/

    String ref_token = getToken();
    if(message==ref_token){
      if(deleteToken()==true){
        blocked = false;
      }
      else{
        blocked = true;
      }
    }
    else{
      blocked = true;
    }
  }



  //--------------------------------------------------------------------------------------------------
  //------------------------------------------ End NFC unlocking -----------------------------------------------
  //--------------------------------------------------------------------------------------------------




  //--------------------------------------------------------------------------------------------------
  //----------------------------------------------locking scooter-------------------------------------
  //--------------------------------------------------------------------------------------------------


  if(blocked==false){
    if(counter == 15){
      counter = 0;
      String ref_token = getToken();
      if(ref_token!=""){
        if(deleteToken()==true){
          blocked = true;
        }
        else{
          blocked  = false;
        }
      }
    }
    else{
      counter = counter + 1;
    }
  }
  else{
    counter = 0;
  }


  //--------------------------------------------------------------------------------------------------
  //--------------------------------------------End locking scooter-------------------------------------
  //--------------------------------------------------------------------------------------------------

 

}









