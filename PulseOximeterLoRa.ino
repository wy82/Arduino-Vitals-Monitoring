// LoRa Libraries
#include <SPI.h>
#include <RH_RF95.h>

// Pulse Oximeter Libararies
#include <SparkFun_Bio_Sensor_Hub_Library.h>
#include <Wire.h>

// Lora pins
#define RFM95_CS 4
#define RFM95_RST 2
#define RFM95_INT 3

// Pulse Oximeter pins
int resPin = 6;
int mfioPin = 7;

// Takes address, reset pin, and MFIO pin.
SparkFun_Bio_Sensor_Hub bioHub(resPin, mfioPin); 

bioData body;  

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0
 
// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

int16_t packetnum = 0;  // packet counter, we increment per xmission

void setup() {
  // LoRa Setup
  pinMode(RFM95_RST, OUTPUT);                                                                                                         
  digitalWrite(RFM95_RST, HIGH);
 
  while (!Serial);
  Serial.begin(9600);
  delay(100);
 
  Serial.println("Arduino LoRa TX Test!");
 
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
 
  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while(1);
  }
  Serial.println("LoRa radio init OK!");
 
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
 
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
 
  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(5, false);

  // Pulse oximeter setup
  Wire.begin();
  int result = bioHub.begin();
  if (result == 0) //Zero errors!
    Serial.println("Sensor started!");
  else
    Serial.println("Could not communicate with the sensor!!!");
 
  Serial.println("Configuring Sensor...."); 
  int error = bioHub.configBpm(MODE_TWO); // Configuring just the BPM settings. 
  if(error == 0){ // Zero errors
    Serial.println("Sensor configured.");
  }
  else {
    Serial.println("Error configuring sensor.");
    Serial.print("Error: "); 
    Serial.println(error); 
  }

  // Data lags a bit behind the sensor, if you're finger is on the sensor when
  // it's being configured this delay will give some time for the data to catch
  // up. 
  Serial.println("Loading up the buffer with data....");
  delay(4000); 
  
}

void loop() {
  // read pulse oximeter information
  body = bioHub.readBpm();
  
  Serial.println("Sending to rf95_server");
  // Send a message to rf95_server
 
  //char radiopacket[22] = "Goodbye World #      ";
  //itoa(packetnum++, radiopacket+15, 10);
  char heartratepacket[20] = "Heart Rate:         ";
  itoa(body.heartRate, heartratepacket+12, 10);

  char oxygenpacket[20] = "Oxygen:             ";
  itoa(body.oxygen, oxygenpacket+8, 10);

  char confidencepacket[20] = "Confidence:        ";
  itoa(body.confidence, confidencepacket+13, 10);
  
  Serial.print("Sending "); Serial.println(heartratepacket);
  heartratepacket[19] = 0;
  Serial.print("Sending "); Serial.println(oxygenpacket);
  oxygenpacket[19] = 0;
  Serial.print("Sending "); Serial.println(confidencepacket);
  confidencepacket[19] = 0;
 
  Serial.println("Sending..."); delay(10);
  rf95.send((uint8_t *)heartratepacket, 20);
  delay(1000);
  rf95.send((uint8_t *)oxygenpacket, 20);
  delay(1000);
  rf95.send((uint8_t *)confidencepacket, 20);
 
  Serial.println("Waiting for packet to complete..."); delay(10);
  rf95.waitPacketSent();
  // Now wait for a reply
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
 
  Serial.println("Waiting for reply..."); delay(10);
  if (rf95.waitAvailableTimeout(1000))
  {
    // Should be a reply message for us now   
    if (rf95.recv(buf, &len))
   {
      Serial.print("Got reply: ");
      Serial.println((char*)buf);
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);   
    }
    else
    {
      Serial.println("Receive failed");
    }
  }
  else
  {
    Serial.println("No reply, is there a listener around?");
  }
  delay(1000);
}
