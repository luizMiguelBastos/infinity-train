// ============================================================================
// RegistroSerial.cpp
// Exemplo de saída:  [12345 ms] EM MOVIMENTO -> DETECTANDO (pulso detectado no LDR)
// ============================================================================
#include "RegistroSerial.h"

void RegistroSerial::aoComando(Comando comando) {
  Serial.print(F("[comando] "));
  switch (comando) {
    case Comando::PARTIDA:
      Serial.println(F("PARTIDA"));
      break;
    case Comando::PARADA:
      Serial.println(F("PARADA"));
      break;
    case Comando::TROCA_TRILHO:
      Serial.println(F("TROCA DE TRILHO"));
      break;
  }
}

void RegistroSerial::aoMudarEstado(const Estado& anterior, const Estado& novo, const __FlashStringHelper* evento,
                                   unsigned long instante) {
  Serial.print(F("["));
  Serial.print(instante);
  Serial.print(F(" ms] "));
  Serial.print(anterior.nome());
  Serial.print(F(" -> "));
  Serial.print(novo.nome());
  Serial.print(F(" ("));
  Serial.print(evento);
  Serial.println(F(")"));
}

void RegistroSerial::aoSelecionarRota(Rota rota) {
  Serial.print(F("[rota] proxima estacao usa a rota "));
  Serial.println(rota == Rota::PRINCIPAL ? F("PRINCIPAL") : F("DESVIO"));
}
