// ============================================================================
// testes.cpp — roda o firmware inteiro no computador, com o simulador do
// Arduino, e confere cada transição do Diagrama de Máquina de Estados.
// Uso: ./rodar_testes.sh   (ou ./rodar_testes.sh -v para ver o Monitor Serial)
// ============================================================================
#include <stdio.h>
#include <string.h>
#include <set>
#include <string>
#include <vector>

#include "../trem_interativo/trem_interativo.ino"

static int verificacoes = 0;
static int falhas = 0;

#define VERIFICAR(condicao, descricao)                                    \
  do {                                                                    \
    verificacoes++;                                                       \
    if (!(condicao)) {                                                    \
      falhas++;                                                           \
      printf("  FALHOU (linha %d): %s\n", __LINE__, descricao);           \
    }                                                                     \
  } while (0)

static std::string texto(const __FlashStringHelper* t) { return reinterpret_cast<const char*>(t); }
static std::string estado() { return texto(trem.estadoAtual().nome()); }

// Grava cada transição no formato "ANTERIOR -> NOVO (evento)".
class GravadorTransicoes : public OuvinteTransicoes {
public:
  std::vector<std::string> log;
  void aoMudarEstado(const Estado& anterior, const Estado& novo, const __FlashStringHelper* evento,
                     unsigned long) override {
    log.push_back(texto(anterior.nome()) + " -> " + texto(novo.nome()) + " (" + texto(evento) + ")");
  }
  std::string ultima() const { return log.empty() ? "" : log.back(); }
};
static GravadorTransicoes gravador;

// ---------------------------------------------------------------- ajudantes
static void passo() {
  sim::agora++;
  loop();
}
static void avancar(unsigned long ms) {
  for (unsigned long i = 0; i < ms; i++) passo();
}
static bool avancarAte(const std::string& alvo, unsigned long limiteMs) {
  for (unsigned long i = 0; i < limiteMs; i++) {
    passo();
    if (estado() == alvo) return true;
  }
  return false;
}
static void pressionar(uint8_t pino) {
  sim::nivel[pino] = LOW;
  if (pino == Pinos::BOTAO_PARADA && sim::rotinaInterrupcao[1] != nullptr) {
    sim::rotinaInterrupcao[1]();  // borda de descida no INT1
  }
}
static void soltar(uint8_t pino) { sim::nivel[pino] = HIGH; }
static void apertar(uint8_t pino) {
  pressionar(pino);
  avancar(80);
  soltar(pino);
  avancar(80);
}
static void cobrirLdr(uint8_t estacao, bool coberto) {
  sim::analogico[Pinos::LDR_ESTACAO[estacao]] = coberto ? 900 : 200;  // escuro = valor alto
}
static bool ledEstacao(uint8_t estacao) { return sim::nivel[Pinos::LED_ESTACAO[estacao]] == HIGH; }
static int pwmMotor() { return sim::pwm[Pinos::MOTOR_PWM]; }

struct Quadro {
  uint8_t b[10];
};
static std::vector<Quadro> quadrosDfplayer() {
  std::vector<Quadro> q;
  for (size_t i = 0; i + 10 <= sim::bytesDfplayer.size(); i += 10) {
    Quadro x;
    memcpy(x.b, &sim::bytesDfplayer[i], 10);
    q.push_back(x);
  }
  return q;
}
static bool quadroValido(const Quadro& q) {
  unsigned soma = 0;
  for (int i = 1; i <= 6; i++) soma += q.b[i];
  const unsigned verificacao = (static_cast<unsigned>(q.b[7]) << 8) | q.b[8];
  return q.b[0] == 0x7E && q.b[1] == 0xFF && q.b[2] == 0x06 && q.b[9] == 0xEF && ((soma + verificacao) & 0xFFFF) == 0;
}
static unsigned parametro(const Quadro& q) { return (static_cast<unsigned>(q.b[5]) << 8) | q.b[6]; }

// Leva o trem de EM MOVIMENTO até NA ESTAÇÃO na estação indicada.
static bool chegarNaEstacao(uint8_t estacao) {
  cobrirLdr(estacao, true);
  return avancarAte("NA ESTAÇÃO", 500);
}

// ---------------------------------------------------------------- testes
static void testeInicializacao() {
  printf("1. Inicialização (RF01)\n");
  VERIFICAR(estado() == "PARADO", "estado inicial deve ser PARADO");
  VERIFICAR(pwmMotor() == 0 && sim::modo[Pinos::MOTOR_PWM] == OUTPUT, "motor parado ao ligar");
  VERIFICAR(sim::anguloServo == Agulha::ANGULO_ROTA_PRINCIPAL && sim::pinoServo == Pinos::SERVO_DESVIO,
            "servo na rota principal, no pino D9");
  VERIFICAR(!sim::servoConectado, "servo solto antes de ligar a serial do DFPlayer");
  VERIFICAR(sim::bytesComServoConectado == 0, "nenhum byte para o DFPlayer com o servo conectado");
  bool ledsApagados = sim::nivel[Pinos::LED_AVISO] == LOW;
  for (uint8_t i = 0; i < NUM_ESTACOES; i++) ledsApagados = ledsApagados && !ledEstacao(i);
  VERIFICAR(ledsApagados, "LEDs das estações e de aviso apagados");
  VERIFICAR(sim::rotinaInterrupcao[1] != nullptr, "interrupção do botão de parada no INT1 (D3)");
  VERIFICAR(sim::baudDfplayer == 9600, "serial do DFPlayer a 9600");

  const std::vector<Quadro> q = quadrosDfplayer();
  VERIFICAR(q.size() == 1 && quadroValido(q[0]) && q[0].b[3] == 0x06 && parametro(q[0]) == Som::VOLUME,
            "DFPlayer recebe o volume configurado em um quadro válido");

  // Exemplo do manual do DFPlayer: tocar a faixa 1 = 7E FF 06 03 00 00 01 FE F7 EF.
  uint8_t quadro[10];
  AudioDFPlayer::montarQuadro(0x03, 1, quadro);
  const uint8_t esperado[10] = {0x7E, 0xFF, 0x06, 0x03, 0x00, 0x00, 0x01, 0xFE, 0xF7, 0xEF};
  VERIFICAR(memcmp(quadro, esperado, 10) == 0, "quadro igual ao exemplo do manual do DFPlayer");
}

static void testePartidaERampa() {
  printf("2. Partida e controle de velocidade (RF02, RF03)\n");
  apertar(Pinos::BOTAO_PARTIDA);
  VERIFICAR(estado() == "EM MOVIMENTO", "botão de partida leva a EM MOVIMENTO");
  VERIFICAR(gravador.ultima() == "PARADO -> EM MOVIMENTO (botão de partida)", "evento com o rótulo do diagrama");
  VERIFICAR(pwmMotor() > 0 && pwmMotor() < 255, "motor acelera aos poucos (rampa)");
  avancar(500);
  VERIFICAR(pwmMotor() == 255, "potenciômetro no máximo = PWM 255");
  sim::analogico[Pinos::POTENCIOMETRO] = 0;
  avancar(300);
  VERIFICAR(pwmMotor() == Velocidade::PWM_MINIMO, "potenciômetro no mínimo = PWM mínimo");
  sim::analogico[Pinos::POTENCIOMETRO] = 1023;
  avancar(600);
  VERIFICAR(pwmMotor() == 255, "volta ao PWM máximo");
}

static void testeCicloDeEstacao() {
  printf("3. Ciclo completo na estação 1 (RF04 a RF07, HU03, HU04)\n");
  cobrirLdr(0, true);
  passo();
  VERIFICAR(estado() == "DETECTANDO", "LDR coberto leva a DETECTANDO");
  VERIFICAR(gravador.ultima() == "EM MOVIMENTO -> DETECTANDO (pulso detectado no LDR)", "evento do diagrama");
  const unsigned long inicioDeteccao = sim::agora;
  VERIFICAR(avancarAte("PARANDO", 200), "detecção confirmada leva a PARANDO");
  VERIFICAR(sim::agora - inicioDeteccao >= Tempos::CONFIRMACAO_DETECCAO_MS, "confirmação respeita o tempo mínimo");
  VERIFICAR(avancarAte("NA ESTAÇÃO", 500), "velocidade = 0 leva a NA ESTAÇÃO");
  VERIFICAR(gravador.ultima() == "PARANDO -> NA ESTAÇÃO (velocidade = 0)", "evento do diagrama");
  VERIFICAR(pwmMotor() == 0, "motor parado na estação");

  const unsigned long chegada = sim::agora;
  VERIFICAR(ledEstacao(0) && !ledEstacao(1) && !ledEstacao(2) && !ledEstacao(3), "só o LED da estação 1 aceso");
  VERIFICAR(sim::anguloServo == Agulha::ANGULO_ROTA_PRINCIPAL, "servo posicionado para a rota");
  VERIFICAR(sim::servoConectado, "servo conectado enquanto se move");
  VERIFICAR(trem.estacaoAtual() == 0 && trem.proximaEstacao() == 1, "atualiza a próxima estação");

  const size_t quadrosAntes = quadrosDfplayer().size();
  while (quadrosDfplayer().size() == quadrosAntes && sim::agora - chegada < 1500) passo();
  const unsigned long atrasoAudio = sim::agora - chegada;
  std::vector<Quadro> q = quadrosDfplayer();
  VERIFICAR(q.size() == quadrosAntes + 1, "um comando de áudio enviado na estação");
  VERIFICAR(quadroValido(q.back()) && q.back().b[3] == 0x12 && parametro(q.back()) == 1,
            "toca /MP3/0001.mp3 (chegada + nome da estação 1)");
  VERIFICAR(atrasoAudio >= Tempos::ESPERA_SERVO_MS && atrasoAudio <= 1000,
            "áudio sai depois do servo e em até 1 s após a parada (KPI)");
  VERIFICAR(!sim::servoConectado && sim::bytesComServoConectado == 0, "servo já solto quando o áudio é enviado");

  VERIFICAR(avancarAte("RETOMANDO", 7000), "tempo de parada concluído leva a RETOMANDO");
  VERIFICAR(sim::agora - chegada == Tempos::TEMPO_PARADA_MS, "trem fica parado o tempo configurado (RF06)");
  VERIFICAR(!ledEstacao(0), "LED da estação apaga ao sair");
  q = quadrosDfplayer();
  VERIFICAR(quadroValido(q.back()) && q.back().b[3] == 0x12 && parametro(q.back()) == Som::FAIXA_PARTIDA,
            "toca o som de partida (RF07)");
  avancar(100);
  VERIFICAR(estado() == "RETOMANDO" && pwmMotor() > 0, "anda enquanto o LDR ainda está coberto");
  cobrirLdr(0, false);  // oscilação na saída da estação: descobre e cobre de novo
  avancar(50);
  cobrirLdr(0, true);
  avancar(50);
  VERIFICAR(estado() == "RETOMANDO", "oscilação do LDR na saída não conta como sensor liberado");
  cobrirLdr(0, false);
  const unsigned long descoberto = sim::agora;
  VERIFICAR(avancarAte("EM MOVIMENTO", 1000), "sensor liberado leva a EM MOVIMENTO");
  VERIFICAR(sim::agora - descoberto >= Tempos::LIBERACAO_SENSOR_MS, "liberado só depois de 300 ms descoberto");
  VERIFICAR(gravador.ultima() == "RETOMANDO -> EM MOVIMENTO (sensor liberado)", "evento do diagrama");
}

static void testeLeituraInvalida() {
  printf("4. Leitura inválida (ruído no LDR)\n");
  cobrirLdr(1, true);
  passo();
  VERIFICAR(estado() == "DETECTANDO", "pulso curto leva a DETECTANDO");
  avancar(10);
  cobrirLdr(1, false);
  passo();
  VERIFICAR(estado() == "EM MOVIMENTO", "pulso curto volta para EM MOVIMENTO");
  VERIFICAR(gravador.ultima() == "DETECTANDO -> EM MOVIMENTO (leitura inválida)", "evento do diagrama");
  VERIFICAR(pwmMotor() > 0, "trem não para por causa de ruído");
}

static void testeParadaManual() {
  printf("5. Botão de parada em EM MOVIMENTO (RF10, RNF02, HU07)\n");
  avancar(600);
  VERIFICAR(pwmMotor() == 255, "trem andando antes da parada");
  pressionar(Pinos::BOTAO_PARADA);
  passo();
  VERIFICAR(estado() == "PARADO", "botão de parada leva a PARADO");
  VERIFICAR(pwmMotor() == 0, "motor cortado na mesma volta do loop");
  VERIFICAR(gravador.ultima() == "EM MOVIMENTO -> PARADO (botão de parada)", "evento do diagrama");
  avancar(80);
  soltar(Pinos::BOTAO_PARADA);
  const size_t transicoes = gravador.log.size();
  avancar(200);
  VERIFICAR(estado() == "PARADO" && gravador.log.size() == transicoes, "continua parado até nova partida");
}

static void testeTimeout() {
  printf("6. Timeout sem estação e reset\n");
  pressionar(Pinos::BOTAO_PARTIDA);
  VERIFICAR(avancarAte("EM MOVIMENTO", 200), "partida aceita");
  const unsigned long partida = sim::agora;
  avancar(80);
  soltar(Pinos::BOTAO_PARTIDA);
  avancar(15000);
  cobrirLdr(2, true);  // ruído no meio do trecho não reinicia a contagem
  avancar(10);
  cobrirLdr(2, false);
  VERIFICAR(avancarAte("FALHA", 6000), "sem estação por 20 s leva a FALHA");
  VERIFICAR(gravador.ultima() == "EM MOVIMENTO -> FALHA (timeout sem estação)", "evento do diagrama");
  const unsigned long tempo = sim::agora - partida;
  VERIFICAR(tempo == Tempos::TIMEOUT_SEM_ESTACAO_MS, "timeout de 20 s contado desde a partida");
  VERIFICAR(pwmMotor() == 0, "motor desligado em FALHA");
  int trocas = 0;
  int anterior = sim::nivel[Pinos::LED_AVISO];
  for (int i = 0; i < 1000; i++) {
    passo();
    if (sim::nivel[Pinos::LED_AVISO] != anterior) {
      trocas++;
      anterior = sim::nivel[Pinos::LED_AVISO];
    }
  }
  VERIFICAR(trocas >= 3, "LED de aviso piscando");
  apertar(Pinos::BOTAO_PARTIDA);
  VERIFICAR(estado() == "FALHA", "partida é ignorada em FALHA (aguarda reset)");
  apertar(Pinos::BOTAO_PARADA);
  VERIFICAR(estado() == "PARADO", "reset leva a PARADO");
  VERIFICAR(gravador.ultima() == "FALHA -> PARADO (reset)", "evento do diagrama");
  VERIFICAR(sim::nivel[Pinos::LED_AVISO] == LOW, "LED de aviso apagado depois do reset");
}

static void testeSensorIncoerente() {
  printf("7. Sensor incoerente\n");
  apertar(Pinos::BOTAO_PARTIDA);
  cobrirLdr(0, true);
  cobrirLdr(2, true);  // impossível: o trem não está em duas estações ao mesmo tempo
  VERIFICAR(avancarAte("FALHA", 50), "dois LDRs cobertos levam a FALHA");
  VERIFICAR(gravador.ultima() == "DETECTANDO -> FALHA (sensor incoerente)", "evento do diagrama");
  cobrirLdr(0, false);
  cobrirLdr(2, false);
  apertar(Pinos::BOTAO_PARADA);
  VERIFICAR(estado() == "PARADO", "reset depois da falha");
}

static void testeTrocaDeTrilho() {
  printf("8. Troca de trilho (RF09, HU06)\n");
  apertar(Pinos::BOTAO_TROCA_TRILHO);
  VERIFICAR(trem.rotaSelecionada() == Rota::DESVIO, "botão seleciona a rota de desvio");
  VERIFICAR(sim::anguloServo == Agulha::ANGULO_ROTA_PRINCIPAL, "servo só se move com o trem na estação");
  apertar(Pinos::BOTAO_PARTIDA);
  VERIFICAR(chegarNaEstacao(1), "trem chega na estação 2");
  VERIFICAR(sim::anguloServo == Agulha::ANGULO_ROTA_DESVIO, "servo posicionado para a rota de desvio");
  VERIFICAR(trem.estacaoAtual() == 1 && trem.proximaEstacao() == 2, "estação 2, próxima é a 3");
}

static void testeParadaNaEstacaoERetomando() {
  printf("9. Botão de parada em NA ESTAÇÃO e em RETOMANDO (RNF02)\n");
  pressionar(Pinos::BOTAO_PARADA);
  passo();
  VERIFICAR(estado() == "PARADO", "parada na estação leva a PARADO");
  VERIFICAR(!ledEstacao(1), "LED da estação apaga");
  avancar(80);
  soltar(Pinos::BOTAO_PARADA);
  avancar(80);
  cobrirLdr(1, false);

  apertar(Pinos::BOTAO_PARTIDA);
  VERIFICAR(chegarNaEstacao(3), "trem chega na estação 4");
  VERIFICAR(trem.proximaEstacao() == 0, "depois da estação 4 vem a estação 1");
  VERIFICAR(avancarAte("RETOMANDO", 7000), "trem retoma");
  pressionar(Pinos::BOTAO_PARADA);
  passo();
  VERIFICAR(estado() == "PARADO" && pwmMotor() == 0, "parada em RETOMANDO leva a PARADO");
  avancar(80);
  soltar(Pinos::BOTAO_PARADA);
  avancar(80);
  cobrirLdr(3, false);
}

static void testePartidaEmCimaDoLdr() {
  printf("10. Partida com o trem em cima de um LDR\n");
  VERIFICAR(estado() == "PARADO", "começa parado");
  cobrirLdr(2, true);  // trem parado sobre o LDR da estação 3
  apertar(Pinos::BOTAO_PARTIDA);
  avancar(500);
  VERIFICAR(estado() == "EM MOVIMENTO", "não para de novo na estação de onde partiu");
  cobrirLdr(2, false);
  avancar(Tempos::LIBERACAO_SENSOR_MS + 50);
  cobrirLdr(2, true);  // volta à mesma estação na volta seguinte
  passo();
  VERIFICAR(estado() == "DETECTANDO", "na volta seguinte a estação volta a ser detectada");
  cobrirLdr(2, false);
  apertar(Pinos::BOTAO_PARADA);
  VERIFICAR(estado() == "PARADO", "para o trem");
}

static void testeEstrategias() {
  printf("11. Strategy de leitura do LDR\n");
  LeituraDigitalLDR digital(HIGH);
  sim::nivel[A1] = HIGH;
  VERIFICAR(digital.tremPresente(0, A1), "leitura digital: DO em HIGH = trem presente");
  sim::nivel[A1] = LOW;
  VERIFICAR(!digital.tremPresente(0, A1), "leitura digital: DO em LOW = sem trem");

  sensores.trocarEstrategia(digital);
  sim::nivel[A3] = HIGH;
  VERIFICAR(sensores.estacoesComTrem() == 0x04, "sensores usam a estratégia trocada (estação 3)");
  sim::nivel[A3] = LOW;
  sensores.trocarEstrategia(leituraAnalogica);

  const int limiares[NUM_ESTACOES] = {500, 500, 500, 500};
  LeituraAnalogicaLDR claroAlto(limiares, false);
  sim::analogico[A2] = 100;
  VERIFICAR(claroAlto.tremPresente(1, A2), "leitura analógica invertida funciona");
  sim::analogico[A2] = 200;

  VERIFICAR(SensoresEstacao::contarAtivos(0x0B) == 3, "contarAtivos");
  VERIFICAR(SensoresEstacao::primeiraAtiva(0x04) == 2, "primeiraAtiva");
}

static void testeCoberturaDoDiagrama() {
  printf("12. Todas as transições do diagrama foram percorridas\n");
  const char* doDiagrama[] = {
      "PARADO -> EM MOVIMENTO (botão de partida)",
      "EM MOVIMENTO -> DETECTANDO (pulso detectado no LDR)",
      "EM MOVIMENTO -> FALHA (timeout sem estação)",
      "EM MOVIMENTO -> PARADO (botão de parada)",
      "DETECTANDO -> EM MOVIMENTO (leitura inválida)",
      "DETECTANDO -> FALHA (sensor incoerente)",
      "DETECTANDO -> PARANDO (detecção confirmada)",
      "PARANDO -> NA ESTAÇÃO (velocidade = 0)",
      "NA ESTAÇÃO -> RETOMANDO (tempo de parada concluído)",
      "RETOMANDO -> EM MOVIMENTO (sensor liberado)",
      "FALHA -> PARADO (reset)",
  };
  // Acréscimos pelo RNF02 (ver README.md): parada também fora de EM MOVIMENTO.
  const char* acrescimos[] = {
      "DETECTANDO -> PARADO (botão de parada)",
      "PARANDO -> PARADO (botão de parada)",
      "NA ESTAÇÃO -> PARADO (botão de parada)",
      "RETOMANDO -> PARADO (botão de parada)",
  };
  const std::set<std::string> vistas(gravador.log.begin(), gravador.log.end());
  std::set<std::string> permitidas;
  for (const char* t : doDiagrama) {
    permitidas.insert(t);
    const bool percorrida = vistas.count(t) == 1;
    VERIFICAR(percorrida, t);
  }
  for (const char* t : acrescimos) permitidas.insert(t);
  for (const std::string& t : vistas) {
    VERIFICAR(permitidas.count(t) == 1, ("transição fora do diagrama: " + t).c_str());
  }
  printf("   %zu transições registradas, %zu diferentes\n", gravador.log.size(), vistas.size());
}

int main(int argc, char** argv) {
  sim::mostrarSerial = argc > 1 && strcmp(argv[1], "-v") == 0;
  for (uint8_t i = 0; i < NUM_ESTACOES; i++) cobrirLdr(i, false);
  sim::analogico[Pinos::POTENCIOMETRO] = 1023;

  setup();
  trem.adicionarOuvinte(gravador);

  testeInicializacao();
  testePartidaERampa();
  testeCicloDeEstacao();
  testeLeituraInvalida();
  testeParadaManual();
  testeTimeout();
  testeSensorIncoerente();
  testeTrocaDeTrilho();
  testeParadaNaEstacaoERetomando();
  testePartidaEmCimaDoLdr();
  testeEstrategias();
  testeCoberturaDoDiagrama();

  VERIFICAR(sim::bytesComServoConectado == 0, "nenhum comando ao DFPlayer com o servo conectado, no teste todo");
  printf("\n%d verificações, %d falhas\n", verificacoes, falhas);
  return falhas == 0 ? 0 : 1;
}
