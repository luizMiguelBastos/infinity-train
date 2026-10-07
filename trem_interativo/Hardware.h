// ============================================================================
// Hardware.h — classes que controlam cada componente do Mapeamento de
// Componentes: motor, potenciômetro, LDRs, LEDs, DFPlayer e servo.
// ============================================================================
#ifndef HARDWARE_H
#define HARDWARE_H

#include <Arduino.h>
#include <Servo.h>
#include <SoftwareSerial.h>
#include "Interfaces.h"

// ---------------------------------------------------------------- Motor
// Motor DC via driver L9110S, com rampa de aceleração e de frenagem para
// reduzir o pico de corrente na partida (risco R02).
class MotorTracao {
public:
  MotorTracao(uint8_t pinoPwm, unsigned long rampaAceleracaoMs, unsigned long rampaFrenagemMs);
  void iniciar();
  void definirAlvo(uint8_t pwm);   // muda a velocidade aos poucos (rampa)
  void pararImediatamente();       // corta o motor na hora (parada e falha)
  void atualizar(unsigned long agora);
  bool parado() const { return atual_ == 0 && alvo_ == 0; }
  uint8_t pwmAtual() const { return atual_; }

private:
  void aplicar(uint8_t pwm);
  uint8_t pino_;
  unsigned long rampaAceleracaoMs_;
  unsigned long rampaFrenagemMs_;
  uint8_t atual_ = 0;
  uint8_t alvo_ = 0;
  unsigned long ultimaAtualizacao_ = 0;
  unsigned long resto_ = 0;  // fração de PWM acumulada entre as voltas do loop
  bool acelerandoAntes_ = true;
};

// ---------------------------------------------------------------- Potenciômetro
class PotenciometroVelocidade {
public:
  PotenciometroVelocidade(uint8_t pino, uint8_t pwmMinimo, uint8_t pwmMaximo);
  uint8_t lerPwm() const;  // leitura de 0 a 1023 convertida para PWM mínimo..máximo (RF03)

private:
  uint8_t pino_;
  uint8_t pwmMinimo_;
  uint8_t pwmMaximo_;
};

// ---------------------------------------------------------------- Strategy (LDR)
class LeituraAnalogicaLDR final : public EstrategiaLeituraLDR {
public:
  LeituraAnalogicaLDR(const int* limiares, bool escuroEhValorAlto);
  bool tremPresente(uint8_t estacao, uint8_t pino) const override;
  int valorBruto(uint8_t pino) const override;

private:
  const int* limiares_;
  bool escuroEhValorAlto_;
};

class LeituraDigitalLDR final : public EstrategiaLeituraLDR {
public:
  explicit LeituraDigitalLDR(uint8_t nivelComTrem);
  bool tremPresente(uint8_t estacao, uint8_t pino) const override;
  int valorBruto(uint8_t pino) const override;

private:
  uint8_t nivelComTrem_;
};

// ---------------------------------------------------------------- Sensores
// Os 4 LDRs das estações. A forma de leitura é uma estratégia trocável.
class SensoresEstacao {
public:
  SensoresEstacao(const uint8_t* pinos, uint8_t quantidade, EstrategiaLeituraLDR& estrategia);
  void iniciar();
  void trocarEstrategia(EstrategiaLeituraLDR& estrategia) { estrategia_ = &estrategia; }
  uint8_t estacoesComTrem() const;          // bit N ligado = trem sobre o LDR da estação N
  bool tremNaEstacao(uint8_t estacao) const;
  int valorBruto(uint8_t estacao) const;
  uint8_t quantidade() const { return quantidade_; }
  static uint8_t contarAtivos(uint8_t mascara);
  static uint8_t primeiraAtiva(uint8_t mascara);

private:
  const uint8_t* pinos_;
  uint8_t quantidade_;
  EstrategiaLeituraLDR* estrategia_;
};

// ---------------------------------------------------------------- LEDs
// LEDs das estações (NA ESTAÇÃO) e LED de aviso (FALHA). O farol e os postes
// não usam pino: ficam ligados direto na alimentação (Mapeamento de Componentes).
class Sinalizacao {
public:
  Sinalizacao(const uint8_t* pinosEstacao, uint8_t quantidade, uint8_t pinoAviso, unsigned long periodoPiscaMs);
  void iniciar();
  void acenderEstacao(uint8_t estacao);
  void apagarEstacoes();
  void avisoPiscando(bool ligado);
  void atualizar(unsigned long agora);

private:
  const uint8_t* pinosEstacao_;
  uint8_t quantidade_;
  uint8_t pinoAviso_;
  unsigned long periodoPiscaMs_;
  bool piscando_ = false;
  bool avisoAceso_ = false;
  unsigned long ultimaTroca_ = 0;
};

// ---------------------------------------------------------------- Adapter (áudio)
// Converte as chamadas de IAudio nos quadros de 10 bytes do protocolo serial
// do DFPlayer Mini. Não precisa de biblioteca externa.
class AudioDFPlayer final : public IAudio {
public:
  static const uint8_t TAMANHO_QUADRO = 10;
  static const uint8_t CMD_VOLUME = 0x06;
  static const uint8_t CMD_TOCAR_PASTA_MP3 = 0x12;  // toca /MP3/NNNN.mp3

  explicit AudioDFPlayer(SoftwareSerial& serial);
  void iniciar() override;
  void definirVolume(uint8_t volume) override;
  void tocarFaixa(uint16_t faixa) override;
  void atualizar() override;
  static void montarQuadro(uint8_t comando, uint16_t parametro, uint8_t quadro[TAMANHO_QUADRO]);

private:
  void enviar(uint8_t comando, uint16_t parametro);
  SoftwareSerial& serial_;
};

// ---------------------------------------------------------------- Adapter (desvio)
// Converte "rota" em ângulo do servo SG90. Depois de chegar na posição, o servo
// é solto (detach): assim a SoftwareSerial do DFPlayer não faz o servo tremer.
class DesvioServo final : public IDesvio {
public:
  DesvioServo(uint8_t pino, uint8_t anguloPrincipal, uint8_t anguloDesvio, unsigned long tempoMovimentoMs);
  void iniciar(Rota inicial) override;
  void posicionar(Rota rota) override;
  void atualizar(unsigned long agora) override;
  Rota rotaAtual() const override { return rota_; }
  bool conectado() const { return conectado_; }

private:
  uint8_t angulo(Rota rota) const;
  void mover(Rota rota);
  void soltar();
  Servo servo_;
  uint8_t pino_;
  uint8_t anguloPrincipal_;
  uint8_t anguloDesvio_;
  unsigned long tempoMovimentoMs_;
  Rota rota_ = Rota::PRINCIPAL;
  bool conectado_ = false;
  unsigned long inicioMovimento_ = 0;
  unsigned long ultimoInstante_ = 0;
};

#endif
