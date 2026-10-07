// Simulador da SoftwareSerial: guarda os bytes enviados ao DFPlayer. NÃO vai para o Arduino.
#ifndef SIM_SOFTWARE_SERIAL_H
#define SIM_SOFTWARE_SERIAL_H

#include <stddef.h>
#include <stdint.h>
#include <vector>

namespace sim {
extern std::vector<uint8_t> bytesDfplayer;
extern unsigned long baudDfplayer;
extern bool servoConectado;
extern int bytesComServoConectado;  // bytes enviados enquanto o servo recebia pulsos
}  // namespace sim

class SoftwareSerial {
public:
  SoftwareSerial(uint8_t, uint8_t) {}
  void begin(long baud) { sim::baudDfplayer = static_cast<unsigned long>(baud); }
  size_t write(uint8_t b) {
    if (sim::servoConectado) sim::bytesComServoConectado++;
    sim::bytesDfplayer.push_back(b);
    return 1;
  }
  size_t write(const uint8_t* dados, size_t tamanho) {
    for (size_t i = 0; i < tamanho; i++) {
      write(dados[i]);
    }
    return tamanho;
  }
  int available() { return 0; }
  int read() { return -1; }
};

#endif
