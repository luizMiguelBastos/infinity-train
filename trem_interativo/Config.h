// ============================================================================
// Config.h — Projeto Infinity Train
// Pinos conforme o documento "Mapeamento de Componentes" (seções 2 e 3).
// Tempos e limites são valores iniciais: calibrar nos testes (PB06 e PB24).
// ============================================================================
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

const uint8_t NUM_ESTACOES = 4;

namespace Pinos {
const uint8_t BOTAO_PARTIDA = 2;
const uint8_t BOTAO_PARADA = 3;        // INT1: interrupção externa (RNF02)
const uint8_t BOTAO_TROCA_TRILHO = 4;
const uint8_t MOTOR_PWM = 5;           // entrada A-IA do driver L9110S (A-IB no GND)
const uint8_t SERVO_DESVIO = 9;        // a biblioteca Servo desativa o PWM do D9 e do D10
const uint8_t DFPLAYER_RX = 10;        // recebe do TX do DFPlayer
const uint8_t DFPLAYER_TX = 11;        // vai ao RX do DFPlayer, com resistor de 1 kΩ
const uint8_t LED_AVISO = 13;
const uint8_t POTENCIOMETRO = A0;
const uint8_t LED_ESTACAO[NUM_ESTACOES] = {6, 7, 8, 12};
const uint8_t LDR_ESTACAO[NUM_ESTACOES] = {A1, A2, A3, A4};
}  // namespace Pinos

namespace Tempos {
const unsigned long DEBOUNCE_MS = 50;
const unsigned long CONFIRMACAO_DETECCAO_MS = 40;    // DETECTANDO: LDR coberto por este tempo
const unsigned long TIMEOUT_SEM_ESTACAO_MS = 20000;  // EM MOVIMENTO: sem estação = FALHA
const unsigned long TEMPO_PARADA_MS = 6000;          // NA ESTAÇÃO (RF06)
const unsigned long MOVIMENTO_SERVO_MS = 400;        // tempo para o servo chegar na posição; depois ele é solto
const unsigned long ESPERA_SERVO_MS = 450;           // NA ESTAÇÃO: o áudio só sai depois de o servo ser solto
const unsigned long LIBERACAO_SENSOR_MS = 300;       // LDR descoberto por este tempo = trem saiu de cima dele
const unsigned long RAMPA_ACELERACAO_MS = 500;       // de 0 a 255 de PWM
const unsigned long RAMPA_FRENAGEM_MS = 150;         // de 255 a 0 de PWM (PARANDO)
const unsigned long PISCA_AVISO_MS = 250;            // FALHA: LED de aviso piscando
const unsigned long INICIALIZACAO_DFPLAYER_MS = 2000;
const unsigned long INTERVALO_CALIBRACAO_MS = 200;
}  // namespace Tempos

// KPI do TAP: áudio em até 1 s depois da parada física.
static_assert(Tempos::ESPERA_SERVO_MS < 1000, "O áudio precisa começar em até 1 s depois da parada");
static_assert(Tempos::TEMPO_PARADA_MS > Tempos::ESPERA_SERVO_MS, "O tempo de parada precisa cobrir o áudio");
static_assert(Tempos::ESPERA_SERVO_MS > Tempos::MOVIMENTO_SERVO_MS, "O áudio só pode sair com o servo já solto");

namespace Velocidade {
const uint8_t PWM_MINIMO = 90;   // menor PWM que ainda move o trem (definir em teste)
const uint8_t PWM_MAXIMO = 255;
}  // namespace Velocidade

namespace Ldr {
// Padrão Strategy: true = leitura analógica (saída AO do módulo); false = digital (saída DO).
const bool USAR_LEITURA_ANALOGICA = true;
// Leitura analógica: no módulo HW-072, o valor costuma subir quando o LDR fica no escuro.
const bool ESCURO_E_VALOR_ALTO = true;
const int LIMIAR[NUM_ESTACOES] = {600, 600, 600, 600};  // um limiar por estação (risco R01)
// Leitura digital: nível da saída DO quando o trem cobre o LDR.
const uint8_t NIVEL_DIGITAL_COM_TREM = HIGH;
}  // namespace Ldr

namespace Som {
// Cartão microSD: pasta MP3 com 0001.mp3 a 0004.mp3 (chegada + nome da estação 1 a 4)
// e 0005.mp3 (som de partida).
const uint8_t VOLUME = 20;  // de 0 a 30; protege o alto-falante de 0,5 W
const uint16_t FAIXA_PARTIDA = 5;
inline uint16_t faixaChegada(uint8_t estacao) { return static_cast<uint16_t>(estacao + 1); }
}  // namespace Som

namespace Agulha {
const uint8_t ANGULO_ROTA_PRINCIPAL = 60;  // calibrar com a agulha montada
const uint8_t ANGULO_ROTA_DESVIO = 120;
}  // namespace Agulha

namespace Depuracao {
const unsigned long VELOCIDADE_SERIAL = 115200;
// true = só imprime as leituras dos LDRs e do potenciômetro (calibração do PB06).
const bool MODO_CALIBRACAO_LDR = false;
}  // namespace Depuracao

#endif
