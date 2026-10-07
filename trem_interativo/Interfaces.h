// ============================================================================
// Interfaces.h — contratos usados entre as partes do sistema.
// As interfaces não têm destrutor virtual porque nenhum objeto é criado com
// "new": todos são estáticos (boa prática no Arduino Uno, que tem 2 KB de RAM).
// ============================================================================
#ifndef INTERFACES_H
#define INTERFACES_H

#include <Arduino.h>

class Estado;

// ---------------------------------------------------------------- Comandos
// Comandos do painel do usuário (HU02, HU06 e HU07).
enum class Comando : uint8_t { PARTIDA, PARADA, TROCA_TRILHO };

// Rotas possíveis no ponto de desvio (HU06).
enum class Rota : uint8_t { PRINCIPAL, DESVIO };

// ---------------------------------------------------------------- Observer
// Padrão Observer: quem quer saber dos comandos do painel implementa esta interface.
class OuvinteComandos {
public:
  virtual void aoComando(Comando comando) = 0;

protected:
  ~OuvinteComandos() = default;
};

// Padrão Observer: quem quer saber das transições da máquina de estados.
class OuvinteTransicoes {
public:
  virtual void aoMudarEstado(const Estado& anterior, const Estado& novo,
                             const __FlashStringHelper* evento, unsigned long instante) = 0;
  virtual void aoSelecionarRota(Rota) {}

protected:
  ~OuvinteTransicoes() = default;
};

// ---------------------------------------------------------------- Adapter
// Padrão Adapter: interface de áudio que o sistema usa. A classe AudioDFPlayer
// adapta o protocolo serial do DFPlayer Mini para esta interface.
class IAudio {
public:
  virtual void iniciar() = 0;
  virtual void definirVolume(uint8_t volume) = 0;
  virtual void tocarFaixa(uint16_t faixa) = 0;
  virtual void atualizar() = 0;

protected:
  ~IAudio() = default;
};

// Padrão Adapter: interface do ponto de desvio. A classe DesvioServo adapta a
// biblioteca Servo (ângulos) para esta interface (rotas).
class IDesvio {
public:
  virtual void iniciar(Rota inicial) = 0;
  virtual void posicionar(Rota rota) = 0;
  virtual void atualizar(unsigned long agora) = 0;
  virtual Rota rotaAtual() const = 0;

protected:
  ~IDesvio() = default;
};

// ---------------------------------------------------------------- Strategy
// Padrão Strategy: forma de decidir se o trem está sobre o LDR. Há duas
// estratégias (analógica e digital) porque o módulo HW-072 pode ter saída AO ou só DO.
class EstrategiaLeituraLDR {
public:
  virtual bool tremPresente(uint8_t estacao, uint8_t pino) const = 0;
  virtual int valorBruto(uint8_t pino) const = 0;  // usado na calibração

protected:
  ~EstrategiaLeituraLDR() = default;
};

#endif
