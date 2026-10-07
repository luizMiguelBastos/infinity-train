// ============================================================================
// Trem.cpp
// ============================================================================
#include "Trem.h"
#include "Config.h"
#include "Estados.h"

Trem::Trem(MotorTracao& motor, SensoresEstacao& sensores, PotenciometroVelocidade& potenciometro,
           Sinalizacao& sinalizacao, IAudio& audio, IDesvio& desvio)
    : motor_(motor),
      sensores_(sensores),
      potenciometro_(potenciometro),
      sinalizacao_(sinalizacao),
      audio_(audio),
      desvio_(desvio),
      estado_(&EstadoParado::instancia()) {}

bool Trem::adicionarOuvinte(OuvinteTransicoes& ouvinte) {
  if (totalOuvintes_ >= MAX_OUVINTES) {
    return false;
  }
  ouvintes_[totalOuvintes_++] = &ouvinte;
  return true;
}

void Trem::iniciar(unsigned long agora) {
  // O motor é o primeiro a ser iniciado, para garantir que o trem não se mexa ao ligar (RF01).
  motor_.iniciar();
  sensores_.iniciar();
  sinalizacao_.iniciar();
  desvio_.iniciar(rotaSelecionada_);

  agora_ = agora;
  entradaNoEstado_ = agora;
  inicioDoTrecho_ = agora;
  estado_ = &EstadoParado::instancia();  // estado inicial do diagrama
  estado_->entrar(*this);
}

void Trem::atualizar(unsigned long agora) {
  agora_ = agora;
  atualizarSensoresIgnorados();
  estado_->atualizar(*this);
  motor_.atualizar(agora);
  desvio_.atualizar(agora);
  sinalizacao_.atualizar(agora);
  audio_.atualizar();
}

uint8_t Trem::sensoresAtivos() const {
  return static_cast<uint8_t>(sensores_.estacoesComTrem() & ~sensoresIgnorados_);
}

void Trem::ignorarSensoresCobertos() {
  // Se o trem partir em cima de um LDR, ele não deve parar de novo nessa mesma estação.
  sensoresIgnorados_ = sensores_.estacoesComTrem();
  ignoradosLivresDesde_ = agora_;
}

void Trem::atualizarSensoresIgnorados() {
  if (sensoresIgnorados_ == 0) {
    return;
  }
  if ((sensores_.estacoesComTrem() & sensoresIgnorados_) != 0) {
    ignoradosLivresDesde_ = agora_;
  } else if (agora_ - ignoradosLivresDesde_ >= Tempos::LIBERACAO_SENSOR_MS) {
    sensoresIgnorados_ = 0;  // o trem já saiu de cima: volta a valer
  }
}

void Trem::aoComando(Comando comando) {
  if (comando == Comando::TROCA_TRILHO) {
    // A rota escolhida é aplicada no estado NA ESTAÇÃO ("posiciona servo para a rota"),
    // sempre com o trem parado (risco R09).
    alternarRota();
    return;
  }
  estado_->aoComando(*this, comando);
}

void Trem::mudarPara(Estado& novo, const __FlashStringHelper* evento) {
  Estado& anterior = *estado_;
  anterior.sair(*this);
  estado_ = &novo;
  entradaNoEstado_ = agora_;
  novo.entrar(*this);
  for (uint8_t i = 0; i < totalOuvintes_; i++) {
    ouvintes_[i]->aoMudarEstado(anterior, novo, evento, agora_);
  }
}

void Trem::registrarChegada(uint8_t estacao) {
  estacaoAtual_ = estacao;
  proximaEstacao_ = static_cast<uint8_t>((estacao + 1) % NUM_ESTACOES);
}

void Trem::alternarRota() {
  rotaSelecionada_ = (rotaSelecionada_ == Rota::PRINCIPAL) ? Rota::DESVIO : Rota::PRINCIPAL;
  for (uint8_t i = 0; i < totalOuvintes_; i++) {
    ouvintes_[i]->aoSelecionarRota(rotaSelecionada_);
  }
}
