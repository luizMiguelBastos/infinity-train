// ============================================================================
// Projeto Infinity Train — Sistema de Trem Interativo
// Firmware do Arduino Uno R3, baseado no Diagrama de Máquina de Estados (Semana 4)
// e no Mapeamento de Componentes.
//
// Padrões de projeto aplicados (detalhes no README.md):
//   State      -> Estado.h, Estados.h/.cpp e Trem.h/.cpp
//   Singleton  -> uma instância de cada estado (Estados.h)
//   Observer   -> PainelControle avisa o Trem e o RegistroSerial; o Trem avisa o RegistroSerial
//   Strategy   -> leitura do LDR analógica ou digital (Interfaces.h, Hardware.h)
//   Adapter    -> AudioDFPlayer (protocolo do DFPlayer) e DesvioServo (biblioteca Servo)
//
// Bibliotecas: Servo e SoftwareSerial, que já vêm com a Arduino IDE.
// ============================================================================
#include <Arduino.h>
#include <Servo.h>
#include <SoftwareSerial.h>

#include "Config.h"
#include "Estados.h"
#include "Hardware.h"
#include "PainelControle.h"
#include "RegistroSerial.h"
#include "Trem.h"

// ---------------------------------------------------------------- Objetos
// Todos estáticos: nada de "new" (o Uno tem só 2 KB de RAM).

// Strategy: as duas formas de ler o LDR; Config.h escolhe qual usar.
LeituraAnalogicaLDR leituraAnalogica(Ldr::LIMIAR, Ldr::ESCURO_E_VALOR_ALTO);
LeituraDigitalLDR leituraDigital(Ldr::NIVEL_DIGITAL_COM_TREM);

SensoresEstacao sensores(Pinos::LDR_ESTACAO, NUM_ESTACOES,
                         Ldr::USAR_LEITURA_ANALOGICA ? static_cast<EstrategiaLeituraLDR&>(leituraAnalogica)
                                                     : static_cast<EstrategiaLeituraLDR&>(leituraDigital));
MotorTracao motor(Pinos::MOTOR_PWM, Tempos::RAMPA_ACELERACAO_MS, Tempos::RAMPA_FRENAGEM_MS);
PotenciometroVelocidade potenciometro(Pinos::POTENCIOMETRO, Velocidade::PWM_MINIMO, Velocidade::PWM_MAXIMO);
Sinalizacao sinalizacao(Pinos::LED_ESTACAO, NUM_ESTACOES, Pinos::LED_AVISO, Tempos::PISCA_AVISO_MS);

// Adapters.
SoftwareSerial serialDfplayer(Pinos::DFPLAYER_RX, Pinos::DFPLAYER_TX);
AudioDFPlayer audio(serialDfplayer);
DesvioServo desvio(Pinos::SERVO_DESVIO, Agulha::ANGULO_ROTA_PRINCIPAL, Agulha::ANGULO_ROTA_DESVIO,
                   Tempos::MOVIMENTO_SERVO_MS);

// Contexto do State e ouvintes do Observer.
Trem trem(motor, sensores, potenciometro, sinalizacao, audio, desvio);
PainelControle painel(Pinos::BOTAO_PARTIDA, Pinos::BOTAO_PARADA, Pinos::BOTAO_TROCA_TRILHO, Tempos::DEBOUNCE_MS);
RegistroSerial registro;

unsigned long ultimaCalibracao = 0;

// ---------------------------------------------------------------- Calibração
// Com Depuracao::MODO_CALIBRACAO_LDR = true, o trem não anda: só mostra as
// leituras, para escolher os limiares de cada estação (PB06, risco R01).
void imprimirCalibracao(unsigned long agora) {
  if (agora - ultimaCalibracao < Tempos::INTERVALO_CALIBRACAO_MS) {
    return;
  }
  ultimaCalibracao = agora;
  for (uint8_t i = 0; i < NUM_ESTACOES; i++) {
    Serial.print(F("LDR"));
    Serial.print(i + 1);
    Serial.print(F("="));
    Serial.print(sensores.valorBruto(i));
    Serial.print(F("  "));
  }
  Serial.print(F("PWM do potenciometro="));
  Serial.println(potenciometro.lerPwm());
}

// ---------------------------------------------------------------- setup / loop
void setup() {
  Serial.begin(Depuracao::VELOCIDADE_SERIAL);

  trem.iniciar(millis());  // motor parado, LEDs apagados, servo na rota inicial, estado PARADO

  painel.adicionarOuvinte(registro);  // registra o comando antes da transição
  painel.adicionarOuvinte(trem);
  trem.adicionarOuvinte(registro);
  painel.iniciar();

  // O servo é solto antes de ligar a serial do DFPlayer, para não tremer.
  delay(Tempos::MOVIMENTO_SERVO_MS);
  desvio.atualizar(millis());

  audio.iniciar();
  delay(Tempos::INICIALIZACAO_DFPLAYER_MS);  // o DFPlayer leva até ~2 s para ler o cartão
  audio.definirVolume(Som::VOLUME);

  Serial.println(F("Infinity Train pronto. Estado inicial: PARADO"));
}

void loop() {
  const unsigned long agora = millis();
  if (Depuracao::MODO_CALIBRACAO_LDR) {
    imprimirCalibracao(agora);
    return;
  }
  trem.atualizar(agora);   // primeiro: atualiza o relógio do trem e o estado atual
  painel.atualizar(agora); // depois: os comandos dos botões usam o mesmo instante
}
