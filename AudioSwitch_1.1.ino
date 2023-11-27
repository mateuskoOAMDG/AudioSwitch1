/*
 * @brief:   Audioswitch with IR Controller
 * @author: (c) mateusko O.A.M.D.G
 * @date: 2023-07-21
 * @version: 1.1.0
 * @microcontroller: ATtiny1624, 20 MHz
 * @PCB: ver. 1.1.3
 * */

/* Program pouziva kniznicu IRRemote, do ktorej som pridal podporu pre ATtiny1624 */

#include <EEPROM.h>
#include <IRremote.hpp>

#define SW_MAX          3       //pocet tlacitok
#define PIN_K1_SET      PIN_PA2 //HW pin 12
#define PIN_K1_RESET    PIN_PA1 //HW pin 11
#define PIN_K2_SET      PIN_PA3 //HW pin 13
#define PIN_K2_RESET    PIN_PA4 //HW pin 2
#define PIN_LED1        PIN_PB1 //HW pin 8
#define PIN_LED2        PIN_PB2 //HW pin 7
#define PIN_LED3        PIN_PB3 //HW pin 6
#define PIN_SW1         PIN_PB0 //HW pin 9
#define PIN_SW2         PIN_PA7 //HW pin 5
#define PIN_SW3         PIN_PA6 //HW pin 4
#define PIN_IR          PIN_PA5 //HW pin 3

#define SWITCH_TIME   50 //cas zopnutia rele
#define DEBOUNCE_TIME 20  //cas debouncingu
#define LONG_TIME     2000 //cas dlheho stlacena tlacidla
#define SAVE_TIMEOUT  5000 //cas, kym je zaznam aktivny (storage)

uint8_t setinput = 0; //ktory vstup je aktivny (1,2,3) 0=nevieme, (po zapnuti)

/// @brief Trieda Button obsluhuje tlacitko. Vykonava sw debouncing. Ma dve metody onPress a OnLong na citanie stavu tlacitka.
///        Pravidelne treba volat update() na citanie stavu tlacitka a obsluhu.
class Button {
  int _pin;
  bool _indebounce; //prebieha debouncing
  bool _state; //posledny zaznamenany stav tlacitka true=stlacene
  bool _read;  //onPress bol precitany
  bool _inlong; //prebieha citanie Long
  bool _longclick; //nastala udalost long
  uint16_t _debounceTimer;
  public:
  Button (int pin) {
    _pin = pin;
    pinMode(_pin, INPUT_PULLUP);
    _state = false; //stav tlacitka
    _longclick = false;
    _inlong = false;
    _read = false;
    _indebounce = false; //prebieha debouncing
  }
  /// @brief Updatuje stav tlacitka, musi sa volat v cykle aspon raz za 10ms
  void update() {
    int state = !digitalRead(_pin); //HIGH=0, LOW=1
    if (state == _state) {
      if (_indebounce) {
        if (uint16_t(millis()) - _debounceTimer > DEBOUNCE_TIME) {
          _indebounce = false; //osetrene; preslo debouncingom
          _inlong = true;
        }
      } else {
        if(state && _inlong) {
          if (uint16_t(millis()) - _debounceTimer > LONG_TIME) {
            _longclick = true;
            _inlong = false;
          }
        }
      }

    } else {
      _debounceTimer = millis();
      _indebounce = true;
      _inlong = false;
      _state = state;
      _read = false;
    }
  }

  /// @brief Testuje, ci bolo stlacene tlacitko
  /// @return True = bolo stlacene (generuje sa po stlaceni len 1x)
  bool onPress() {
    if (!_indebounce && _state && !_read ){
      _read = true;
      return true;
    } else {
      return false;
    }

  }

  /// @brief Testuje, ci bolo "dlho" stlacene tlacitko
  /// @return True = bolo "dlho" stlacene (generuje sa po stlaceni len 1x)
  bool onLong() {
    if (_longclick) {
      _longclick = false;
      return true;
    } else {
      return false;
    }
  }
};


const int ledsinit[] = {PIN_LED1, PIN_LED2, PIN_LED3, PIN_LED3, PIN_LED2, PIN_LED1, PIN_LED1, PIN_LED2, PIN_LED3,-1}; 
const int leds[SW_MAX] =  {PIN_LED1, PIN_LED2, PIN_LED3};


void testBlinkLeds() {
  uint8_t index = 0;
  while (ledsinit[index] >= 0) {
    digitalWrite(ledsinit[index], HIGH);
    delay(75);
    digitalWrite(ledsinit[index], LOW);
    delay(10);
    index ++;
  }
}


/// Sparovane tlacitka
/// pamat protokolov 
uint16_t storageProtocol[SW_MAX] = {0, 0, 0};
/// pamat rawdata
uint32_t storageData[SW_MAX] = {0, 0, 0};

void eepromSave() {
  uint8_t addr = 0;
  for (int i = 0; i < SW_MAX; i++) {
    EEPROM.put(addr, storageProtocol[i]);
    addr += sizeof(storageProtocol[i]);
  }

  for (int i = 0; i < SW_MAX; i++) {
  EEPROM.put(addr, storageData[i]);
  addr += sizeof(storageData[i]);
  }
}

void eepromLoad() {
  uint8_t addr = 0;
  for (int i = 0; i < SW_MAX; i++) {
    EEPROM.get(addr, storageProtocol[i]);
    addr += sizeof(storageProtocol[i]);
  }

  for (int i = 0; i < SW_MAX; i++) {
  EEPROM.get(addr, storageData[i]);
  addr += sizeof(storageData[i]);
  }
}


/// @brief Prijatie kodu IR ovladaca
/// @param protocol cislo protokolu
/// @param data raw data 32bit
/// @return true = Bolo prijate
bool getIRCode(uint16_t& protocol, uint32_t& data) {
  if (IrReceiver.decode()) {

        protocol = IrReceiver.decodedIRData.protocol;
        data = IrReceiver.decodedIRData.decodedRawData;
        IrReceiver.resume();
        return true;
  }
  return false;
}

/// @brief Rozpoznanie IR kodu podla ulozeneho storage*
/// @return cislo storage 0-(SW_MAX-1); alebo SW_MAX ak nebola zhoda
uint8_t recognizeIR(uint16_t& protocol, uint32_t& data) {
  if ((protocol == 0) || (data == 0)) return SW_MAX;
  uint8_t i;
  for (i = 0; i < SW_MAX; i++ ) {
    if (protocol == storageProtocol[i])
      if (data == storageData[i]) return i;
  }
  return SW_MAX;
}

/// @brief Ulozi protokol a data do storage [num]
/// @param num cislo tlacitka/pamate
/// @param protocol 
/// @param data 
/// @return true = ulozene,

bool saveIR(uint8_t num, uint16_t& protocol, uint32_t& data) {
   if ((protocol == 0) || (data == 0) || (num >= SW_MAX)) return false; //chyba, neulozi sa
   storageProtocol[num] = protocol;
   storageData[num] = data;
   return true;
}

bool storaging(uint8_t sw) {
  uint16_t protocol;
  uint32_t data;
  uint16_t timer = millis();
  uint16_t timer_timeout = millis();
  if (sw >= SW_MAX) return false;
  int led = leds[sw];
  while (uint16_t(millis()) - timer_timeout < SAVE_TIMEOUT) {
    if (getIRCode(protocol, data)) {
        saveIR(sw, protocol, data);
        eepromSave();
        digitalWrite(led, HIGH);
        for(uint8_t i = 0; i < 3; i++) {
          digitalWrite(led, HIGH);
          delay(500);
          digitalWrite(led, LOW);
          delay(100);
        }
        digitalWrite(led, HIGH);
        return true;
    } else if (uint16_t(millis()) - timer > 50) {
      digitalWrite(led, !digitalRead(led));
      timer = millis();
    }
  }
  digitalWrite(led, HIGH);
  return true;
}

Button sw1(PIN_SW1);
Button sw2(PIN_SW2);
Button sw3(PIN_SW3);


void switch1() { //zapne audiovstup 1
  if (setinput == 1) return;
  setinput = 1;
  digitalWrite(PIN_LED1, HIGH);
  digitalWrite(PIN_LED2, LOW);
  digitalWrite(PIN_LED3, LOW);

  digitalWrite(PIN_K1_RESET, HIGH);
  delay(SWITCH_TIME);
  digitalWrite(PIN_K1_RESET, LOW);
  digitalWrite(PIN_K2_RESET, HIGH);
  delay(SWITCH_TIME);
  digitalWrite(PIN_K2_RESET, LOW);
}

void switch2() { //zapne audiovstup 2
  if (setinput == 2) return;
  setinput = 2;
  digitalWrite(PIN_LED1, LOW);
  digitalWrite(PIN_LED2, HIGH);
  digitalWrite(PIN_LED3, LOW);

  digitalWrite(PIN_K1_SET, HIGH);
  delay(SWITCH_TIME);
  digitalWrite(PIN_K1_SET, LOW);
  digitalWrite(PIN_K2_RESET, HIGH);
  delay(SWITCH_TIME);
  digitalWrite(PIN_K2_RESET, LOW);
}

void switch3() { //zapne audiovstup 3
  if (setinput == 3) return;
  setinput = 3;
  digitalWrite(PIN_LED1, LOW);
  digitalWrite(PIN_LED2, LOW);
  digitalWrite(PIN_LED3, HIGH);

  digitalWrite(PIN_K2_SET, HIGH);
  delay(SWITCH_TIME);
  digitalWrite(PIN_K2_SET, LOW);
}




void setup() {
  // put your setup code here, to run once:
  pinMode(PIN_K1_SET, OUTPUT);
  pinMode(PIN_K1_RESET, OUTPUT);
  pinMode(PIN_K2_SET, OUTPUT);
  pinMode(PIN_K2_RESET, OUTPUT);

  pinMode(PIN_LED1, OUTPUT);
  pinMode(PIN_LED2, OUTPUT);
  pinMode(PIN_LED3, OUTPUT);

  testBlinkLeds();
  eepromLoad();
  IrReceiver.begin(PIN_IR, false);

  digitalWrite(PIN_LED1, LOW);
  digitalWrite(PIN_LED2, LOW);
  digitalWrite(PIN_LED3, LOW);
}




uint16_t protocol;
uint32_t data;
void loop() {
  
  if (sw1.onPress()) {
    switch1();
  } else if (sw2.onPress()) {
    switch2();
  } else if (sw3.onPress()) {
    switch3();
  } else if (sw1.onLong()) {
    storaging(0);
  } else if (sw2.onLong()) {
    storaging(1);
  } else if (sw3.onLong()) {
    storaging(2);
  } else {
    if(getIRCode(protocol, data))
      switch(recognizeIR(protocol, data)) {
        case 0: switch1(); break;
        case 1: switch2(); break;
        case 2: switch3(); break;
      }
  }

  sw1.update();
  sw2.update();
  sw3.update();
}

//O.A.M.D.G 2023-06-23
