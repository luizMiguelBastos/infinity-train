// ============================================================================
// PainelControle.h — botões de partida, parada e troca de trilho (HU02, HU06, HU07).
//
// Padrão Observer: o painel é o "sujeito". Quando um botão é apertado, ele avisa
// todos os ouvintes cadastrados (o Trem e o registro no Monitor Serial), sem
// precisar saber o que cada um faz com o comando.
// ============================================================================
#ifndef PAINEL_CONTROLE_H
#define PAINEL_CONTROLE_H

#include <Arduino.h>
#include "Interfaces.h"

// Botão ligado entre o pino e o GND, com INPUT_PULLUP (apertado = LOW) e
// tratamento de repique (debounce).
class Botao {
public:
  Botao(uint8_t pino, unsigned long debounceMs);
  void iniciar();
  bool foiApertado(unsigned long agora);  // true uma única vez por aperto

private:
  uint8_t pino_;
  unsigned long debounceMs_;
  bool leituraAnterior_ = false;
  bool estadoEstavel_ = false;
  unsigned long ultimaMudanca_ = 0;
};

class PainelControle {
public:
  static const uint8_t MAX_OUVINTES = 3;

  PainelControle(uint8_t pinoPartida, uint8_t pinoParada, uint8_t pinoTrocaTrilho, unsigned long debounceMs);
  bool adicionarOuvinte(OuvinteComandos& ouvinte);
  void iniciar();
  void atualizar(unsigned long agora);

private:
  void notificar(Comando comando);
  static void aoInterromperParada();  // rotina de interrupção do botão de parada (INT1)
  static volatile bool pedidoParada_;

  Botao partida_;
  Botao trocaTrilho_;
  uint8_t pinoParada_;
  unsigned long debounceMs_;
  unsigned long ultimaParada_ = 0;
  bool travaParada_ = false;  // aguarda soltar o botão antes de aceitar outro aperto
  OuvinteComandos* ouvintes_[MAX_OUVINTES];
  uint8_t totalOuvintes_ = 0;
};

#endif
