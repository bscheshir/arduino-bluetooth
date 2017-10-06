#include <DFPlayer_Mini_Mp3.h> //http://lesson.iarduino.ru/page/urok-17-podklyuchenie-mini-mp3-pleera-k-arduino/ http://iarduino.ru/file/140.html
#include <Stepper_28BYJ.h> //https://lesson.iarduino.ru/page/upravlenie-shagovym-dvigatelem-s-arduiny/ http://iarduino.ru/file/148.html
#include <OneButton.h>//http://microsin.net/programming/avr/arduino-onebutton-library.html https://github.com/mathertel/OneButton
#include <TM1637.h>//http://робопро.рф/?p=41 https://yadi.sk/d/Ci5aiYzsqo7qk
#include <SoftwareSerial.h>
#include <VolumeControl.h> //https://github.com/bscheshir/volume-control


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
SoftwareSerial mp3Serial(mp3TX, mp3RX); // (RX, TX) --//--

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
          ListDisp[BitSelect] = NumTab[i];
          tm1637.display(BitSelect, ListDisp[BitSelect]);
          i ++;
          if (i == sizeof(NumTab)) i = 0;
        }
      }
    }
};

// объявление "потока цифр"
DigitFlow df(2000);


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
      stepperSpeed = 15;
      setStepperSpeed(stepperSpeed);
      nextAction = ACTION_OFF; // на старте остановлен
    }

    void setStepperSpeed (int value) {
      stepperSpeed = value;
      stepper.setSpeed(stepperSpeed);
      //Serial.println(String("speed") + value);
    }

    void setTargets(int forward, int backward) {
      forwardSteps = forward;
      backwardSteps = backward;
      nextAction = ACTION_FORWARD;
      //Serial.println(String("forward: ") + forward + String(" backward:") + backward);
    }

    void Update(unsigned long currentMillis)
    {
      unsigned long checkTime;
      int doSteps;
      int restSteps;
      checkTime = constrain(STEPS / stepperSpeed * 0.1, 10, STEPS);//21..40
      doSteps = constrain(STEPS / checkTime, 10, STEPS);//10..4000
      if ((currentMillis - prevMillis >= checkTime))
      {
        prevMillis = currentMillis; // запоминаем момент времени

        if (nextAction == ACTION_OFF) {
          //Serial.println(String("nextAction == ACTION_OFF ") + checkTime+ String(":") + doSteps);
          //set all to low level stepper0
          digitalWrite(stepper0, LOW);
          digitalWrite(stepper1, LOW);
          digitalWrite(stepper2, LOW);
          digitalWrite(stepper3, LOW);
        } else if (nextAction == ACTION_FORWARD) {
          //Serial.println(String("nextAction == ACTION_FORWARD"));
          // прокрутим вперёд несколько шагов
          //doSteps = 100;
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
          //Serial.println(String("nextAction == ACTION_BACKWARD"));
          // прокрутим назад несколько шагов
          //doSteps = 100;
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

// VolumeControl callback
void setVolume(int volume) {
  //отображение громкости
  tm1637.display(currentVolume);
  //Отправляем ответ устройству
  bluetoothSerial.println("Громкость: " + String(currentVolume));
  //mp3
  mp3_set_volume(currentVolume);
  //delay(100);
  //устанавливаем скорость степпера
  stepperControl.setStepperSpeed(map(constrain(currentVolume, 0, 30), 0, 30, 10, 20));
}

//non-block volume control class
VolumeControl vc(volumeAnalogPin, 500, &setVolume);

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
  button1.attachLongPressStop(longPressStop1);

  //порт плеера
  mp3Serial.begin(9600);
  mp3_set_serial(mp3Serial);    //set Serial for DFPlayer-mini mp3 module

  delay(100);
  //порт блютуза
  bluetoothSerial.begin(9600);
  //мониторинг со стороны консоли среды разработки arduino (инструменты->монитор порта)
  Serial.begin(9600);

  tm1637.init();// инициализация библиотеки «TM1637.h»
  tm1637.set(BRIGHT_TYPICAL);//установка яркости указанная константа равна 2, значение по умолчанию

}

//нажатие кнопки
void click1() {
  bluetoothSerial.println("Следующий трек");
  mp3_next (); // Следующий трек
}

//даблклик
void doubleclick1() {
  bluetoothSerial.println("Фигурка!");
  stepperControl.setTargets(random(0, 4000), random(0, 4000)); // установить цели для вращения вперёд и назад
}

//закончили жать на кнопку
void longPressStop1() {
  bluetoothSerial.println("Стоп музыка");
  mp3_stop (); //стоп
}

void checkStateImmidiatly() {
}

void loop() {
  unsigned long now = millis();

  //Если данные пришли
  if (bluetoothSerial.available() > 0) {

    String readString;
    int i = 0;
    while (bluetoothSerial.available()) {
      delay(3);  //пауза для того, чтобы буфер наполнился
      if (bluetoothSerial.available() > 0) {
        char c = bluetoothSerial.read();  //получить один байт из порта
        if (c == '\n' || i > 3) {
        delay(30);
          break;
        }
        i++;
        readString += c; //дополняем прочитаную строку
      }
    }
    

    //0й байт - номер цифрового пина для управления.
    //1й байт - код действия.
    int incomingBytePin = readString[0];
    int incomingByteState = readString[1];
    Serial.println(readString + ": " + incomingBytePin + "-" + incomingByteState + " " + readString.length());
    //на "1" повесим действия
    switch (incomingBytePin) {
      case 1: // "пин 1" для кнопки music
        switch (incomingByteState) {
          case 1:
            click1();
            break;
          case 2:
            longPressStop1();
            break;
        }
        break;
      case 0: //"пин 0" для громкости
        //2й байт - флаг громкости 3й байт - громкость
        if (constrain(readString[2], 0, 1)) {
          vc.softwareSet(readString[3]);
        }
        break;
      case 12: //"пин 12" для степпера
        switch (incomingByteState) {
          case 1:
            doubleclick1();
            break;
        }
        break;
      default: //остальные в реальные пины переводим, ограничивая свободными от остальных устройст. Также в setup ставим pinMode(LED, OUTPUT);
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

        break;

    }


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

  vc.update(now);
  stepperControl.Update(now);

  // Проверка нажатия кнопок:
  button1.tick();

  // Небольшая пауза для кнопок
  delay(10);
}
