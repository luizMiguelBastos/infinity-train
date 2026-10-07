// ============================================================================
// Estado.h — Padrão State: interface comum a todos os estados do diagrama.
// Cada estado do "Diagrama de Máquina de Estados" é uma classe que herda daqui.
// ============================================================================
#ifndef ESTADO_H
#define ESTADO_H

#include <Arduino.h>
#include "Interfaces.h"

class Trem;

class Estado {
public:
  virtual const __FlashStringHelper* nome() const = 0;
  virtual void entrar(Trem&) {}               // ações ao entrar no estado
  virtual void atualizar(Trem&) {}            // executado a cada volta do loop()
  virtual void aoComando(Trem&, Comando) {}   // comandos do painel
  virtual void sair(Trem&) {}                 // ações ao sair do estado

protected:
  Estado() = default;
  ~Estado() = default;
};

#endif
