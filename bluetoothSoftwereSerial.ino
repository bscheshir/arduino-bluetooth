#include <DFPlayer_Mini_Mp3.h> //http://lesson.iarduino.ru/page/urok-17-podklyuchenie-mini-mp3-pleera-k-arduino/ http://iarduino.ru/file/140.html
#include <Stepper_28BYJ.h> //https://lesson.iarduino.ru/page/upravlenie-shagovym-dvigatelem-s-arduiny/ http://iarduino.ru/file/148.html
#include <OneButton.h>//http://microsin.net/programming/avr/arduino-onebutton-library.html https://github.com/mathertel/OneButton
#include <TM1637.h>//http://робопро.рф/?p=41 https://yadi.sk/d/Ci5aiYzsqo7qk
#include <SoftwareSerial.h>


// количество шагов для мотора
#define STEPS 4078



//Внешнее прерывание: 2 и 3.
//Данные выводы могут быть сконфигурированы на вызов прерывания либо на младшем значении,
//либо на переднем или заднем фронте, или при изменении значения. Подробная информация находится в описании функции attachInterrupt().
int LED = 13;
int tmCLK = 8;
int tmDIO = 7;
//int tmVCC = 7; //Работает без подключённого пина VCC
//int tmGND = 6;


//int bluetoothVCC = 9;
//int bluetoothGND //экономим пины
int bluetoothTX = 11;
int bluetoothRX = 12;

int mp3TX = 9;
int mp3RX = 10;

int stepper0 = 6;
int stepper1 = 5;
int stepper2 = 4;
int stepper3 = 3;

//int buttonPin = 2; // прерывание по 2му пину
int volumeAnalogPin = A0;

// Библиотека для обработки нажатий. Попробовать миксовать с прерыванием.
OneButton button1(2, true); // пин (цифровой/аналоговый), использовать низкий уровень (true)/ использоват высокий уровень (false) как факт нажатия.
//OneButton button1(A1, true);


SoftwareSerial bluetoothSerial(bluetoothTX, bluetoothRX); // (RX, TX) при подключении нужно TX -> RXD ,RX -> TXD
SoftwareSerial mp3Serial(mp3RX, mp3TX); // (RX, TX)

//степпер
Stepper_28BYJ stepper(STEPS, stepper0, stepper1, stepper2, stepper3);

TM1637 tm1637(tmCLK, tmDIO); //создаем экземпляр объекта типа «TM1637», с которым будем далее работать и задаем пины.

class DigitFlow
{
    // Переменные - члены класса
    // Инициализируются при запуске
    unsigned char count;//счётчик перебора

    long OnTickTime; // время отображения одного значения потока в миллисекундах

    // Текущее состояние
    unsigned long previousMillis; // последний момент смены состояния

    // Конструктор создает экземпляр класса
    // и инициализирует переменные-члены класса и состояние
  public:
    DigitFlow(long oneTickDelay)
    {
      count = 0;
      OnTickTime = oneTickDelay;
      previousMillis = 0;
    }

    void Update(unsigned long currentMillis)
    {
      int8_t NumTab[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}; //0~9,A,b,C,d,E,F
      int8_t ListDisp[] = {0x00, 0x00, 0x00, 0x00}; //буфер текущих символов. Очищаем перед использованием, а то мусор с прошлого такта притащить может
      //      String strDisp;
      // выясняем не настал ли момент сменить состояние светодиодов
      if ((currentMillis - previousMillis >= OnTickTime))
      {
        previousMillis = currentMillis; // запоминаем момент времени
        //библиотека работает плохо. любой вариант вызова в данном методе приводит к артефактам.
        //Проверка по монитору порта подтвердила корректность скармливаемого массива
        unsigned char i = count;
        count ++;
        if (count == sizeof(NumTab)) count = 0;
        for (unsigned char BitSelect = 0; BitSelect < 4; BitSelect ++)
        {
          //          Serial.println(NumTab[i]);
          ListDisp[BitSelect] = NumTab[i];
          tm1637.display(BitSelect, ListDisp[BitSelect]);
          i ++;
          if (i == sizeof(NumTab)) i = 0;
        }
        //        tm1637.display(ListDisp);
        //        Serial.println();
      }
    }
};

// объявление "потока цифр"
DigitFlow df(2000);

//класс управления громкостью
class VolumeControl
{
    int pin;//потенциометр
    int currentVolume;//текущее значение громкости после всех преобразований
    int currentVolumeHardwere;// текущее значение громкости для резистора
    int currentVolumeSoftwere;// текущее значение громкости для блютуза
    int targetVolumeHardwere;// целевое значение громкости
    int targetVolumeSoftwere;// целевое  значение громкости
    bool source;// 0 - непосредственное управление; 1 - програмное
    bool currentSource;
    long checkTime;
    unsigned long prevMillis; // последний момент смены состояния

  public:
    VolumeControl(int analogPin, long timeInterval) {
      pin = analogPin;
      currentVolumeHardwere = hardwereRead();
      source = false;
      currentSource = false;
      currentVolumeSoftwere = currentVolumeHardwere;
      targetVolumeHardwere = currentVolumeHardwere;
      targetVolumeSoftwere = currentVolumeHardwere;
      currentVolume = currentVolumeHardwere;
      checkTime = timeInterval;
      prevMillis = 0;
    }
    void Update(unsigned long currentMillis)
    {
      if ((currentMillis - prevMillis >= checkTime))
      {
        prevMillis = currentMillis; // запоминаем момент времени
        setVolume();
      }
    }
    int hardwereRead() {
      // http://arduino.ru/Reference/Map
      int val = analogRead(pin);
      targetVolumeHardwere = constrain(map(val, 0, 1023, 0, 30), 0, 30);
      return targetVolumeHardwere;
    }
    //    int softwereRead() {
    //      return targetVolumeSoftwere;
    //    }
    int softwereSet(int val) {
      targetVolumeSoftwere = constrain(map(val, 0, 127, 0, 30), 0, 30);
    }
    void setVolume() {
      bool changed = false;
      int stepperCheck = currentVolume;
      hardwereRead();
      //      targetVolumeHardwere = hardwereRead();
      //      targetVolumeSoftwere = softwereRead();
      if (targetVolumeHardwere != currentVolumeHardwere) {
        currentVolumeHardwere = targetVolumeHardwere;
        source = false;
        currentSource = false;
        currentVolume = currentVolumeHardwere;
        changed = true;
      }
      else if (targetVolumeSoftwere != currentVolumeSoftwere) {
        currentVolumeSoftwere = targetVolumeSoftwere;
        source = true;
        currentVolume = currentVolumeSoftwere;
        changed = true;
      }

      if (changed) {
        //отображение громкости
        tm1637.display(currentVolume);
        //Отправляем ответ устройству
        bluetoothSerial.println("Громкость: " + String(currentVolume));
        //поворачиваем степпер
        stepper.step(map(currentVolume - stepperCheck, -30, 30, -100, 100));
        //mp3
        mp3_set_volume(currentVolume);
//        delay(100);
      }
    }
};

VolumeControl vc(volumeAnalogPin, 500);

//класс для управления ограничениями и прерываниями степпера
class StepperControl
{
    typedef enum {
      ACTION_OFF,     // set stepper "OFF".
      ACTION_FORWARD, // set stepper "FORWARD"
      ACTION_BACKWARD // set stepper "BACKWARD"
    }
    StepperActions;

    StepperActions nextAction;
    int forwardSteps;
    int currentForwardSteps;
    int backwardSteps;
    int currentBackwardSteps;
    int stepperSpeed;
    unsigned long prevMillis; // последний момент смены состояния

  public:
    StepperControl() {
      forwardSteps = 0;
      currentForwardSteps = 0;
      backwardSteps = 0;
      currentBackwardSteps = 0;
      prevMillis = 0;
      stepperSpeed = 3;

      nextAction = ACTION_OFF; // на старте остановлен
    }

    void setStepperSpeed (int value) {
      stepperSpeed = value;
    }

    void setTargets(int forward, int backward) {
      forwardSteps = forward;
      backwardSteps = backward;
    }

    void Update(unsigned long currentMillis)
    {
      unsigned long checkTime;
      int doSteps;
      int restSteps;
      checkTime = STEPS / stepperSpeed;
      if ((currentMillis - prevMillis >= checkTime))
      {
        prevMillis = currentMillis; // запоминаем момент времени


        if (nextAction == ACTION_OFF) {
          //set all to low level
        } else if (nextAction == ACTION_FORWARD) {

          // прокрутим вперёд несколько шагов
          doSteps = 10;
          restSteps = forwardSteps - doSteps;
          if (restSteps > 0 ) {
            stepper.step(doSteps);
            forwardSteps = restSteps;
          }
          else {
            stepper.step(forwardSteps - restSteps);
            forwardSteps = 0;
            nextAction = ACTION_BACKWARD;
          }
        } else if (nextAction == ACTION_BACKWARD) {

          // прокрутим назад несколько шагов
          doSteps = 10;
          restSteps = backwardSteps - doSteps;
          if (restSteps > 0 ) {
            stepper.step(-1 * doSteps);
            backwardSteps = restSteps;
          }
          else {
            stepper.step(-1 * (backwardSteps - restSteps));
            backwardSteps = 0;
            nextAction = ACTION_OFF;
          }
        }
      }
    }

};

StepperControl stepperControl;

void setup() {

  //Inerrupt 0 ассоциировано с цифровым пином 2. ждем «спада» фронта сигнала на этом выводе.
  //При нажатии кнопки сигнал «спадает» от высокого до низкого уровня и вызывается обработчик прерывания Reset.
  pinMode(2, INPUT_PULLUP);
  attachInterrupt(0, checkStateImmidiatly, FALLING);

  //  pinMode(bluetoothVCC, OUTPUT);
  //  digitalWrite(bluetoothVCC, HIGH);

  //  pinMode(tmVCC, OUTPUT);
  //  digitalWrite(tmVCC, HIGH);
  //  pinMode(tmGND, OUTPUT);
  //  digitalWrite(tmGND, LOW);

  //  pinMode(6, OUTPUT);
  //  pinMode(7, OUTPUT);
  //  pinMode(8, OUTPUT);
  //  pinMode(9, OUTPUT);
  //  pinMode(10, OUTPUT);
  //  pinMode(11, OUTPUT);
  //  pinMode(12, OUTPUT);
  pinMode(LED, OUTPUT);

  button1.attachClick(click1);
  button1.attachDoubleClick(doubleclick1);

  //порт плеера
  mp3Serial.begin(9600);
  // mp3_set_serial (Serial);    //set Serial for DFPlayer-mini mp3 module

  //  delay(1000);
  //порт блютуза
  bluetoothSerial.begin(9600);
  //мониторинг со стороны консоли среды разработки arduino (инструменты->монитор порта)
  Serial.begin(9600);

  tm1637.init();// инициализация библиотеки «TM1637.h»
  tm1637.set(BRIGHT_TYPICAL);//установка яркости указанная константа равна 2, значение по умолчанию

}

//нажатие кнопки
void click1() {
  mp3_next (); // Следующий трек

  bluetoothSerial.println("Клик!");
}

//даблклик
void doubleclick1() {
  stepperControl.setTargets(random(0, 4000), random(0, 4000)); // установить цели для вращения вперёд и назад
  bluetoothSerial.println("Даблклик!");
}

//закончили жать на кнопку
void longPressStop1() {
  mp3_stop (); //стоп
  bluetoothSerial.println("Отпустили!");
}

void checkStateImmidiatly() {
  vc.setVolume();
}

void loop() {
  unsigned long now = millis();

  // put your main code here, to run repeatedly:
  //Если данные пришли
  if (bluetoothSerial.available() > 0) {

    String readString;
    int i = 0;
    while (bluetoothSerial.available()) {
      delay(3);  //пауза для того, чтобы буфер наполнился
      if (bluetoothSerial.available() > 0) {
        char c = bluetoothSerial.read();  //получить один байт из порта
        if (c == '\n' || i > 3) {
          break;
        }
        i++;
        readString += c; //дополняем прочитаную строку
      }
    }

    //    Serial.println(readString);
    //0й байт - номер цифрового пина для управления.
    //1й байт - код действия.
    int incomingBytePin = readString[0];
    int incomingByteState = readString[1];
    Serial.println(readString + ": " + incomingBytePin + "-" + incomingByteState + " " + readString.length());
    //на "1" повесим действия
    if (incomingBytePin == 1) {
      switch (incomingByteState) {
        case 1:
          click1();
          break;
        case 2:
          longPressStop1();
          break;
        default:

          break;
      }
    }

    //2й байт - флаг громкости 3й байт - громкость
    if (constrain(readString[2], 0, 1)) {
      vc.softwereSet(readString[3]);
    }

    //2 байта: № пина, состояние
    //Получаем номер пина
    //и нужное нам действие за счет получения остатка от деления на 2:
    //(1 - зажечь, 0 - погасить)
    int pin = constrain(incomingBytePin, 13, 13);
    int state = incomingByteState % 2;
    digitalWrite(pin, state);


    //Отправляем ответ устройству
    bluetoothSerial.println(String(pin) + ' ' + (state ? "ON" : "OFF"));
    //Отправляем состояние по сериалу на монитор порта
    Serial.println(String(pin) + ' ' + (state ? " ON" : " OFF"));

    //отобразить на табло номер пина и состояние
    //tm1637 http://робопро.рф/?p=41
    tm1637.clearDisplay();
    tm1637.display(0, incomingBytePin);
    tm1637.display(3, incomingByteState);

  }
  else {
    //    bluetoothSerial.println("PING");
    df.Update(now);
  }

  vc.Update(now);
  stepperControl.Update(now);

  // Проверка нажатия кнопок:
  button1.tick();

  // Небольшая пауза для кнопок
  delay(10);
}
