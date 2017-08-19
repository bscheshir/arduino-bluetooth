int LED = 13;
int bluetoothVCC = 2;
#include <SoftwareSerial.h>

//int bluetoothGND
int bluetoothRX = 4;
int bluetoothTX = 3;
// при подключении нужно TX -> RXD ,RX -> TXD
SoftwareSerial bluetoothSerial(bluetoothTX, bluetoothRX); // RX, TX

void setup() {
  // put your setup code here, to run once:
  pinMode(bluetoothVCC, OUTPUT);
  digitalWrite(bluetoothVCC, HIGH);
  delay(1000);
  bluetoothSerial.begin(9600);

  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(11, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(LED, OUTPUT);

  //мониторинг со стороны консоли среды разработки arduino (инструменты->монитор порта)
  Serial.begin(9600);

}

void loop() {
  // put your main code here, to run repeatedly:
  //Если данные пришли
  if (bluetoothSerial.available() > 0) {

    //Считываем пришедший байт
    //    byte incomingByte = bluetoothSerial.read();
    int incomingByte = bluetoothSerial.read();
    //2 байта: № пина, состояние
    //Получаем номер пина путем целочисленного деления значения принятого байта на 10
    //и нужное нам действие за счет получения остатка от деления на 2:
    //(1 - зажечь, 0 - погасить)
    //    digitalWrite(constrain(incomingByte / 10, 6, 13), incomingByte % 2);
    incomingByte += 10; // x->LED 0; y->LED 1 через bluetooth terminal (шлёт код символа, читаем как int)
    int pin = constrain(incomingByte / 10, 6, 13);
//    int pin = LED;
    digitalWrite(pin, incomingByte % 2);
//    Serial.println(incomingByte/10);
    Serial.println(incomingByte);
//    Serial.println(pin);
  }

}
