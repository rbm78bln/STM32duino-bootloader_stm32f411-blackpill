#include <Arduino.h>
#include <string>

String buffer;
bool   bufferComplete;

#ifdef STM32F1xx
#undef LED_BUILTIN
#define LED_BUILTIN PA9
#endif

#ifdef STM32F4xx
#undef LED_BUILTIN
#define LED_BUILTIN PC13
#endif

void setup() {
  // put your setup code here, to run once:
  bufferComplete = false;

  pinMode(LED_BUILTIN, OUTPUT); // LED on
#ifdef STM32F4xx
  pinMode(USER_BTN, INPUT_PULLUP);
#endif

  SerialUSB.begin(115200);
  while(!SerialUSB) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(250);

#ifdef STM32F4xx
    if(digitalRead(USER_BTN) == LOW) {
      pinMode(LED_BUILTIN, INPUT);  // LED off
      __disable_irq();
      NVIC_SystemReset();
      while (true);
    }
#endif
  }
  delay(1000);
  SerialUSB.print(String("\x0d\x0aPlease type something and hit ENTER:\x0d\x0a> "));

  digitalWrite(LED_BUILTIN, HIGH); // LED blue
}

void loop() {
  // put your main code here, to run repeatedly:
  if(SerialUSB.available()>0 && !bufferComplete) {
    while(SerialUSB.available()>0 && !bufferComplete) {
      int c = SerialUSB.read();
      switch (c)
      {
        case 10:
        case 13:
          bufferComplete = buffer.length()>0;
          break;

        default:
          buffer += static_cast<char>(c);
          SerialUSB.print(static_cast<char>(c));
          if(buffer.length()>60) bufferComplete = true;
          break;
      }
    }
  } else {
    delay(100);
    if(!SerialUSB) {
        pinMode(LED_BUILTIN, INPUT);  // LED off
        __disable_irq();
        NVIC_SystemReset();
        while (true);
    }
#ifdef STM32F4xx
    if(digitalRead(USER_BTN) == LOW) {
      pinMode(LED_BUILTIN, INPUT);  // LED off
      __disable_irq();
      NVIC_SystemReset();
      while (true);
    }
#endif
  }
  if(bufferComplete) {
    SerialUSB.print(String("\x0d\x0aYou wrote: ")+buffer+String("\x0d\x0a> "));
    bufferComplete = false;
    buffer.remove(0);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);  // LED red
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH); // LED blue
  }
}