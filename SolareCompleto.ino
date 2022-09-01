#include <Servo.h>
#include<Wire.h>
#include <Adafruit_INA219.h>
#include <SdFat.h>
#include <TinyGPSPlus.h>
#include <NeoSWSerial.h>

/*gps*/
static const int RXPin = 4, TXPin = 3;
static const uint32_t GPSBaud = 9600;
TinyGPSPlus gps;
NeoSWSerial ss(RXPin, TXPin);
String GpsData="vuoto";
String GpsTempo="vuoto";
String GpsLocation="vuoto";
/*sd, nodemcu, multimetro*/
Adafruit_INA219 ina219;

void intToBytes(int x,int y);         //converte da int a bytes prima di mandare

byte sending[4];                   //accumula bytes che devono essere mandati
int b=true;

// microSD 
#define CHIPSELECT 10
#define ENABLE_DEDICATED_SPI 1
#define SD_CONFIG SdSpiConfig(SD_CS_PIN, DEDICATED_SPI, SPI_CLOCK)
#define SPI_DRIVER_SELECT 0
uint8_t cycles = 0;
SdFat32 sd;
File32 measurFile;
//ina
float shuntvoltage = 0.0;
float busvoltage = 0.0;
float current_mA = 0.0;
float loadvoltage = 0.0; 


/*servo fotodiodi*/

Servo servo;   

int eLDRPin = A0; 
int wLDRPin = A1;
int eastLDR = 0; 
int westLDR = 0;
int difference = 0; 
int servoSet = 70; 

/*-------------------------------------------------------------------*/


void setup() {
  
 servo.attach(5);   

Wire.begin(8);                /* apertura i2c bus con indirizzo 8 */
 ina219.begin(); 
 Wire.onReceive(receiveEvent);
 Wire.onRequest(requestEvent); /* evento richiesta  */

 //SD
  sd.begin(CHIPSELECT);
  measurFile.open("SOLAR.csv", O_WRITE | O_CREAT | O_TRUNC);
  measurFile.print("Volt,Corrente,Ora,Data,Coordinate\n");
  measurFile.sync();
 
  Serial.begin(9600); 
  ss.begin(GPSBaud);
}


/*-------------------------------------------------------------------*/

void loop() {

multimetro();  

servomotor();
delay(110000);
}
/*-------------------------------------------------------------------*/
void multimetro() {
  
 
if(b){
  while(ss.available()){
    if(gps.encode(ss.read())){
    gpsdisplayInfo();}
    }
  
  }
else{ 
shuntvoltage = ina219.getShuntVoltage_mV();
busvoltage = ina219.getBusVoltage_V();
current_mA = ina219.getCurrent_mA();
loadvoltage = busvoltage+(shuntvoltage/1000);
// Serial.print("Voltage "); Serial.println(loadvoltage);
// Serial.print("Current ");Serial.println(current_mA);

 writeFile();
intToBytes(loadvoltage*100,current_mA*100);



}delay(100);}
/*-------------------------------------------------------------------*/
void receiveEvent(int howMany) {
 while (0 <Wire.available()) {
    int c = Wire.read();      /* riceve bit a bit */
    b=c;
    }
    
}

// funzione eseguita quando i dati sono richiesti dal master(nodemcu)
void requestEvent() {
  
  if(!b){
 Wire.write(sending[0]);               //Invio dati su richiesta
 Wire.write(sending[1]);
 Wire.write(sending[2]);               //Invio dati su richiesta
 Wire.write(sending[3]);

  }else{
/*String dataora=GpsData+GpsTempo;
Wire.write(dataora.c_str());*/
    
    }

    
}
/*-------------------------------------------------------------------*/
void intToBytes(int x,int y)
{
  sending[0]= (x >>8);                  //Operazione bit a bit
  sending[1]= x & 0xFF;

  sending[2]= (y >>8);                  //Operazione bit a bit
  sending[3]= y & 0xFF;


}
/*-------------------------------------------------------------------*/
void writeFile() {
    char buf[80], voltbuf[16]={0}, curbuf[16]={0};
    char gt[10]={0};
    char gd[10]={0};
    char gl[20]={0};
    // buffer con the volt e corrente in stringhe
    dtostrf(loadvoltage, 10, 3, voltbuf);
    dtostrf(current_mA, 10, 3, curbuf);
    
    GpsTempo.toCharArray(gt, 10);
    GpsData.toCharArray(gd, 10);
    GpsLocation.toCharArray(gl, 20);
    //linea csv : voltage,current\n
    sprintf(buf, "%s,%s,%s,%s,%s\n", voltbuf, curbuf,gd,gt,gl);

    //scrittura 
    measurFile.write(buf);

   //dopo 9 cicli (1 sec.), da buffer a file in SD
    if(cycles >=9)
      measurFile.sync();

    //incrementa contatore cicli e reset a 0 dop 10 cicli
    cycles++;
    cycles %= 10;
}

/*-------------------------------------------------------------------*/
void gpsdisplayInfo()
{
  if (gps.location.isValid())
  {
 GpsLocation=(String(gps.location.lat(), 6)+","+String(gps.location.lng(), 6));
  }
  else
  {
 GpsLocation="INVALID";
  }

  if (gps.date.isValid())
  {
     GpsData=(String(gps.date.day())+"/"+String(gps.date.month())+"/"+String(gps.date.year()));
  }
   else
  {
     GpsData="INVALID";
  }

  
  if (gps.time.isValid())
  {
    if(gps.time.hour()+2>23){
 GpsTempo=(String(gps.time.hour()+2-24)+":"+String(gps.time.minute())+":"+String(gps.time.second()));
}
    else{ 
 GpsTempo=(String(gps.time.hour()+2)+":"+String(gps.time.minute())+":"+String(gps.time.second()));
  
  }
  }
  else
  {
 GpsTempo="INVALID";
  }

  delay(1000);
}
/*-------------------------------------------------------------------*/
void servomotor() {
  
  eastLDR = analogRead(eLDRPin); 
  westLDR = analogRead(wLDRPin);

  difference = eastLDR - westLDR ; 
  
  if (difference > 100) {         
  
      if (servoSet <= 120) {
      servoSet =servoSet+10 ;
      
    }
    
    
   
  } 

  else if (difference < -100) {
   
     if (servoSet >= 80) {
      servoSet =servoSet-10;
       
    }
   
  } 
  
  else if (difference >= -100 && difference <= 100) {
    if (servoSet > 90) {
      servoSet =servoSet-10;
    }
    if (servoSet <= 90) {
      servoSet =servoSet+10;
    }

  }

servo.write(servoSet);

  delay(1000);
}
