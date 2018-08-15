#include "Arduino.h";
#include "EEPROM.h";
#include "EtherCard.h";
#include "Vrekrer_scpi_parser.h"
#include "Adafruit_ADS1015.h"

const int eeprom_eth_data_start = 0;
const static byte dns[] = {0, 0, 0, 0};
const static byte mask[] = {255, 255, 255, 0};
byte mac[6] = { 0x74, 0x69, 0x69, 0x2D, 0x30, 0x31 };
byte ip[4] = {192, 168, 10, 7};
byte gw[4] = {192, 168, 10, 1};
byte Ethernet::buffer[128];

const byte csPin = 2; //ChipSelect Pin

SCPI_Parser my_instrument;
boolean fromSerial = true;

Adafruit_ADS1115 ads(0x48);
float VoltsPerBit;

void setup() {
  Serial.begin(9600);
  while (!Serial);
  my_instrument.RegisterCommand(F("*IDN?"), &Identify);
  my_instrument.SetCommandTreeBase(F("SYSTem:COMMunicate:LAN")); 
    my_instrument.RegisterCommand(F(":ADDRess"), &SetIP);
    my_instrument.RegisterCommand(F(":ADDRess?"), &GetIP);
    my_instrument.RegisterCommand(F(":DGATeway"), &SetGW);
    my_instrument.RegisterCommand(F(":DGATeway?"), &GetGW);
    my_instrument.RegisterCommand(F(":MAC"), &SetMAC);
    my_instrument.RegisterCommand(F(":MAC?"), &GetMAC);
  my_instrument.SetCommandTreeBase("");
  my_instrument.RegisterCommand(F("MEASure:VOLTage?"), &GetVoltage);
  my_instrument.RegisterCommand(F("CONFigure:GAIN?"), &GetGain);
  my_instrument.RegisterCommand(F("CONFigure:GAIN"), &SetGain);

//my_instrument.PrintDebugInfo();

  int eeprom_address = eeprom_eth_data_start;
  if (EEPROM.read(eeprom_address) == 'V') { //Already initialized
    //Serial.println("EEPROM OK");
    ++eeprom_address;
    EEPROM.get(eeprom_address, mac);
    eeprom_address += sizeof(mac);
    EEPROM.get(eeprom_address, ip);
    eeprom_address += sizeof(ip);
    EEPROM.get(eeprom_address, gw);
  } else { //Write default values to EEPROM
    EEPROM.write(eeprom_address, 'V');
    ++eeprom_address;
    EEPROM.put(eeprom_address, mac);
    eeprom_address += sizeof(mac);
    EEPROM.put(eeprom_address, ip);
    eeprom_address += sizeof(ip);
    EEPROM.put(eeprom_address, gw);
  }

  ether.hisport = 5025; //SCPI PORT

  boolean eth_enabled = false;
  if (ether.begin(sizeof Ethernet::buffer, mac, csPin))
    eth_enabled = ether.staticSetup(ip, gw, dns, mask);
  if (!eth_enabled) ether.powerDown();

  //ADS1115 Configuration
  pinMode(A2, INPUT);
  pinMode(A3, OUTPUT);
  digitalWrite(A3, LOW);
  ads.begin();
  ads.setGain(GAIN_FOUR);
  VoltsPerBit = 0.03125E-3;

  pinMode(11, OUTPUT);
  analogWrite(11, 25); //0.5V?
}

void loop() {
  fromSerial = true;
  my_instrument.ProcessInput(Serial, "\n");

  if ( ether.isLinkUp() ) {
      fromSerial = false;
      my_instrument.Execute(GetEthMsg(), Serial);
  }
}

/* SCPI FUNCTIONS */

void Identify(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  char IDN[] = "MagDynLab,ArduinoVoltMeter,SN01,V.1.0\n";
  PrintToInterface(IDN);
}

void SetIP(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  SaveIP(parameters.First(), eeprom_eth_data_start + 7 );
}

void GetIP(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  char ip_str[16];
  IpToString(ether.myip, ip_str);
  PrintToInterface( ip_str );
}

void SetGW(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  SaveIP(parameters.First(), eeprom_eth_data_start + 11 );
}

void GetGW(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  char ip_str[16];
  IpToString(ether.gwip, ip_str);
  PrintToInterface( ip_str );
}

void SetMAC(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  int this_mac[6];
  int mac_ok = 0;
  if (parameters.Size() == 6) {
    for (uint8_t i = 0; i < 6; i++) {
      mac_ok += sscanf(parameters[i], "%x", &this_mac[i]);
    }
  }
  int eeprom_address = eeprom_eth_data_start + 1;  
  if (mac_ok == 6) {
    for (uint8_t i = 0; i < 6; i++) {
      EEPROM.write(eeprom_address, byte(this_mac[i]));
      ++eeprom_address;
    }
  }
}

void GetMAC(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  char mac_str[ ] = "0x##, 0x##, 0x##, 0x##, 0x##, 0x##\n";
  for (int i = 0; i < 6; ++i) {
    char u = ether.mymac[i] / 16;
    char l = ether.mymac[i] % 16;
    mac_str[6 * i + 2] = u < 10 ? u + '0' : u + 'A' - 10;
    mac_str[6 * i + 3] = l < 10 ? l + '0' : l + 'A' - 10;
  }
  PrintToInterface(mac_str);
}

void GetVoltage(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  float voltage;
  int16_t adc_dif01;
  adc_dif01 = ads.readADC_Differential_0_1();
  voltage = -1.0 *  adc_dif01 * VoltsPerBit;
  char volt_str[14];
  dtostre(voltage, volt_str, 5, 0x06);
  volt_str[12] = '\n';
  volt_str[13] = '\0';  
  PrintToInterface(volt_str);
}

void GetGain(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  adsGain_t gain = ads.getGain();
  char* gain_strs[] = {"2/3\n", "1\n", "2\n", "4\n", "8\n", "16\n"};
  adsGain_t gain_values[] = {GAIN_TWOTHIRDS, GAIN_ONE, GAIN_TWO, 
                             GAIN_FOUR, GAIN_EIGHT, GAIN_SIXTEEN};
  for (uint8_t i = 0; i < 6; i++) {
    if (gain == gain_values[i]) PrintToInterface(gain_strs[i]);
  }
}

void SetGain(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  char* gain_strs[] = {"2/3", "1", "2", "4", "8", "16"};
  adsGain_t gain_values[] = {GAIN_TWOTHIRDS, GAIN_ONE, GAIN_TWO, 
                             GAIN_FOUR, GAIN_EIGHT, GAIN_SIXTEEN};
  float vpb_values[] = {0.1875E-3, 0.125E-3, 0.0625E-3, 
                        0.03125E-3, 0.015625E-3, 0.0078125E-3};
  for (uint8_t i = 0; i < 6; i++) {
    int c = strcmp(parameters.First(), gain_strs[i]);
    if (c == 0) {
      ads.setGain(gain_values[i]);
      VoltsPerBit = vpb_values[i];
    }
  }
}

/* HELPER FUNCTIONS */

char* GetEthMsg() {
  word pos = ether.packetLoop(ether.packetReceive());
  if (pos) {
    ether.httpServerReplyAck();
    char* msg = Ethernet::buffer + pos;
    msg = strtok(msg, "\n"); //Remove termination char
    return msg;
  } else {
    return NULL;
  }
}

void WriteEthMsg(char* msg) {
  delayMicroseconds(20); //Delay needed for next package
  BufferFiller bfill = ether.tcpOffset();
  bfill.emit_raw(msg, strlen(msg));
  ether.httpServerReply_with_flags(bfill.position(), TCP_FLAGS_ACK_V | TCP_FLAGS_PUSH_V);
}

void PrintToInterface(char* print_str) {
  if (fromSerial)
    Serial.write(print_str, strlen(print_str));
  else
    WriteEthMsg(print_str);
}

void SaveIP(char* ip_str, int eeprom_address) {
  int this_ip[4];
  int ipOk = sscanf(ip_str, "%d.%d.%d.%d", &this_ip[0], &this_ip[1], &this_ip[2], &this_ip[3]);
  if (ipOk == 4) {
    for (uint8_t i = 0; i < 4; i++) {
      EEPROM.write(eeprom_address, byte(this_ip[i]));
      ++eeprom_address;
    }
  }
}

void IpToString(byte* IP, char* ip_str) {
  sprintf(ip_str, "%d.%d.%d.%d\n", IP[0], IP[1], IP[2], IP[3]);
  return ip_str;
}
