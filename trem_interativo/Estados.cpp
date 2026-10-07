// ============================================================================
// Estados.cpp — ações e transições de cada estado.
// Os textos dos eventos são os mesmos rótulos das setas do diagrama, e aparecem
// no Monitor Serial a cada transição.
// ============================================================================
#include "Estados.h"
#include "Config.h"
#include "Trem.h"

// Instâncias únicas (Singleton).
EstadoParado EstadoParado::unica_;
EstadoEmMovimento EstadoEmMovimento::unica_;
EstadoDetectando EstadoDetectando::unica_;
EstadoParando EstadoParando::unica_;
EstadoNaEstacao EstadoNaEstacao::unica_;
EstadoRetomando EstadoRetomando::unica_;
EstadoFalha EstadoFalha::unica_;

EstadoParado& EstadoParado::instancia() { return unica_; }
EstadoEmMovimento& EstadoEmMovimento::instancia() { return unica_; }
EstadoDetectando& EstadoDetectando::instancia() { return unica_; }
EstadoParando& EstadoParando::instancia() { return unica_; }
EstadoNaEstacao& EstadoNaEstacao::instancia() { return unica_; }
EstadoRetomando& EstadoRetomando::instancia() { return unica_; }
EstadoFalha& EstadoFalha::instancia() { return unica_; }

// ---------------------------------------------------------------- Em operação
void EstadoEmOperacao::aoComando(Trem& trem, Comando comando) {
  if (comando == Comando::PARADA) {
    trem.mudarPara(EstadoParado::instancia(), F("botão de parada"));
  }
}

// ---------------------------------------------------------------- PARADO
const __FlashStringHelper* EstadoParado::nome() const { return F("PARADO"); }

void EstadoParado::entrar(Trem& trem) {
  trem.motor().pararImediatamente();
  trem.sinalizacao().apagarEstacoes();
}

void EstadoParado::aoComando(Trem& trem, Comando comando) {
  if (comando == Comando::PARTIDA) {
    trem.iniciarTrecho();
    trem.ignorarSensoresCobertos();
    trem.mudarPara(EstadoEmMovimento::instancia(), F("botão de partida"));
  }
}

// ---------------------------------------------------------------- EM MOVIMENTO
const __FlashStringHelper* EstadoEmMovimento::nome() const { return F("EM MOVIMENTO"); }

void EstadoEmMovimento::atualizar(Trem& trem) {
  // Ação: motor em PWM conforme o potenciômetro. O farol acende sozinho,
  // porque está ligado em paralelo com o motor (Mapeamento de Componentes).
  trem.seguirPotenciometro();

  // Ação: monitora os 4 LDRs.
  const uint8_t cobertos = trem.sensoresAtivos();
  if (cobertos != 0) {
    trem.definirEstacaoCandidata(SensoresEstacao::primeiraAtiva(cobertos));
    trem.mudarPara(EstadoDetectando::instancia(), F("pulso detectado no LDR"));
    return;
  }

  if (trem.tempoNoTrecho() >= Tempos::TIMEOUT_SEM_ESTACAO_MS) {
    trem.mudarPara(EstadoFalha::instancia(), F("timeout sem estação"));
  }
}

// ---------------------------------------------------------------- DETECTANDO
const __FlashStringHelper* EstadoDetectando::nome() const { return F("DETECTANDO"); }

void EstadoDetectando::atualizar(Trem& trem) {
  trem.seguirPotenciometro();

  const uint8_t cobertos = trem.sensoresAtivos();
  const uint8_t esperado = static_cast<uint8_t>(1u << trem.estacaoCandidata());

  // O trem só pode estar sobre um LDR por vez.
  if (SensoresEstacao::contarAtivos(cobertos) > 1 || (cobertos != 0 && cobertos != esperado)) {
    trem.mudarPara(EstadoFalha::instancia(), F("sensor incoerente"));
  } else if (cobertos == 0) {
    trem.mudarPara(EstadoEmMovimento::instancia(), F("leitura inválida"));
  } else if (trem.tempoNoEstado() >= Tempos::CONFIRMACAO_DETECCAO_MS) {
    trem.mudarPara(EstadoParando::instancia(), F("detecção confirmada"));
  }
}

// ---------------------------------------------------------------- PARANDO
const __FlashStringHelper* EstadoParando::nome() const { return F("PARANDO"); }

void EstadoParando::entrar(Trem& trem) {
  trem.motor().definirAlvo(0);  // frenagem com a rampa curta
}

void EstadoParando::atualizar(Trem& trem) {
  if (trem.motor().parado()) {
    trem.mudarPara(EstadoNaEstacao::instancia(), F("velocidade = 0"));
  }
}

// ---------------------------------------------------------------- NA ESTAÇÃO
const __FlashStringHelper* EstadoNaEstacao::nome() const { return F("NA ESTAÇÃO"); }

void EstadoNaEstacao::entrar(Trem& trem) {
  const uint8_t estacao = trem.estacaoCandidata();
  trem.registrarChegada(estacao);                 // ação: atualiza próxima estação
  trem.sinalizacao().acenderEstacao(estacao);     // ação: LED da estação aceso
  trem.desvio().posicionar(trem.rotaSelecionada());  // ação: posiciona servo para a rota
  audioEnviado_ = false;
}

void EstadoNaEstacao::atualizar(Trem& trem) {
  // Ação: anúncio em áudio (DFPlayer). O comando sai depois do servo, porque a
  // SoftwareSerial pausa as interrupções e faria o servo tremer.
  if (!audioEnviado_ && trem.tempoNoEstado() >= Tempos::ESPERA_SERVO_MS) {
    trem.audio().tocarFaixa(Som::faixaChegada(trem.estacaoAtual()));
    audioEnviado_ = true;
  }

  if (trem.tempoNoEstado() >= Tempos::TEMPO_PARADA_MS) {
    trem.mudarPara(EstadoRetomando::instancia(), F("tempo de parada concluído"));
  }
}

void EstadoNaEstacao::sair(Trem& trem) {
  trem.sinalizacao().apagarEstacoes();
}

// ---------------------------------------------------------------- RETOMANDO
const __FlashStringHelper* EstadoRetomando::nome() const { return F("RETOMANDO"); }

void EstadoRetomando::entrar(Trem& trem) {
  trem.audio().tocarFaixa(Som::FAIXA_PARTIDA);  // som de partida (RF07)
  sensorLivre_ = false;
}

void EstadoRetomando::atualizar(Trem& trem) {
  trem.seguirPotenciometro();
  // "Sensor liberado" só vale com o LDR descoberto por LIBERACAO_SENSOR_MS seguidos;
  // uma oscilação na saída da estação não faz o trem parar de novo nela.
  if (trem.sensores().tremNaEstacao(trem.estacaoAtual())) {
    sensorLivre_ = false;
  } else if (!sensorLivre_) {
    sensorLivre_ = true;
    livreDesde_ = trem.instante();
  } else if (trem.instante() - livreDesde_ >= Tempos::LIBERACAO_SENSOR_MS) {
    trem.iniciarTrecho();
    trem.mudarPara(EstadoEmMovimento::instancia(), F("sensor liberado"));
  }
}

// ---------------------------------------------------------------- FALHA
const __FlashStringHelper* EstadoFalha::nome() const { return F("FALHA"); }

void EstadoFalha::entrar(Trem& trem) {
  trem.motor().pararImediatamente();          // ação: motor desligado
  trem.sinalizacao().apagarEstacoes();
  trem.sinalizacao().avisoPiscando(true);     // ação: LED de aviso piscando
}

void EstadoFalha::aoComando(Trem& trem, Comando comando) {
  // Ação: aguarda reset. O reset é feito pelo botão de parada.
  if (comando == Comando::PARADA) {
    trem.mudarPara(EstadoParado::instancia(), F("reset"));
  }
}

void EstadoFalha::sair(Trem& trem) {
  trem.sinalizacao().avisoPiscando(false);
}
