// ============================================================================
// Hardware.cpp — implementação das classes de Hardware.h.
// ============================================================================
#include "Hardware.h"

// ---------------------------------------------------------------- Motor
MotorTracao::MotorTracao(uint8_t pinoPwm, unsigned long rampaAceleracaoMs, unsigned long rampaFrenagemMs)
    : pino_(pinoPwm), rampaAceleracaoMs_(rampaAceleracaoMs), rampaFrenagemMs_(rampaFrenagemMs) {}

void MotorTracao::iniciar() {
  pinMode(pino_, OUTPUT);
  pararImediatamente();
}

void MotorTracao::definirAlvo(uint8_t pwm) { alvo_ = pwm; }

void MotorTracao::pararImediatamente() {
  alvo_ = 0;
  atual_ = 0;
  aplicar(0);
}

void MotorTracao::atualizar(unsigned long agora) {
  const unsigned long decorrido = agora - ultimaAtualizacao_;
  ultimaAtualizacao_ = agora;
  if (atual_ == alvo_) {
    resto_ = 0;
    return;
  }
  const bool acelerando = alvo_ > atual_;
  if (acelerando != acelerandoAntes_) {
    resto_ = 0;  // mudou de acelerar para frear (ou o contrário): começa a conta do zero
    acelerandoAntes_ = acelerando;
  }
  const unsigned long rampa = acelerando ? rampaAceleracaoMs_ : rampaFrenagemMs_;
  unsigned long passo = 255;
  if (rampa > 0) {
    // 255 pontos de PWM em "rampa" ms; a sobra fica para a próxima volta do loop.
    resto_ += decorrido * 255UL;
    passo = resto_ / rampa;
    resto_ %= rampa;
  }
  if (passo == 0) {
    return;
  }

  const unsigned long diferenca = acelerando ? (alvo_ - atual_) : (atual_ - alvo_);
  if (passo >= diferenca) {
    atual_ = alvo_;
  } else if (acelerando) {
    atual_ = static_cast<uint8_t>(atual_ + passo);
  } else {
    atual_ = static_cast<uint8_t>(atual_ - passo);
  }
  aplicar(atual_);
}

void MotorTracao::aplicar(uint8_t pwm) { analogWrite(pino_, pwm); }

// ---------------------------------------------------------------- Potenciômetro
PotenciometroVelocidade::PotenciometroVelocidade(uint8_t pino, uint8_t pwmMinimo, uint8_t pwmMaximo)
    : pino_(pino), pwmMinimo_(pwmMinimo), pwmMaximo_(pwmMaximo) {}

uint8_t PotenciometroVelocidade::lerPwm() const {
  const long leitura = analogRead(pino_);
  return static_cast<uint8_t>(map(leitura, 0, 1023, pwmMinimo_, pwmMaximo_));
}

// ---------------------------------------------------------------- Strategy (LDR)
LeituraAnalogicaLDR::LeituraAnalogicaLDR(const int* limiares, bool escuroEhValorAlto)
    : limiares_(limiares), escuroEhValorAlto_(escuroEhValorAlto) {}

bool LeituraAnalogicaLDR::tremPresente(uint8_t estacao, uint8_t pino) const {
  const int valor = analogRead(pino);
  return escuroEhValorAlto_ ? (valor >= limiares_[estacao]) : (valor <= limiares_[estacao]);
}

int LeituraAnalogicaLDR::valorBruto(uint8_t pino) const { return analogRead(pino); }

LeituraDigitalLDR::LeituraDigitalLDR(uint8_t nivelComTrem) : nivelComTrem_(nivelComTrem) {}

bool LeituraDigitalLDR::tremPresente(uint8_t, uint8_t pino) const {
  return digitalRead(pino) == nivelComTrem_;
}

int LeituraDigitalLDR::valorBruto(uint8_t pino) const { return digitalRead(pino); }

// ---------------------------------------------------------------- Sensores
SensoresEstacao::SensoresEstacao(const uint8_t* pinos, uint8_t quantidade, EstrategiaLeituraLDR& estrategia)
    : pinos_(pinos), quantidade_(quantidade), estrategia_(&estrategia) {}

void SensoresEstacao::iniciar() {
  for (uint8_t i = 0; i < quantidade_; i++) {
    pinMode(pinos_[i], INPUT);
  }
}

uint8_t SensoresEstacao::estacoesComTrem() const {
  uint8_t mascara = 0;
  for (uint8_t i = 0; i < quantidade_; i++) {
    if (estrategia_->tremPresente(i, pinos_[i])) {
      mascara |= static_cast<uint8_t>(1u << i);
    }
  }
  return mascara;
}

bool SensoresEstacao::tremNaEstacao(uint8_t estacao) const {
  return estrategia_->tremPresente(estacao, pinos_[estacao]);
}

int SensoresEstacao::valorBruto(uint8_t estacao) const { return estrategia_->valorBruto(pinos_[estacao]); }

uint8_t SensoresEstacao::contarAtivos(uint8_t mascara) {
  uint8_t total = 0;
  while (mascara != 0) {
    total += mascara & 1u;
    mascara >>= 1;
  }
  return total;
}

uint8_t SensoresEstacao::primeiraAtiva(uint8_t mascara) {
  uint8_t indice = 0;
  while (mascara != 0 && (mascara & 1u) == 0) {
    mascara >>= 1;
    indice++;
  }
  return indice;
}

// ---------------------------------------------------------------- LEDs
Sinalizacao::Sinalizacao(const uint8_t* pinosEstacao, uint8_t quantidade, uint8_t pinoAviso,
                         unsigned long periodoPiscaMs)
    : pinosEstacao_(pinosEstacao), quantidade_(quantidade), pinoAviso_(pinoAviso), periodoPiscaMs_(periodoPiscaMs) {}

void Sinalizacao::iniciar() {
  for (uint8_t i = 0; i < quantidade_; i++) {
    pinMode(pinosEstacao_[i], OUTPUT);
  }
  pinMode(pinoAviso_, OUTPUT);
  apagarEstacoes();
  avisoPiscando(false);
}

void Sinalizacao::acenderEstacao(uint8_t estacao) {
  for (uint8_t i = 0; i < quantidade_; i++) {
    digitalWrite(pinosEstacao_[i], i == estacao ? HIGH : LOW);
  }
}

void Sinalizacao::apagarEstacoes() {
  for (uint8_t i = 0; i < quantidade_; i++) {
    digitalWrite(pinosEstacao_[i], LOW);
  }
}

void Sinalizacao::avisoPiscando(bool ligado) {
  piscando_ = ligado;
  avisoAceso_ = ligado;
  digitalWrite(pinoAviso_, ligado ? HIGH : LOW);
}

void Sinalizacao::atualizar(unsigned long agora) {
  if (!piscando_) {
    ultimaTroca_ = agora;
    return;
  }
  if (agora - ultimaTroca_ >= periodoPiscaMs_) {
    ultimaTroca_ = agora;
    avisoAceso_ = !avisoAceso_;
    digitalWrite(pinoAviso_, avisoAceso_ ? HIGH : LOW);
  }
}

// ---------------------------------------------------------------- Adapter (áudio)
AudioDFPlayer::AudioDFPlayer(SoftwareSerial& serial) : serial_(serial) {}

void AudioDFPlayer::iniciar() { serial_.begin(9600); }

void AudioDFPlayer::definirVolume(uint8_t volume) { enviar(CMD_VOLUME, volume > 30 ? 30 : volume); }

void AudioDFPlayer::tocarFaixa(uint16_t faixa) { enviar(CMD_TOCAR_PASTA_MP3, faixa); }

void AudioDFPlayer::atualizar() {
  // O DFPlayer manda avisos (ex.: fim da faixa). Eles não são usados, então
  // são descartados para não encher o buffer da serial.
  while (serial_.available() > 0) {
    serial_.read();
  }
}

// Quadro: 7E FF 06 CMD 00 PARAM_H PARAM_L SOMA_H SOMA_L EF
// Soma de verificação = 0 - (FF + 06 + CMD + 00 + PARAM_H + PARAM_L).
void AudioDFPlayer::montarQuadro(uint8_t comando, uint16_t parametro, uint8_t quadro[TAMANHO_QUADRO]) {
  quadro[0] = 0x7E;
  quadro[1] = 0xFF;
  quadro[2] = 0x06;
  quadro[3] = comando;
  quadro[4] = 0x00;  // sem pedido de confirmação (não trava o loop)
  quadro[5] = static_cast<uint8_t>(parametro >> 8);
  quadro[6] = static_cast<uint8_t>(parametro & 0xFF);
  uint16_t soma = 0;
  for (uint8_t i = 1; i <= 6; i++) {
    soma = static_cast<uint16_t>(soma + quadro[i]);
  }
  const uint16_t verificacao = static_cast<uint16_t>(0 - soma);
  quadro[7] = static_cast<uint8_t>(verificacao >> 8);
  quadro[8] = static_cast<uint8_t>(verificacao & 0xFF);
  quadro[9] = 0xEF;
}

void AudioDFPlayer::enviar(uint8_t comando, uint16_t parametro) {
  uint8_t quadro[TAMANHO_QUADRO];
  montarQuadro(comando, parametro, quadro);
  serial_.write(quadro, TAMANHO_QUADRO);
}

// ---------------------------------------------------------------- Adapter (desvio)
DesvioServo::DesvioServo(uint8_t pino, uint8_t anguloPrincipal, uint8_t anguloDesvio, unsigned long tempoMovimentoMs)
    : pino_(pino), anguloPrincipal_(anguloPrincipal), anguloDesvio_(anguloDesvio), tempoMovimentoMs_(tempoMovimentoMs) {}

void DesvioServo::iniciar(Rota inicial) { mover(inicial); }

void DesvioServo::posicionar(Rota rota) { mover(rota); }

void DesvioServo::mover(Rota rota) {
  rota_ = rota;
  servo_.write(angulo(rota));  // o ângulo é definido antes do attach para evitar um tranco (RF01)
  if (!conectado_) {
    servo_.attach(pino_);
    conectado_ = true;
  }
  inicioMovimento_ = ultimoInstante_;
}

void DesvioServo::atualizar(unsigned long agora) {
  ultimoInstante_ = agora;
  if (conectado_ && agora - inicioMovimento_ >= tempoMovimentoMs_) {
    soltar();
  }
}

void DesvioServo::soltar() {
  // Espera o fim do pulso atual (no máximo ~2,4 ms). Soltar no meio do pulso
  // deixaria o pino em HIGH.
  for (uint16_t i = 0; i < 3000 && digitalRead(pino_) == HIGH; i++) {
    delayMicroseconds(1);
  }
  servo_.detach();
  digitalWrite(pino_, LOW);
  conectado_ = false;
}

uint8_t DesvioServo::angulo(Rota rota) const {
  return rota == Rota::PRINCIPAL ? anguloPrincipal_ : anguloDesvio_;
}
