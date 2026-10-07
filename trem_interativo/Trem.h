// ============================================================================
// Trem.h — contexto da máquina de estados (Padrão State).
//
// O Trem guarda o estado atual e repassa a ele cada volta do loop() e cada
// comando do painel. Também é ouvinte do painel (Padrão Observer) e avisa os
// seus próprios ouvintes a cada transição (usado para o registro no Serial).
// ============================================================================
#ifndef TREM_H
#define TREM_H

#include <Arduino.h>
#include "Estado.h"
#include "Hardware.h"
#include "Interfaces.h"

class Trem final : public OuvinteComandos {
public:
  static const uint8_t MAX_OUVINTES = 3;

  Trem(MotorTracao& motor, SensoresEstacao& sensores, PotenciometroVelocidade& potenciometro,
       Sinalizacao& sinalizacao, IAudio& audio, IDesvio& desvio);

  bool adicionarOuvinte(OuvinteTransicoes& ouvinte);
  void iniciar(unsigned long agora);    // liga o hardware e entra em PARADO (RF01)
  void atualizar(unsigned long agora);  // chamado a cada volta do loop()
  void aoComando(Comando comando) override;

  // Usado pelos estados.
  void mudarPara(Estado& novo, const __FlashStringHelper* evento);
  const Estado& estadoAtual() const { return *estado_; }
  unsigned long instante() const { return agora_; }
  unsigned long tempoNoEstado() const { return agora_ - entradaNoEstado_; }
  unsigned long tempoNoTrecho() const { return agora_ - inicioDoTrecho_; }
  void iniciarTrecho() { inicioDoTrecho_ = agora_; }
  void seguirPotenciometro() { motor_.definirAlvo(potenciometro_.lerPwm()); }

  // LDRs cobertos, sem contar os que o trem ainda não deixou desde a partida.
  uint8_t sensoresAtivos() const;
  void ignorarSensoresCobertos();

  void definirEstacaoCandidata(uint8_t estacao) { estacaoCandidata_ = estacao; }
  uint8_t estacaoCandidata() const { return estacaoCandidata_; }
  void registrarChegada(uint8_t estacao);
  uint8_t estacaoAtual() const { return estacaoAtual_; }
  uint8_t proximaEstacao() const { return proximaEstacao_; }

  Rota rotaSelecionada() const { return rotaSelecionada_; }
  void alternarRota();

  MotorTracao& motor() { return motor_; }
  SensoresEstacao& sensores() { return sensores_; }
  Sinalizacao& sinalizacao() { return sinalizacao_; }
  IAudio& audio() { return audio_; }
  IDesvio& desvio() { return desvio_; }

private:
  MotorTracao& motor_;
  SensoresEstacao& sensores_;
  PotenciometroVelocidade& potenciometro_;
  Sinalizacao& sinalizacao_;
  IAudio& audio_;
  IDesvio& desvio_;

  Estado* estado_;
  unsigned long agora_ = 0;
  unsigned long entradaNoEstado_ = 0;
  unsigned long inicioDoTrecho_ = 0;
  uint8_t sensoresIgnorados_ = 0;
  unsigned long ignoradosLivresDesde_ = 0;
  uint8_t estacaoCandidata_ = 0;
  uint8_t estacaoAtual_ = 0;
  uint8_t proximaEstacao_ = 0;
  Rota rotaSelecionada_ = Rota::PRINCIPAL;
  void atualizarSensoresIgnorados();
  OuvinteTransicoes* ouvintes_[MAX_OUVINTES];
  uint8_t totalOuvintes_ = 0;
};

#endif
