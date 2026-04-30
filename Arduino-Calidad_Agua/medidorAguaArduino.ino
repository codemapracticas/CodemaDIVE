#include "BluetoothSerial.h"
#include <OneWire.h>
#include "DFRobot_ESP_PH.h"
#include <EEPROM.h>
#include <LiquidCrystal.h>
#include <EasyButton.h>

#define BTN_PIN 23
bool btn_pressed = false;

//DEBUG
bool debug = false;
bool debugPH = true;

//LCD
const int rs = 19/*2*/, en = 21/*4*/, d4 = 16, d5 = 17, d6 = 22/*5*/, d7 = 18;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//BT
BluetoothSerial SerialBT;

//TDS
const float VREF = 3.3;      // analog reference voltage(Volt) of the ADC
const int SCOUNT = 30;           // sum of sample point

int analogBuffer[SCOUNT];    // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0,copyIndex = 0;
float averageVoltage = 0;

//PH
float voltagePH;
DFRobot_ESP_PH ph;

//PINES
int pinTDS = 35;
int pinTurbidez = 34;
int pinTemp = 5;
int pinPH = 32;

//Variables medidas
float medidaPH;
float medidaTemp = 25;
float medidaTDS;
float medidaTurb;

//Inicializaciones varias
OneWire ds(pinTemp);  //OneWire temp


unsigned long button_time = 0;  
unsigned long last_button_time = 0; 
void btnEvent() {
  button_time = millis();
  if (button_time - last_button_time > 250) {
    Serial.println("Button pressed");
    btn_pressed = true;
    last_button_time = button_time;
  }
}

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32_BT_Classic"); // Nombre del dispositivo Bluetooth
  Serial.println("El dispositivo Bluetooth está listo para emparejarse.");

  //PINES
  pinMode(pinTurbidez,INPUT); //Turbidez

  //PH
  ph.begin();
  EEPROM.begin(32);

  //LCD
  lcd.begin(16, 2);
  lcd.print("Calidad de Agua");
  lcd.setCursor(0, 1);

  //TDS
  pinMode(pinTDS,INPUT);


  pinMode(BTN_PIN, INPUT_PULLUP);
  attachInterrupt(BTN_PIN, btnEvent, FALLING);
}

void loop() {
  //COMUNICACION SERIAL<->BT
  if (Serial.available()) {
    realizarMedidas();
    updateLCD();
    Serial.print("Data: ");
    Serial.println(prepareResponse());
    ////SerialBT.write(Serial.read()); // Envía datos desde el monitor serie al Bluetooth
  }
  if (SerialBT.available()) {
    String a = SerialBT.readString();

    if (a == "escaneo") {
      realizarMedidas();
      updateLCD();
      SerialBT.println(prepareResponse());
      //SerialBT.println("5.6,25.5,15.6,78.4"); //PH temperatura conductividad turbidez
    }
    Serial.println(a); // Envía datos desde el Bluetooth al monitor serie
  }

   if(btn_pressed) {
    Serial.println("Pulsado");
    
    realizarMedidas();
    updateLCD();
    Serial.print("Data: ");
    Serial.println(prepareResponse());

    btn_pressed = false;
   }

  //OPERACIONES CONSTANTES
  static unsigned long analogSampleTimepoint = millis();
   if(millis()-analogSampleTimepoint > 40U)     //every 40 milliseconds,read the analog value from the ADC
   {
     analogSampleTimepoint = millis();
     analogBuffer[analogBufferIndex] = analogRead(pinTDS);    //read the analog value and store into the buffer
     analogBufferIndex++;
     if(analogBufferIndex == SCOUNT) 
         analogBufferIndex = 0;
   }  

}

void updateLCD (){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(medidaTemp);
  lcd.setCursor(8, 0);
  lcd.print("TDS: ");
  lcd.print(medidaTDS);
  lcd.setCursor(0, 1);
  lcd.print("Turb: ");
  lcd.print(medidaTurb);
  lcd.setCursor(8, 1);
  lcd.print("PH: ");
  lcd.print(medidaPH);


}

void realizarMedidas() {
  readTempSensor();
  readPHSensor();
  readTDSSensor();
  readTurbiditySensor();
}


String prepareResponse () {
  String out = "";
  out += ""+String(medidaPH);
  out += ","+String(medidaTemp);
  out += ","+String(medidaTDS);
  out += ","+String(medidaTurb);
  return out;
}





void readPHSensor () {
  Serial.println(analogRead(pinPH)*VREF/4096.0);
  voltagePH = (analogRead(pinPH)*VREF/4096.0)*1000;  // read the voltage
  medidaPH = ph.readPH(voltagePH,medidaTemp);  // convert voltage to pH with temperature compensation
  if(debugPH){
    Serial.print("voltage:");
    Serial.print(voltagePH,1);
    Serial.print(" temperature:");
    Serial.print(medidaTemp,1);
    Serial.print(" ^C  pH:");
    Serial.println(medidaPH,2);
  }
  ph.calibration(voltagePH,medidaTemp);           // calibration process by Serail CMD
  /* You can send commands in the serial monitor to execute the calibration.
   * Serial Commands:
   *   enterph -> enter the calibration mode
   *   calph   -> calibrate with the standard buffer solution, two buffer solutions(4.0 and 7.0) will be automaticlly recognized
   *   exitph  -> save the calibrated parameters and exit from calibration mode
   */

}

void readTempSensor () {
  byte data[12];
  byte addr[8];

  if ( !ds.search(addr)) {
      //no more sensors on chain, reset search
      ds.reset_search();
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
      Serial.print("Device is not recognized");
  }

  ds.reset();
  ds.select(addr);
  ds.write(0x44,1); // start conversion, with parasite power on at the end

  byte present = ds.reset();
  ds.select(addr);
  ds.write(0xBE); // Read Scratchpad


  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds.read();
  }

  ds.reset_search();

  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  medidaTemp = tempRead / 16;
}

void readTurbiditySensor () {
  float valor = analogRead(pinTurbidez); 
  medidaTurb = valor * (VREF / 4096.0);
}

void readTDSSensor () {
  for(copyIndex=0;copyIndex<SCOUNT;copyIndex++)
    analogBufferTemp[copyIndex]= analogBuffer[copyIndex];
  averageVoltage = getMedianNum(analogBufferTemp,SCOUNT) * (float)VREF / 4096.0; // read the analog value more stable by the median filtering algorithm, and convert to voltage value
  Serial.println(getMedianNum(analogBufferTemp,SCOUNT));
  Serial.println(averageVoltage);
  float compensationCoefficient=1.0+0.02*(medidaTemp-25.0);    //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0));
  float compensationVolatge=averageVoltage/compensationCoefficient;  //temperature compensation
  medidaTDS=(133.42*compensationVolatge*compensationVolatge*compensationVolatge - 255.86*compensationVolatge*compensationVolatge + 857.39*compensationVolatge)*0.5; //convert voltage value to tds value
}





int getMedianNum(int bArray[], int iFilterLen) 
{
      int bTab[iFilterLen];
      for (byte i = 0; i<iFilterLen; i++)
	  bTab[i] = bArray[i];
      int i, j, bTemp;
      for (j = 0; j < iFilterLen - 1; j++) 
      {
	  for (i = 0; i < iFilterLen - j - 1; i++) 
          {
	    if (bTab[i] > bTab[i + 1]) 
            {
		bTemp = bTab[i];
	        bTab[i] = bTab[i + 1];
		bTab[i + 1] = bTemp;
	     }
	  }
      }
      if ((iFilterLen & 1) > 0)
	bTemp = bTab[(iFilterLen - 1) / 2];
      else
	bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
      return bTemp;
}