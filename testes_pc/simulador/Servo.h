// Simulador da biblioteca Servo. NÃO vai para o Arduino.
#ifndef SIM_SERVO_H
#define SIM_SERVO_H

#include <stdint.h>

namespace sim {
extern int anguloServo;
extern int pinoServo;
extern int escritasServo;
extern bool servoConectado;
}  // namespace sim

class Servo {
public:
  uint8_t attach(int pino) {
    sim::pinoServo = pino;
    sim::servoConectado = true;
    return 0;
  }
  void detach() { sim::servoConectado = false; }
  bool attached() { return sim::servoConectado; }
  void write(int angulo) {
    sim::anguloServo = angulo;
    sim::escritasServo++;
  }
  int read() { return sim::anguloServo; }
};

#endif
