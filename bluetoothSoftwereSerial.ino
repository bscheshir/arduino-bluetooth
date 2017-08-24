#include <TM1637.h>
#include <SoftwareSerial.h>

int LED = 13;
int bluetoothVCC = 2;
int tmCLK = 5;
int tmDIO = 6;
int tmVCC = 7;
int tmGND = 8;

int8_t NumTab[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}; //0~9,A,b,C,d,E,F
int8_t ListDisp[4];
unsigned char count = 0;


//int bluetoothGND
int bluetoothRX = 4;
int bluetoothTX = 3;
// при подключении нужно TX -> RXD ,RX -> TXD
SoftwareSerial bluetoothSerial(bluetoothTX, bluetoothRX); // RX, TX

TM1637 tm1637(tmCLK, tmDIO); //создаем экземпляр объекта типа «TM1637», с которым будем далее работать и задаем пины.

void setup() {
  // put your setup code here, to run once:
  pinMode(bluetoothVCC, OUTPUT);
  digitalWrite(bluetoothVCC, HIGH);

  pinMode(tmVCC, OUTPUT);
  digitalWrite(tmVCC, HIGH);
  pinMode(tmGND, OUTPUT);
  digitalWrite(tmGND, LOW);

  //  pinMode(6, OUTPUT);
  //  pinMode(7, OUTPUT);
  //  pinMode(8, OUTPUT);
  //  pinMode(9, OUTPUT);
  //  pinMode(10, OUTPUT);
  //  pinMode(11, OUTPUT);
  //  pinMode(12, OUTPUT);
  pinMode(LED, OUTPUT);

  //  delay(1000);
  //порт блютуза
  bluetoothSerial.begin(9600);
  //мониторинг со стороны консоли среды разработки arduino (инструменты->монитор порта)
  Serial.begin(9600);

  tm1637.init();// инициализация библиотеки «TM1637.h»
  tm1637.set(BRIGHT_TYPICAL);//установка яркости указанная константа равна 2, значение по умолчанию

}

void loop() {
  // put your main code here, to run repeatedly:
  //Если данные пришли
  if (bluetoothSerial.available() > 0) {

    String readString;
    while (bluetoothSerial.available()) {
      delay(3);  //пауза для того, чтобы буфер наполнился
      if (bluetoothSerial.available() > 0) {
        char c = bluetoothSerial.read();  //получить один байт из порта
        if (c == '\n') {
          break;
        }
        readString += c; //дополняем прочитаную строку
      }
    }
    //readString.substring(0, 2);
    int incomingBytePin = readString[0];
    int incomingByteState = readString[1];
    //2 байта: № пина, состояние
    //Получаем номер пина
    //и нужное нам действие за счет получения остатка от деления на 2:
    //(1 - зажечь, 0 - погасить)
    int pin = constrain(incomingBytePin, 6, 13);
    int state = incomingByteState % 2;
    digitalWrite(pin, state);
    //Отправляем ответ устройству
    bluetoothSerial.println(String(pin) + ' ' + (state ? "ON" : "OFF"));
    //Отправляем состояние по сериалу на монитор порта
    Serial.println(String(pin) + ' ' + (state ? " ON" : " OFF"));

    //отобразить на табло номер пина и состояние
    //tm1637 http://робопро.рф/?p=41
    tm1637.display(incomingBytePin);
    delay(500);
    tm1637.display(incomingByteState);

  }
  else {
//    bluetoothSerial.println("PING");

    delay(1000);
    unsigned char i = count;
    count ++;
    if (count == sizeof(NumTab)) count = 0;
    for (unsigned char BitSelect = 0; BitSelect < 4; BitSelect ++)
    {
      ListDisp[BitSelect] = NumTab[i];
      i ++;
      if (i == sizeof(NumTab)) i = 0;
    }
    tm1637.display(0, ListDisp[0]);
    tm1637.display(1, ListDisp[1]);
    tm1637.display(2, ListDisp[2]);
    tm1637.display(3, ListDisp[3]);
  }

}
