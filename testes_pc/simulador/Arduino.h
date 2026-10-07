// Simulador mínimo do Arduino para rodar o firmware no computador (testes).
// Imita só as funções usadas pelo projeto. NÃO vai para o Arduino.
#ifndef SIM_ARDUINO_H
#define SIM_ARDUINO_H

#include <stddef.h>
#include <stdint.h>

typedef uint8_t byte;

#define HIGH 0x1
#define LOW 0x0
#define INPUT 0x0
#define OUTPUT 0x1
#define INPUT_PULLUP 0x2
#define CHANGE 1
#define FALLING 2
#define RISING 3

static const uint8_t A0 = 14;
static const uint8_t A1 = 15;
static const uint8_t A2 = 16;
static const uint8_t A3 = 17;
static const uint8_t A4 = 18;
static const uint8_t A5 = 19;

class __FlashStringHelper;
#define F(texto) (reinterpret_cast<const __FlashStringHelper*>(texto))

void pinMode(uint8_t pino, uint8_t modo);
void digitalWrite(uint8_t pino, uint8_t valor);
int digitalRead(uint8_t pino);
int analogRead(uint8_t pino);
void analogWrite(uint8_t pino, int valor);
unsigned long millis();
void delay(unsigned long ms);
void delayMicroseconds(unsigned int us);
int digitalPinToInterrupt(uint8_t pino);
void attachInterrupt(uint8_t interrupcao, void (*rotina)(), int modo);
long map(long x, long inMin, long inMax, long outMin, long outMax);

class SerialSimulado {
public:
  void begin(unsigned long) {}
  size_t print(const char* texto);
  size_t print(const __FlashStringHelper* texto);
  size_t print(int valor);
  size_t print(unsigned int valor);
  size_t print(long valor);
  size_t print(unsigned long valor);
  size_t println();
  template <class T>
  size_t println(T valor) {
    size_t n = print(valor);
    return n + println();
  }
};
extern SerialSimulado Serial;

namespace sim {
const int PINOS = 20;
extern unsigned long agora;
extern int nivel[PINOS];       // nível digital lido ou escrito em cada pino
extern int analogico[PINOS];   // valor de analogRead
extern int pwm[PINOS];         // último analogWrite
extern int modo[PINOS];
extern void (*rotinaInterrupcao[2])();
extern bool mostrarSerial;
}  // namespace sim

#endif
