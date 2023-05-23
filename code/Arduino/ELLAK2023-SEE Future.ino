/***********************************************************  
 *  5ος Πανελλήνιος Διαγωνισμός ΕΛΛΑΚ 2023
 *  Έξυπνο σύστημα εξοικονόμησης ενέργειας σχολικής αίθουσας
 *  Υλοποίηση με Arduino Mega, DHT11, PMS5003, CCS811, HC-05
 ***********************************************************/

// Βιβλιοθήκη για Αισθητήρα Μικροσωματιδίων PMS5003
#include "PMS.h"
// Βιβλιοθήκη για αισθητήρα θερμοκρασίας-υγρασίας DHT-11
#include "DHT.h"
#define DHTPIN 2        // θύρα σύνδεσης του DHT11
#define DHTTYPE DHT11   // τύπος αισθητήρα (DHT 11)
DHT dht(DHTPIN, DHTTYPE);
// Βιβλιοθήκη για την οθόνη υγρών κρυστάλλων
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
// Βιβλιοθήκη για τον αισθητήρα ποιότητας αέρα (CO2 και tVOC)
#include "SparkFunCCS811.h" // Σύνδεσμος: http://librarymanager/All#SparkFun_CCS811

// Διεύθυνση I2C για τον αισθητήρα CCS811
#define CCS811_ADDR 0x5B    // Default I2C Address
//#define CCS811_ADDR 0x5A  // Alternate I2C Address

// Οι θύρες για τα LEDs
const int PurifierRed = 31;
const int PurifierBlue= 33;
const int ACBlue = 35;
const int ACRed = 37;
const int CaloriferRed = 39;
const int CaloriferBlue = 41;
const int whiteLights = 50;
// Θύρα σύνδεσης αισθητήρα παραθύρου (reed switch)
const int WINDOW_SENSOR_PIN = 10;

// Αρχικοποίηση ενός αντικειμένου για τον αισθητήρα PMS 
PMS pms(Serial1);
PMS::DATA data;
// Αρχικοποίηση οθόνης LCD
LiquidCrystal_I2C lcd(0x27, 20, 4);
// Αρχικοποίηση αισθητήρα CCS811
CCS811 mySensor(CCS811_ADDR);

// Ορισμός μεταβλητών
int hum;    // υγρασία
int temp;   // θερμοκρασία
int state;  // κατάσταση παραθύρου (ανοικτό/κλειστό)
unsigned long currentTime;      // για μέτρηση χρόνου
unsigned long previousTime = 0;
int pmsData25 = 0;    // για μικροσωματίδια 2.5μm
int pmsData100 = 0;   // για μικροσωματίδια 10μm
int co2 = 0;      // διοξείδιο του άνθρακα
int tvoc = 0;     // πτητικές οργανικές ενώσεις

void setup()
{
  // Ορισμός θυρών
  pinMode(PurifierRed, OUTPUT);
  pinMode(PurifierBlue, OUTPUT);
  pinMode(ACBlue, OUTPUT);
  pinMode(ACRed, OUTPUT);
  pinMode(CaloriferRed, OUTPUT);
  pinMode(CaloriferBlue, OUTPUT);
  pinMode(whiteLights, OUTPUT);
  // στη θύρα του διακόπτη (reed switch) ενεργοποιούμε την εσωτερική pull-up  αντίσταση
  pinMode(WINDOW_SENSOR_PIN, INPUT_PULLUP);

  // Αρχικοποίηση LED
  digitalWrite(PurifierRed, HIGH);
  digitalWrite(PurifierBlue, LOW);
  digitalWrite(ACBlue, LOW);
  digitalWrite(ACRed, HIGH);
  digitalWrite(CaloriferRed, HIGH);
  digitalWrite(CaloriferBlue, LOW);
  digitalWrite(whiteLights, HIGH);
  delay(1000);
  digitalWrite(whiteLights, LOW);

  // αρχικοποίηση DHT11
  dht.begin();
  
  // Αρχικοποίηση σειριακών συνδέσεων για το ARDUINO MEGA
  Serial.begin(9600);   // σύνδεση με την πλακέτα bluetooth (HC-05)
  Serial1.begin(9600);  // σύνδεση με την πλακέτα του PMS5003
  pms.passiveMode();    // Ορισμός passive mode
  pms.wakeUp();

  // αρχικοποίηση LCD
  lcd.init();
  // εμφάνιση μηνύματος στην οθόνη LCD
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("1st EPAL of Trikala ");
  
  // αρχικοποίηση διαύλου I2C (για CCS811)
  Wire.begin();

  // αρχικοποίηση CCS811
  if( mySensor.begin() == false )
  {
    lcd.print("CCS811 Error...     ");
    while (1); // τέλος σε περίπτωση σφάλματος...
  }
}

void loop()
{
  // καταγραφή κατάστασης παραθύρου (ανοικτό/κλειστό)
  state=digitalRead(WINDOW_SENSOR_PIN);
  
  // καταγραφή θερμοκρασίας-υγρασίας από αισθητήρα DHT11
  float hum = dht.readHumidity();
  float temp = dht.readTemperature();

  // καταγραφή φωτεινότητητας από τη φωτοαντίσταση
  int light = analogRead(A0);
  
  // έλεγχος φωτισμού
  if( light < 550 )
  {
    digitalWrite(whiteLights, HIGH);  // άναψε φώτα
  }
  else
  {
    digitalWrite(whiteLights, LOW);   // σβήσε φώτα
  }

  // μέτρηση χρόνου
  currentTime = millis();
  // το παρακάτω block εκτελείται κάθε 5 δευτερόλεπτα
  if( currentTime - previousTime >= 5000 )
  {
    // έλεγχος αν υπάρχουν διαθέσιμα δεδομένα από τον CCS811
    if( mySensor.dataAvailable() )
    {
      // ανάγνωση των δεδομένων και καταγραφή CO2 και tVOC
      mySensor.readAlgorithmResults();
      co2 = mySensor.getCO2();
      tvoc = mySensor.getTVOC();
    }

    // έλεγχος για δεδομένα στον αισθητήρα PMS5003
    pms.requestRead();  
    if( pms.readUntil(data) )
    {
      // καταγραφή μικροσωματιδίων 2.5mμ και 10μm
      pmsData25 = data.PM_AE_UG_2_5;
      pmsData100 = data.PM_AE_UG_10_0;
    }

    // αποστολή των δεδομένων στην εφαρμογή android
    // (μέσω σειριακής σύνδεσης και bluetooth)
    // τα δεδομένα χωρίζονται με τον χαρακτήρα # 
    // όπως απαιτεί η εφαρμογή android που υλοποίησαμε
    Serial.print(temp);
    Serial.print("#");
    Serial.print(hum);
    Serial.print("#");
    Serial.print(pmsData25);
    Serial.print("#");
    Serial.print(pmsData100);
    Serial.print("#");
    Serial.print(co2);
    Serial.print("#");
    Serial.print(tvoc);
    Serial.print("#");
    Serial.print(state);
    Serial.print("#");
    Serial.println(light);
    
    // εμφάνιση δεδομένων στην οθόνη LCD
    lcd.setCursor(0,1);
    lcd.print("PM2.5=");
    lcd.setCursor(6,1);
    lcd.print("    ");
    lcd.setCursor(6,1);
    lcd.print(pmsData25);
    
    lcd.setCursor(10,1);
    lcd.print("PM10=");
    lcd.setCursor(15,1);
    lcd.print("     ");
    lcd.setCursor(15,1);
    lcd.print(pmsData100);

    lcd.setCursor(0, 2);
    lcd.print("CO2=");
    lcd.print("      ");
    lcd.setCursor(4, 2);
    lcd.print(co2);

    lcd.setCursor(10, 2);
    lcd.print("tVOC=");
    lcd.print("    ");    
    lcd.setCursor(15, 2);
    lcd.print(tvoc);

    lcd.setCursor(0, 3);
    lcd.print("T=");
    lcd.print("      ");
    lcd.setCursor(2, 3);
    lcd.print(temp);
    lcd.print("oC ");
    lcd.setCursor(10, 3);
    lcd.print("H=");
    lcd.print("      ");    
    lcd.setCursor(12, 3);
    lcd.print(hum);
    lcd.print("%  ");

    // έλεγχος θερμοκρασίας για ενεργοποίηση καλοριφέρ
    if( temp<25 )
    {
      digitalWrite(CaloriferRed, LOW);
      digitalWrite(CaloriferBlue, HIGH);
    }
    else
    {
      digitalWrite(CaloriferRed, HIGH);
      digitalWrite(CaloriferBlue, LOW);    
    }
    
    // έλεγχος θερμοκρασίας για ενεργοποίηση air condition
    if( temp>27 )
    {
      digitalWrite(ACRed, LOW);
      digitalWrite(ACBlue, HIGH);
    }
    else
    {
      digitalWrite(ACRed, HIGH);
      digitalWrite(ACBlue, LOW);    
    }
    
    // έλεγχος ποιότητας αέρα για ενεργοποίηση air purifier
    if( pmsData25>12 || pmsData100>54 || co2>5000 || tvoc>500 )
    {
      digitalWrite(PurifierRed, LOW);
      digitalWrite(PurifierBlue, HIGH);
    }
    else
    {
      digitalWrite(PurifierRed, HIGH);
      digitalWrite(PurifierBlue, LOW);    
    }

    // ενημέρωση χρόνου
    previousTime = currentTime;
  }
}
