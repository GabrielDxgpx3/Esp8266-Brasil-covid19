#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

const char* ssid = "SSID";
const char* password = "PASS";
const char* url = "http://covid-tracker.000webhostapp.com/";
const String BRASIL = "?tipo=brasil";
const String MUNDO = "?tipo=mundo";
String CURRENT_LOCAL = "";
String categorias[4] = {"CONFIRMADOS", "RECUPERADOS", "MORTES", "ATIVOS"};
String values_brasil[4] = {"0","0","0","0"};
String values_mundo[4] = {"0","0","0","0"};

int confirmedFreq = 1200;
int confirmedDelay = 130;
int deathsFreq = 4000;
int deathsDelay = 1000;
byte toneConfirmedRepeat = 4;
byte toneDeathRepeat = 1;

int buzzer = 12;

LiquidCrystal_I2C lcd(0x27, 16, 2);

WiFiClient client;
HTTPClient http;

boolean is_sleeping = false;
unsigned long previousMillis = 0;
unsigned long sleepInterval = 10 * 60000;

void request(String tipo);
void parseDados(String dados, int size);
void exibirDados(String dados[4], String values[4], String local);
void toneConfirmed();
void toneDeaths();
void sleep();

void initWifi(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("CONECTANDO WiFi");
  lcd.setCursor(0,1);
  lcd.print(ssid);
  
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(5000);
    ESP.restart();
  }

  lcd.blink_off();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("CONECTADO!");
  lcd.setCursor(0,1);
  lcd.print(WiFi.localIP());
  delay(400);

}

void setup() {
  pinMode(buzzer, OUTPUT);
  lcd.init();
  lcd.backlight();
  initWifi();
  request(BRASIL);
  request(MUNDO);
}

void loop() {
  sleep();
}

void request(String tipo){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("SOLICITANDO");
  lcd.setCursor(0,1);
  lcd.print("DADOS...");

  CURRENT_LOCAL = (tipo == BRASIL) ? "BRASIL" : "MUNDO";

  int is_ok = http.begin(client, url + tipo);

  if(is_ok){

    int code = http.GET();

    if(code == HTTP_CODE_OK){
      parseDados(http.getString(), http.getSize());
    }else{
      lcd.blink_off();
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Falha na");
      lcd.setCursor(0,1);
      lcd.print("Requisicao: " + String(code));

    }


  }else{
    lcd.print("Falha ao iniciar");
  }

}

void parseDados(String dados, int size){
  
  char data[size];
  strcpy(data, dados.c_str());
  char limiter = ',';
  int pos = 0;
  String res[4];
  char BREAK_CHAR = '@';
  
  for(int i = 0; i < size; i++){
    
    if(data[i] == limiter){
      pos++;
      continue;
    }else if(data[i] == BREAK_CHAR){
      break;
    }

    res[pos] += data[i];

  }

  

  if(CURRENT_LOCAL == "BRASIL"){
    exibirDados(res, values_brasil, CURRENT_LOCAL);

    values_brasil[0] = res[0];
    values_brasil[1] = res[1];
    values_brasil[2] = res[2];
    values_brasil[3] = res[3];

    values_brasil[3].replace("\\r", "");
    values_brasil[3].replace("\\n", "");

  }else{
    exibirDados(res, values_mundo, CURRENT_LOCAL);

    values_mundo[0] = res[0];
    values_mundo[1] = res[1];
    values_mundo[2] = res[2];
    values_mundo[3] = res[3];

    values_mundo[3].replace("\\r", "");
    values_mundo[3].replace("\\n", "");
  }
  
  
  

}

void verificarDelta(String current_value, String new_value, boolean confirmados){

  if(current_value != new_value){
    
    if(confirmados){
      toneConfirmed();
    }else{
      toneDeaths();
    }
  
  }

}

void toneConfirmed(){

  for(byte z = 0; z < toneConfirmedRepeat; z++){
    tone(buzzer, confirmedFreq);
    delay(confirmedDelay);
    noTone(buzzer);
    delay(confirmedDelay);
  }

}

void toneDeaths(){

  for(byte z = 0; z < toneDeathRepeat; z++){
    tone(buzzer, deathsFreq);
    delay(deathsDelay);
    noTone(buzzer);
    delay(deathsDelay);
  }
  
}

void exibirDados(String res[4], String values[4], String local){
  lcd.clear();
  lcd.setCursor(4,0);
  lcd.print("COVID-19");
  lcd.setCursor(5,1);
  lcd.print(local);
  delay(4000);

  for(int z = 0; z < sizeof(res); z++){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(categorias[z]);
    lcd.setCursor(0,1);
    lcd.print(res[z]);
    
    if(categorias[z] == "CONFIRMADOS"){
      verificarDelta(res[z], values[z], true);
    }else if(categorias[z] == "MORTES"){
      verificarDelta(res[z], values[z], false);
    }
    
    delay(4000);
  }


}

void sleep(){

  unsigned long currentMillis = millis();

  if(is_sleeping){

    if(currentMillis - previousMillis > sleepInterval){
      previousMillis = currentMillis;

      lcd.backlight();
      initWifi();
      request(BRASIL);
      request(MUNDO);

      is_sleeping = false;
      return;

    }

  }

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Hibernando..");
  lcd.blink_off();
  delay(1000);
  lcd.noBacklight();
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  delay(1);
  is_sleeping = true;

}