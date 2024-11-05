#include <ChainableLED.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include "DS1307.h"
#include <SD.h>
#include <EEPROM.h>
#define NUM_LEDS 1
#define BOUTON_VERT 2
#define BOUTON_ROUGE 3
#define adresseI2CduBME280
Adafruit_BME280 bme;
DS1307 clock;

//SD
const int sd_model = 4;
int FILE_MAX_SIZE = 2048;
int revision = 0;
int num_copie = 0;
String Fichier = "";

//GPS
SoftwareSerial SoftSerial(2,3);
String DonneesGPS;
bool cond;

enum Mode {
    STANDARD,
    MAINTENANCE,
    CONFIGURATION,
    ECONOMIQUE
};
Mode modeActuel;
Mode modePrecedent;

ChainableLED leds (7, 8, NUM_LEDS);
volatile bool flag = false;
volatile bool flag2 =false;
unsigned long debut = 0;
unsigned long debut2 = 0;
unsigned long temp = 0;
const int light_sensor = A3;

const char standard[] PROGMEM = "Mode Standard";//0
const char configuration[] PROGMEM = "Mode Configuration";//1
const char maintenance[] PROGMEM = "Mode Maintenance";//2
const char economique[] PROGMEM = "Mode Economie";//3
const char retourStandard[] PROGMEM = "Passage automatique en mode Standard après 30 minutes d'inactivité";//4
const char luminosite[] PROGMEM = "Valeur de luminosité :  ";//5
const char erreurLuminosite[] PROGMEM = "Erreur de communication avec le capteur luminosite !";//6
const char erreurBME[] PROGMEM = "Erreur de communication avec le capteur BME280 !";//7
const char errorNoCommand[] PROGMEM = "Erreur : aucune commande fournie.";//8
const char errorInvalidDate[] PROGMEM = "Erreur : Valeurs de date invalides.";//9
const char errorInvalidCommand[] PROGMEM = "Commande non reconnue.";//10
const char resetMessage[] PROGMEM = "Paramètres réinitialisés.";//11
const char erreurDate[] PROGMEM = "Erreur : Format de date invalide. Utilisez JJ/MM/AAAA.";//12
const char version[] PROGMEM = "Version : 1.0.0, Lot : 001";//13
const char errorStockage[] PROGMEM = "mémoire pleine, archivage";//14
const char errorOuverture[] PROGMEM = "Erreur ouverture du fichier de données";//15
const char initCarteSd[] PROGMEM = "Initialisation carte SD\n";//16
const char errorSdInit[] PROGMEM = "Mauvaise ou aucune carte détéctée";//17
const char carteSdInit[] PROGMEM = "Carte initialisée";//18
const char erreurHeure[] PROGMEM = "Erreur : Format d'heure invalide. Utilisez HH:MM:SS.";//19
const char erreurFormat[] PROGMEM = "Erreur : Heure, minute ou seconde invalide.";//20
const char erreurJour[] PROGMEM = "Erreur : Jour invalide. Utilisez MON, TUE, WED, THU, FRI, SAT, SUN.";//21

const char* const messages[] PROGMEM = {standard, configuration, maintenance, economique, retourStandard,luminosite,erreurLuminosite, erreurBME, errorNoCommand, errorInvalidDate,
 errorInvalidCommand, resetMessage, erreurDate, version, errorStockage, errorOuverture, initCarteSd, errorSdInit,carteSdInit, erreurHeure, erreurFormat, erreurJour};

void afficherMessage(int index) {
  char buffer[50];
  strcpy_P(buffer, (char*)pgm_read_word(&(messages[index])));
  Serial.println(buffer);
}
void parametre(){
  EEPROM.put(1, 10000);
  EEPROM.put(2, 2000);
  EEPROM.write(3, 30000);
  EEPROM.write(4, 1);
  EEPROM.write(5, 255);
  EEPROM.put(6, 768);
  EEPROM.write(7, 1);
  EEPROM.write(8, -10);
  EEPROM.write(9, 60);
  EEPROM.write(10, 1);
  EEPROM.write(11, 0);
  EEPROM.write(12, 50);
  EEPROM.write(13, 1);
  EEPROM.put(14, 850);
  EEPROM.put(15, 1080);
  EEPROM.put(16, 2024);
  EEPROM.put(17, 1);
  EEPROM.put(18, 1);
  EEPROM.put(19, "Lundi");
  EEPROM.put(20, 12);
  EEPROM.put(21, 0);
  EEPROM.put(22, 0);
}

void basculer() {
  if(digitalRead(BOUTON_ROUGE) == LOW){
  debut = millis();
  flag = !flag;
  }
}
void basculer2() {
  if(digitalRead(BOUTON_VERT) == LOW){
  debut2 = millis();
  flag2 = !flag2;
  }
}
bool AppuiLong(volatile unsigned long &dernierAppui, volatile bool &appui) {
     while (digitalRead(BOUTON_ROUGE) == LOW || digitalRead(BOUTON_VERT) == LOW){
        if (appui && millis() - dernierAppui >= 5000) {
            delay(50);
            appui = false;
            return true;
        }
    }
    return false;
}
void erreur_capt() {
    leds.setColorRGB(0, 255, 0, 0);
    delay(1000);
    leds.setColorRGB(0, 0, 255, 0);
    delay(1000);
}
String gps()
{
  String DonneesGPS = "";
  if (SoftSerial.available()){
    cond = true;
    while(cond){
      DonneesGPS += SoftSerial.readStringUntil('\n');
      if (DonneesGPS.startsWith("$GPGGA",0)){
        cond  = false;
      }
    }
    return DonneesGPS;
  }
  else {
    while (SoftSerial.available() !=0){
    }
  }
}
void capteurLuminosite(){
  int light = analogRead(light_sensor);
  Serial.print(F("Luminosité : ")); 
  Serial.println(light);
  Serial.println();
}

void bmesensor(){
  while (!bme.begin(0x76)) {
    afficherMessage(7);
    erreur_capt();
  }
  if(EEPROM[4] == 1){
    float temperature = bme.readTemperature();
    Serial.print("Temperature = ");
    Serial.print(temperature);
    Serial.println(" °C");
  }
  if(EEPROM[7] == 1){
    float humidity = bme.readHumidity();
    Serial.print("Humidity = ");
    Serial.print(humidity);
    Serial.println(" %");
  }

  if(EEPROM[10] == 1){
    float pressure = bme.readPressure() / 100.0F;
    Serial.print("Pressure = ");
    Serial.print(pressure);
    Serial.println(" hPa");
  }
}
void ClockDS1307()
{
    clock.getTime();
    Serial.print(clock.hour, DEC);
    Serial.print(F(":"));
    Serial.print(clock.minute, DEC);
    Serial.print(F(":"));
    Serial.print(clock.second, DEC);
    Serial.print(F("  "));
    Serial.print(clock.month, DEC);
    Serial.print(F("/"));
    Serial.print(clock.dayOfMonth, DEC);
    Serial.print(F("/"));
    Serial.print(clock.year+2000, DEC);
    Serial.println(F(" "));
}

String createFile(int num){
  String Log_Name ="";
  Log_Name += clock.year;
  Log_Name += clock.month;
  Log_Name += clock.dayOfMonth;
  Log_Name += num;
  Serial.println(Log_Name);
  return Log_Name;
} 
void enregistrement_SD(){
  if (Fichier == ""){
    Fichier = createFile(revision);
  }
  int light = analogRead(light_sensor);
  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F;

  String DonneesString = "";  
  DonneesString += gps();
  DonneesString += "    Luminosite : ";
  DonneesString += light;
  DonneesString += "    Temperature = ";
  DonneesString += temperature;
  DonneesString += " °C    ";
  DonneesString += "    Humidité = ";
  DonneesString += humidity;
  DonneesString += " %     ";
  DonneesString += "    Pression = ";
  DonneesString += pressure;
  DonneesString += " hPa   ";
  File fichierDonnees = SD.open(Fichier, FILE_WRITE);
  if (SD.exists(Fichier)){
    if (fichierDonnees.size() < FILE_MAX_SIZE){
        fichierDonnees.println(DonneesString);
        fichierDonnees.close();
    }
    else {
        afficherMessage(14);
        num_copie ++;
        String Fichier_archive = createFile(revision+num_copie);
        if (SD.exists(Fichier_archive)){
            File archive = SD.open(Fichier_archive, FILE_WRITE);
            archive.println(fichierDonnees.read());
            archive.close();
        }
        Fichier = createFile(revision+num_copie);
    }
  }
  else {
    afficherMessage(15);
  }
}
void setup()
{
  Serial.begin(9600);
  leds.init();
  pinMode(BOUTON_ROUGE,INPUT_PULLUP);
  pinMode(BOUTON_VERT,INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BOUTON_ROUGE),basculer,CHANGE);
  attachInterrupt(digitalPinToInterrupt(BOUTON_VERT),basculer2,CHANGE);
  SoftSerial.begin(9600);
  clock.begin();
  clock.fillByYMD(2024,10,25);  //Mettre AAAA/MM/JJ
  clock.fillByHMS(10,28,30);    //Mettre HH/MM/SS
  clock.fillDayOfWeek(FRI);
  clock.setTime();
  afficherMessage(16);
  if (!SD.begin(sd_model)) {
    afficherMessage(17);
    while (true);
  }
  afficherMessage(18);

  if (digitalRead(BOUTON_ROUGE) == LOW){
    modeActuel = CONFIGURATION;
    leds.setColorRGB( 0, 125, 125, 0);
    afficherMessage(1);
  }else { 
    modeActuel = STANDARD;
    leds.setColorRGB(0, 0, 255, 0);
    afficherMessage(0);
  }
}

void loop()
{
  switch (modeActuel) {

    case STANDARD:
    enregistrement_SD();
    delay(2000);
      if (AppuiLong(debut, flag)){
        afficherMessage(2);
        leds.setColorRGB(0,255, 125, 0);
        modeActuel = MAINTENANCE;
        modePrecedent = STANDARD;
      }if (AppuiLong(debut2, flag2)) {
        afficherMessage(3);
        leds.setColorRGB(0,0, 0, 255);
        modeActuel = ECONOMIQUE;
      }break;

    case MAINTENANCE:
    ClockDS1307();
    bmesensor();
    capteurLuminosite();
    delay(EEPROM.read(1));
      if (AppuiLong(debut, flag)) {
        if(modePrecedent == STANDARD){
          modeActuel = STANDARD;
          leds.setColorRGB(0, 0, 255, 0);
          afficherMessage(0);
        }else if(modePrecedent == ECONOMIQUE){
          modeActuel = ECONOMIQUE;
          leds.setColorRGB(0,0, 0, 255);
          afficherMessage(3);
        }
      }break;
      
    case ECONOMIQUE:
    enregistrement_SD();
    delay(EEPROM.read(1)*2);
      if (AppuiLong(debut, flag)) {
        modeActuel = STANDARD;
        leds.setColorRGB(0, 0, 255, 0);
        afficherMessage(0);
      } else if (AppuiLong(debut2, flag2)) {
        modeActuel = MAINTENANCE;
        modePrecedent = ECONOMIQUE;
        leds.setColorRGB(0,255, 125, 0);
        afficherMessage(2);
      }break;

    case CONFIGURATION:
    //command();
    temp = millis();
    if (millis() - temp >= 1800000) { // 30 minutes d'inactivité
      modeActuel = STANDARD;
      leds.setColorRGB(0, 0, 255, 0);
      afficherMessage(4); // Message de retour automatique au mode Standard
    } break;
  }
}

void error() {
  Serial.println(F("Erreur"));
}

//##################### Command of setup mode ######################
void command() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n'); // read the line until the last character
    
    if (input.startsWith("LOG_INTERVAL=")) {
      long log = input.substring(13).toInt();
      EEPROM.put(1, log);
      Serial.print(F("New log_interval : "));
      Serial.println(log);

    } else if (input.startsWith("FILE_MAX_SIZE=")) {
      int newFileSize = input.substring(14).toInt();
      EEPROM.put(2, newFileSize);
      Serial.print("New File_Max_Size : ");
      Serial.println(newFileSize);

    } else if (input == "RESET") {
      resetToDefaults();
      afficherMessage(11);

    } else if (input == "VERSION") {
      afficherMessage(13);

    } else if (input.startsWith("TIMEOUT=")) {
      int newTimeout = input.substring(8).toInt();
      EEPROM.put(3, newTimeout);
      Serial.print("New TimeOut : ");
      Serial.println(newTimeout);

    } else if (input.startsWith("LUMIN=")) {
      int newLum = input.substring(6).toInt();
      if (newLum == 1) {
        EEPROM.put(4, newLum);
        Serial.println(F("Light Sensor activated"));
      } else if (newLum == 0) {
        EEPROM.put(4, newLum);
        Serial.println(F("Light Sensor disabled"));
      } else {
        error();
      }
      
    } else if (input.startsWith("LUMIN_HIGH=")) {
      int newLumHigh = input.substring(11).toInt();
      if (newLumHigh >= 0 && newLumHigh <= 1023) {
      EEPROM.put(5, newLumHigh);
      Serial.print(F("New Lum High : "));
      Serial.println(newLumHigh);
    } else {
      error();
    }

    } else if (input.startsWith("LUMIN_LOW=")) {
      int newLumLow = input.substring(10).toInt();
      if (newLumLow >= 0 && newLumLow <= 1023) {
      EEPROM.put(6, newLumLow);
      Serial.print(F("New Lum Low : "));
      Serial.println(newLumLow);
    } else {
      error();
    }
    
    } else if (input.startsWith("TEMP_AIR=")) {
      int newTempAir = input.substring(9).toInt();
      if (newTempAir == 1) {
        EEPROM.put(7, newTempAir);
        Serial.println(F("Temp Sensor activated"));
      } else if (newTempAir == 0) {
        EEPROM.put(7, newTempAir);
        Serial.println(F("Temp Sensor disabled"));
      } else {
        error();
      }

    } else if (input.startsWith("MIN_TEMP_AIR=")) {
      int newMinTempAir = input.substring(13).toInt();
      if (newMinTempAir >= -40 && newMinTempAir <= 85) {
      EEPROM.put(8, newMinTempAir);
      Serial.print(F("New Temp Min : "));
      Serial.println(newMinTempAir);
    } else {
      error();
    }

    } else if (input.startsWith("MAX_TEMP_AIR=")) {
      int newMaxTempAir = input.substring(13).toInt();
      if (newMaxTempAir >= -40 && newMaxTempAir <= 85) {
      EEPROM.put(9, newMaxTempAir);
      Serial.print(F("New Temp Max : "));
      Serial.println(newMaxTempAir);
    } else {
      error();
    }

    } else if (input.startsWith("HYGR=")) {
      int newHygr = input.substring(5).toInt();
      if (newHygr == 1) {
        EEPROM.put(10, newHygr);
        Serial.println(F("Hygrometry sensor activated"));
      } else if (newHygr == 0) {
        EEPROM.put(10, newHygr);
        Serial.println(F("Hygrometry sensor disabled"));
      } else {
        error();
      }

    } else if (input.startsWith("HYGR_MINT=")) {
      int newHygrMint = input.substring(10).toInt();
      if (newHygrMint >= -40 && newHygrMint <= 85) {
      EEPROM.put(11, newHygrMint);
      Serial.print(F("New Hygr Mint : "));
      Serial.println(newHygrMint);
    } else {
      error();
    }

    } else if (input.startsWith("HYGR_MAXT=")) {
      int newHygrMaxt = input.substring(10).toInt();
      if (newHygrMaxt >= -40 && newHygrMaxt <= 85) {
      EEPROM.put(12, newHygrMaxt);
      Serial.print(F("New Hygr Maxt : "));
      Serial.println(newHygrMaxt);
    } else {
      error();
    }

     } else if (input.startsWith("PRESSURE=")) {
      int newPression = input.substring(9).toInt();
      if (newPression == 1) {
        EEPROM.put(12, newPression);
        Serial.println(F("Pressure Sensor activated"));
      } else if (newPression == 0) {
        EEPROM.put(12, newPression);
        Serial.println(F("Pressure Sensor disabled"));
      } else {
        error();
      }

    } else if (input.startsWith("PRESSURE_MIN=")) {
      int newPressionMin = input.substring(9).toInt();
      if (newPressionMin >= 300 && newPressionMin <= 1100) {
      EEPROM.put(12, newPressionMin);
      Serial.print(F("New Pression : "));
      Serial.println(newPressionMin);
    } else {
      error();
    }

    } else if (input.startsWith("PRESSURE_MAX=")) {
      int newPressionMax = input.substring(9).toInt();
      if (newPressionMax >= 300 && newPressionMax <= 1100) {
      EEPROM.put(13, newPressionMax);
      Serial.print(F("New Pression Max : "));
      Serial.println(newPressionMax);
    } else {
      error();
    }
    
    }else if (input.startsWith("DATE=")) {
    int day, month, year;
    if (sscanf(input.c_str(), "%d/%d/%d", &day, &month, &year) == 3) {
      if (month >= 1 && month <= 12 && day >= 1 && day <= 31 && year >= 2000 && year <= 2099) {
        EEPROM.put(18, day);
        EEPROM.put(17, month);
        EEPROM.put(16, year);
        Serial.println("Date mise à jour : " +  String(day) + "/" + String(month) + "/" + String(year));
        } else {
          afficherMessage(9);
        }
    } else {
        afficherMessage(12);
    }
    }
    else if (input.startsWith("DAY=")) {
    const char* validDays[] = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};
    const char* validFrenchDays[] = {"Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi", "Dimanche"};

    for (int i = 0; i < 7; i++) {
      if (input.equalsIgnoreCase(validDays[i]) || input.equalsIgnoreCase(validFrenchDays[i])) {
        EEPROM.put(19, validFrenchDays[i]);
        Serial.println("Jour de la semaine mis à jour : " + String(validFrenchDays[i]));
        return;
      }
    }
    afficherMessage(21);
    }else if (input.startsWith("CLOCK=")) {
    int hour, minute, second;
    if (sscanf(input.c_str(), "%d:%d:%d", &hour, &minute, &second) == 3) {
        if (hour >= 0 && hour < 24 && minute >= 0 && minute < 60 && second >= 0 && second < 60) {
          EEPROM.put(20, hour);
          EEPROM.put(21, minute);
          EEPROM.put(22, second);
          Serial.println("Heure mise à jour : " + String(hour) + ":" + String(minute) + ":" + String(second));
        } else {
            afficherMessage(20);
        }
    } else {
        afficherMessage(19);
    }
    }else {
      afficherMessage(10);
    }
  }
}
void resetToDefaults(){
  EEPROM.put(1, 10000);
  EEPROM.put(3, 2000);
  EEPROM.put(5, 30000);
  EEPROM.write(7, 1);
  EEPROM.put(9, 255);
  EEPROM.put(11, 768);
  EEPROM.write(13, 1);
  EEPROM.write(15, -10);
  EEPROM.write(17, 60);
  EEPROM.write(19, 1);
  EEPROM.write(21, 0);
  EEPROM.write(23, 50);
  EEPROM.write(25, 1);
  EEPROM.put(27, 850);
  EEPROM.put(29, 1080);
}