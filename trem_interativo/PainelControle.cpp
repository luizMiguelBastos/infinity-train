// ============================================================================
// PainelControle.cpp
// ============================================================================
#include "PainelControle.h"

// ---------------------------------------------------------------- Botão
Botao::Botao(uint8_t pino, unsigned long debounceMs) : pino_(pino), debounceMs_(debounceMs) {}

void Botao::iniciar() { pinMode(pino_, INPUT_PULLUP); }

bool Botao::foiApertado(unsigned long agora) {
  const bool apertado = digitalRead(pino_) == LOW;
  if (apertado != leituraAnterior_) {
    leituraAnterior_ = apertado;
    ultimaMudanca_ = agora;
  }
  if (apertado != estadoEstavel_ && agora - ultimaMudanca_ >= debounceMs_) {
    estadoEstavel_ = apertado;
    return apertado;  // só gera evento quando o botão desce, não quando sobe
  }
  return false;
}

// ---------------------------------------------------------------- Painel
volatile bool PainelControle::pedidoParada_ = false;

PainelControle::PainelControle(uint8_t pinoPartida, uint8_t pinoParada, uint8_t pinoTrocaTrilho,
                               unsigned long debounceMs)
    : partida_(pinoPartida, debounceMs),
      trocaTrilho_(pinoTrocaTrilho, debounceMs),
      pinoParada_(pinoParada),
      debounceMs_(debounceMs) {}

bool PainelControle::adicionarOuvinte(OuvinteComandos& ouvinte) {
  if (totalOuvintes_ >= MAX_OUVINTES) {
    return false;
  }
  ouvintes_[totalOuvintes_++] = &ouvinte;
  return true;
}

void PainelControle::iniciar() {
  partida_.iniciar();
  trocaTrilho_.iniciar();
  pinMode(pinoParada_, INPUT_PULLUP);
  // Interrupção externa: o aperto é registrado mesmo se o loop estiver ocupado (RNF02).
  attachInterrupt(digitalPinToInterrupt(pinoParada_), aoInterromperParada, FALLING);
}

void PainelControle::aoInterromperParada() { pedidoParada_ = true; }

void PainelControle::atualizar(unsigned long agora) {
  if (travaParada_) {
    // Depois de um aperto, ignora o repique até o botão ficar solto por um tempo de debounce.
    pedidoParada_ = false;
    if (digitalRead(pinoParada_) == LOW) {
      ultimaParada_ = agora;
    } else if (agora - ultimaParada_ >= debounceMs_) {
      travaParada_ = false;
    }
  } else if (pedidoParada_) {
    pedidoParada_ = false;
    // O pino é lido depois de consumir o pedido, para não perder um aperto que
    // chegue entre as duas linhas. Confirma que foi um aperto de verdade, e não ruído.
    if (digitalRead(pinoParada_) == LOW) {
      travaParada_ = true;
      ultimaParada_ = agora;
      notificar(Comando::PARADA);
    }
  }
  if (partida_.foiApertado(agora)) {
    notificar(Comando::PARTIDA);
  }
  if (trocaTrilho_.foiApertado(agora)) {
    notificar(Comando::TROCA_TRILHO);
  }
}

void PainelControle::notificar(Comando comando) {
  for (uint8_t i = 0; i < totalOuvintes_; i++) {
    ouvintes_[i]->aoComando(comando);
  }
}
