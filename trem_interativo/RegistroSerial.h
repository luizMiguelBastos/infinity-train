// ============================================================================
// RegistroSerial.h — mostra no Monitor Serial os comandos e as transições.
// É um ouvinte (Padrão Observer) do painel e do trem: serve para conferir, nos
// testes, que o trem segue exatamente as setas do diagrama de estados.
// ============================================================================
#ifndef REGISTRO_SERIAL_H
#define REGISTRO_SERIAL_H

#include <Arduino.h>
#include "Estado.h"
#include "Interfaces.h"

class RegistroSerial final : public OuvinteComandos, public OuvinteTransicoes {
public:
  void aoComando(Comando comando) override;
  void aoMudarEstado(const Estado& anterior, const Estado& novo, const __FlashStringHelper* evento,
                     unsigned long instante) override;
  void aoSelecionarRota(Rota rota) override;
};

#endif
