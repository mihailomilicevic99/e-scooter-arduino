

#include <Wire.h>
#include <Adafruit_PN532.h>


#define PN532_SCL  A5
#define PN532_SDA   A4

Adafruit_PN532 nfc(PN532_SCL, PN532_SDA);



void setup(void) {
  Serial.begin(115200);
  while (!Serial) delay(10);

  //Serial.println("Looking for PN532...");

  nfc.begin();
  

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    //Serial.print("Didn't find PN53x board");
    while (1); // halt
  }

  //Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
  //Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
  //Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);

  //Serial.println("Waiting for an ISO14443A Card ...");
}


void loop(void) {
  uint8_t success;                          // Flag to check if there was an error with the PN532
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  uint8_t currentblock;                     // Counter to keep track of which block we're on
  bool authenticated = false;               // Flag to indicate if the sector is authenticated
  uint8_t data[16];                         // Array to store block data during reads
  String message;                           //message received by mobile app

  // Keyb on NDEF and Mifare Classic should be the same
  uint8_t keyuniversal[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

  
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) {
    // Display some basic information about the card
    //Serial.println("Found an ISO14443A card");
    //Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    //Serial.print("  UID Value: ");
    //nfc.PrintHex(uid, uidLength);
    //Serial.println("");

   

      // Now we try to go through 5 blocks
      for (currentblock = 0; currentblock < 6; currentblock++)
      {
        // Check if this is a new block so that we can reauthenticate
        if (nfc.mifareclassic_IsFirstBlock(currentblock)) authenticated = false;

        // If the sector hasn't been authenticated, do so first
        if (!authenticated)
        {
          // Starting of a new sector ... try to to authenticate
          //Serial.print("------------------------Sector ");Serial.print(currentblock/4, DEC);Serial.println("-------------------------");
         
          success = nfc.mifareclassic_AuthenticateBlock (uid, uidLength, currentblock, 1, keyuniversal);
          
          if (success)
          {
            authenticated = true;
          }
          else
          {
            //Serial.println("Authentication error");
          }
        }
        // If we're still not authenticated just skip the block
        if (!authenticated)
        {
         // Serial.print("Block ");Serial.print(currentblock, DEC);Serial.println(" unable to authenticate");
        }
        else
        {
          // Authenticated ... we should be able to read the block now
          // Dump the data into the 'data' array
          success = nfc.mifareclassic_ReadDataBlock(currentblock, data);
          if (success)
          {
            // Read successful
            //Serial.print("Block ");Serial.print(currentblock, DEC);
            //if (currentblock == 10)
            
           // Serial.print("  ");
            
            
            // Dump the raw data
            //nfc.PrintHexChar(data, 16);

            if(currentblock == 4){
              message = "";
              for (int i = 11; i < 16; i++) {
                message = message + char(data[i]);
              }
            }

            if(currentblock == 5){
              for (int i = 0; i < 6; i++) {
                message = message + char(data[i]);
              }
              Serial.println(message);
            }

            


          }
          else
          {
            // Oops ... something happened
            Serial.print("Block ");Serial.print(currentblock, DEC);
            Serial.println(" unable to read this block");
          }
        }
      }
    
    
  }
  // Wait a bit before trying again
  delay(2000);
}


