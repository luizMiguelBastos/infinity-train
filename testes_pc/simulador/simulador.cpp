// Implementação do simulador do Arduino. NÃO vai para o Arduino.
#include <stdio.h>
#include <vector>
#include "Arduino.h"
#include "Servo.h"
#include "SoftwareSerial.h"

namespace sim {
unsigned long agora = 0;
int nivel[PINOS] = {0};
int analogico[PINOS] = {0};
int pwm[PINOS] = {0};
int modo[PINOS] = {0};
void (*rotinaInterrupcao[2])() = {nullptr, nullptr};
bool mostrarSerial = false;
int anguloServo = -1;
int pinoServo = -1;
int escritasServo = 0;
bool servoConectado = false;
int bytesComServoConectado = 0;
std::vector<uint8_t> bytesDfplayer;
unsigned long baudDfplayer = 0;
}  // namespace sim

SerialSimulado Serial;

void pinMode(uint8_t pino, uint8_t modo) {
  sim::modo[pino] = modo;
  if (modo == INPUT_PULLUP) {
    sim::nivel[pino] = HIGH;  // botão solto
  }
}
void digitalWrite(uint8_t pino, uint8_t valor) { sim::nivel[pino] = valor; }
int digitalRead(uint8_t pino) { return sim::nivel[pino]; }
int analogRead(uint8_t pino) { return sim::analogico[pino]; }
void analogWrite(uint8_t pino, int valor) { sim::pwm[pino] = valor; }
unsigned long millis() { return sim::agora; }
void delay(unsigned long ms) { sim::agora += ms; }
void delayMicroseconds(unsigned int) {}
int digitalPinToInterrupt(uint8_t pino) { return pino == 2 ? 0 : (pino == 3 ? 1 : -1); }
void attachInterrupt(uint8_t interrupcao, void (*rotina)(), int) { sim::rotinaInterrupcao[interrupcao] = rotina; }
long map(long x, long inMin, long inMax, long outMin, long outMax) {
  return (x - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}

size_t SerialSimulado::print(const char* texto) {
  if (sim::mostrarSerial) printf("%s", texto);
  return 0;
}
size_t SerialSimulado::print(const __FlashStringHelper* texto) {
  return print(reinterpret_cast<const char*>(texto));
}
size_t SerialSimulado::print(int valor) {
  if (sim::mostrarSerial) printf("%d", valor);
  return 0;
}
size_t SerialSimulado::print(unsigned int valor) {
  if (sim::mostrarSerial) printf("%u", valor);
  return 0;
}
size_t SerialSimulado::print(long valor) {
  if (sim::mostrarSerial) printf("%ld", valor);
  return 0;
}
size_t SerialSimulado::print(unsigned long valor) {
  if (sim::mostrarSerial) printf("%lu", valor);
  return 0;
}
size_t SerialSimulado::println() {
  if (sim::mostrarSerial) printf("\n");
  return 0;
}
