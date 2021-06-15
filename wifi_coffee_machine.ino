#include "FirebaseESP8266.h"
#include <ESP8266WiFi.h>

const char* ssid = "RG_INTELBRAS";                 //SSID da rede WIFI
const char* password =  "12023382";    //senha da rede wifi

const double VCC = 3.3;             // NodeMCU on board 3.3v vcc
const double R2 = 10000;            // 10k ohm series resistor
const double adc_resolution = 1023; // 10-bit adc

const double A = 0.001129148;   // thermistor equation parameters
const double B = 0.000234125;
const double C = 0.0000000876741; 

#define FIREBASE_HOST "REALTIME_DATABASE_URL"   //firebaseio
#define FIREBASE_AUTH "SECRET_KEY_REALTIME_DATABASE"  //firebaseio

#define INTERVALO_ENVIO       2000
#define INTERVALO_DISPLAY     1000

#define WIFI_SSID "WIFI_SSID"
#define WIFI_PASSWORD "PASSWORD"

bool machine_active, withCup, preparing;

int ultimoEnvio = 0, ultimoDisplay = 0, quantity;

FirebaseData firebaseData;

WiFiClient espClient;

void prepare_coffee(float quantity) {

  aquece();
  
  int current_quantity = 0;
  Firebase.setInt(firebaseData, "current_quantity", current_quantity);

  while(current_quantity < quantity) {
    current_quantity += random(10, 15);
        
    if (current_quantity > quantity) {
      current_quantity = quantity;
    }
    
    float percentage = (current_quantity / quantity)*100;
    
    Firebase.setInt(firebaseData, "current_quantity", current_quantity);
    Firebase.setInt(firebaseData, "percentage", percentage);
  }
  Firebase.setBool(firebaseData, "preparing", false);
  digitalWrite(D8,LOW);
  Firebase.setString(firebaseData, "status", "Café pronto! Pode retirar.");
  Firebase.setBool(firebaseData, "withCup", false);
}

void aquece() {
  double Vout, Rth, temperature, temperature1, adc_value; 
  adc_value = analogRead(A0);
  Vout = (adc_value * VCC) / adc_resolution;
  Rth = (VCC * R2 / Vout) - R2;

/*  Steinhart-Hart Thermistor Equation:
 *  Temperature in Kelvin = 1 / (A + B[ln(R)] + C[ln(R)]^3)
 *  where A = 0.001129148, B = 0.000234125 and C = 8.76741*10^-8  */
  temperature = (1 / (A + (B * log(Rth)) + (C * pow((log(Rth)),3))));   // Temperature in kelvin
  temperature = temperature - 273.15;  // Temperature in degree celsius
  
  if (temperature < 10) {
    temperature = 10;
  }
  
  Firebase.setFloat(firebaseData, "temperature", temperature);
  while(temperature < 90) {
    temperature += random(10, 15);

    if (temperature > 90) {
      temperature = 90;
    }
    Firebase.setInt(firebaseData, "temperature", temperature);
  }
}

void setup() {
  pinMode(D1,OUTPUT);
  pinMode(D7, INPUT);
  pinMode(D8, OUTPUT);
  
  Serial.begin(115200);
//  Serial.println("Inicio");
  WiFi.enableInsecureWEP(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

 }

void loop() {
  if (ultimoEnvio == 0) {
    Firebase.setBool(firebaseData, "active", false);
    Firebase.setBool(firebaseData, "withCup", false);
    Firebase.setBool(firebaseData, "preparing", false);
    Firebase.setFloat(firebaseData, "quantity", 0);
    Firebase.setFloat(firebaseData, "temperature", 0);
    Firebase.setInt(firebaseData, "percentage", 0);
    Firebase.setString(firebaseData, "status", "Cafeteira pronta para uso!");
    ultimoEnvio = millis();
  }
  
  Firebase.getBool(firebaseData, "active");
  machine_active = firebaseData.boolData();

  if (machine_active == true) {
    digitalWrite(D1,HIGH);

    if (digitalRead(D7) == 1) {
      Firebase.setBool(firebaseData, "withCup", true);
      Firebase.setString(firebaseData, "status", "Copo adicionado! Pode iniciar o preparo ...");
    }
    
    Firebase.getBool(firebaseData, "preparing");
    preparing = firebaseData.boolData();
    
    if (preparing == true){
      digitalWrite(D8,HIGH);
      Firebase.setString(firebaseData, "status", "Preparação iniciada!");
      Firebase.getFloat(firebaseData, "quantity");
      quantity = firebaseData.floatData();

      prepare_coffee(quantity);
    }
    
  } else {
    digitalWrite(D1,LOW);
    Firebase.setBool(firebaseData, "withCup", false);
  } 
}


