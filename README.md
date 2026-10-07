# Firmware do Infinity Train

Código C++ do Arduino Uno R3 do Sistema de Trem Interativo. Ele implementa o **Diagrama de Máquina de Estados** da Semana 4 e usa os pinos do documento **Mapeamento de Componentes**.

## Como usar

1. Abra `trem_interativo/trem_interativo.ino` na Arduino IDE. A pasta `trem_interativo` inteira é o projeto.
2. Escolha a placa **Arduino Uno**. Não é preciso instalar nenhuma biblioteca: Servo e SoftwareSerial já vêm com a IDE.
3. No cartão microSD do DFPlayer, crie uma pasta `MP3` com estes arquivos:
   - `0001.mp3` a `0004.mp3`: som de chegada + nome das estações 1 a 4;
   - `0005.mp3`: som de partida.
4. Abra o Monitor Serial em **115200**. Cada transição aparece com o mesmo texto da seta do diagrama:
   ```
   [3561 ms] EM MOVIMENTO -> DETECTANDO (pulso detectado no LDR)
   ```
5. Ajuste os valores de `Config.h` durante os testes: limiares dos LDRs, PWM mínimo, ângulos do servo e tempos.

**Calibração dos LDRs (PB06).** Em `Config.h`, coloque `MODO_CALIBRACAO_LDR = true` e envie o código. O trem fica parado, e o Monitor Serial mostra a leitura de cada LDR. Anote o valor com e sem o trem em cima. Depois, use um valor intermediário em `Ldr::LIMIAR` para cada estação e volte o modo para `false`.

## Arquivos

| Arquivo | O que faz | Pacote da EAP |
| --- | --- | --- |
| `trem_interativo.ino` | Cria os objetos, `setup()` e `loop()` | 1.5.8 |
| `Config.h` | Pinos, tempos e valores de calibração | 1.5.1 |
| `Estado.h` | Interface comum dos estados | 1.5.8 |
| `Estados.h` / `.cpp` | Os 7 estados do diagrama, com ações e transições | 1.5.2, 1.5.4, 1.5.8 |
| `Trem.h` / `.cpp` | Contexto da máquina de estados | 1.5.8 |
| `Hardware.h` / `.cpp` | Motor com rampa, potenciômetro, LDRs, LEDs, DFPlayer e servo | 1.5.3, 1.5.5, 1.5.6, 1.5.7 |
| `PainelControle.h` / `.cpp` | Botões de partida, parada e troca de trilho | 1.5.2, 1.5.7 |
| `RegistroSerial.h` / `.cpp` | Mostra comandos e transições no Monitor Serial | 1.6 |
| `Interfaces.h` | Interfaces usadas pelos padrões de projeto | — |

## Ligação com o diagrama de estados

Cada estado do diagrama é uma classe em `Estados.h`, e cada seta é uma chamada `trem.mudarPara(...)` em `Estados.cpp`.

| Estado | Classe | Ações do diagrama no código |
| --- | --- | --- |
| PARADO | `EstadoParado` | Estado inicial; motor desligado |
| EM MOVIMENTO | `EstadoEmMovimento` | Motor em PWM pelo potenciômetro; monitora os 4 LDRs; o farol acende porque está em paralelo com o motor |
| DETECTANDO | `EstadoDetectando` | Confirma se o LDR continua coberto |
| PARANDO | `EstadoParando` | Freia o motor até zero |
| NA ESTAÇÃO | `EstadoNaEstacao` | LED da estação aceso; atualiza a próxima estação; posiciona o servo para a rota; anúncio em áudio |
| RETOMANDO | `EstadoRetomando` | Som de partida; volta a andar |
| FALHA | `EstadoFalha` | Motor desligado; LED de aviso piscando; aguarda reset |

| Seta do diagrama | Condição no código |
| --- | --- |
| PARADO → EM MOVIMENTO (botão de partida) | Botão do D2 apertado |
| EM MOVIMENTO → DETECTANDO (pulso detectado no LDR) | Algum LDR coberto |
| EM MOVIMENTO → FALHA (timeout sem estação) | 20 s sem estação desde a partida ou desde a última estação |
| EM MOVIMENTO → PARADO (botão de parada) | Botão do D3 apertado |
| DETECTANDO → EM MOVIMENTO (leitura inválida) | O LDR ficou coberto menos de 40 ms (ruído) |
| DETECTANDO → FALHA (sensor incoerente) | Dois LDRs cobertos ao mesmo tempo, ou outro LDR no lugar do detectado |
| DETECTANDO → PARANDO (detecção confirmada) | O LDR ficou coberto por 40 ms |
| PARANDO → NA ESTAÇÃO (velocidade = 0) | PWM do motor chegou a 0 |
| NA ESTAÇÃO → RETOMANDO (tempo de parada concluído) | Passaram 6 s |
| RETOMANDO → EM MOVIMENTO (sensor liberado) | O LDR da estação ficou descoberto por 300 ms seguidos (uma oscilação na saída não conta) |
| FALHA → PARADO (reset) | Botão do D3 apertado |

## Padrões de projeto aplicados

| Padrão | Onde | Por que foi usado |
| --- | --- | --- |
| **State** | `Estado.h`, `Estados.h/.cpp`, `Trem.h/.cpp` | Cada estado do diagrama vira uma classe com as próprias ações e transições. O `Trem` só repassa o trabalho ao estado atual. Assim, o código tem a mesma forma do diagrama e não depende de um `switch` gigante. Um estado novo vira uma classe nova. |
| **Singleton** | `instancia()` de cada estado | Cada estado existe uma única vez e é criado sem `new`. Isso economiza a RAM do Uno, que tem só 2 KB, e evita criar e destruir objetos a cada transição. |
| **Observer** | `PainelControle` → `OuvinteComandos`; `Trem` → `OuvinteTransicoes` | O painel avisa quem estiver cadastrado quando um botão é apertado, sem conhecer o trem. O trem avisa cada transição ao `RegistroSerial`. Assim dá para acrescentar registros ou testes sem mexer no painel nem no trem. |
| **Strategy** | `EstrategiaLeituraLDR` → `LeituraAnalogicaLDR` ou `LeituraDigitalLDR` | O módulo HW-072 pode ter saída analógica (AO) ou só digital (DO), um ponto ainda em aberto no Mapeamento de Componentes. A forma de ler muda em uma linha do `Config.h`, sem mexer nos estados. |
| **Adapter** | `AudioDFPlayer` (interface `IAudio`) e `DesvioServo` (interface `IDesvio`) | O `AudioDFPlayer` traduz "tocar a faixa N" para os bytes do protocolo serial do DFPlayer. O `DesvioServo` traduz "rota principal ou desvio" para o ângulo do servo. Os estados não dependem do hardware: trocar o módulo de áudio ou o servo só exige uma classe nova. |

## Decisões e ajustes sugeridos no diagrama

1. **Botão de parada fora de EM MOVIMENTO.** O RNF02 e a HU07 exigem que o botão pare o trem sempre. Por isso ele também funciona em DETECTANDO, PARANDO, NA ESTAÇÃO e RETOMANDO (classe `EstadoEmOperacao`). **Sugestão:** no diagrama, agrupar esses 5 estados em um superestado "Em operação", com uma única seta "botão de parada" para PARADO.
2. **Reset.** No estado FALHA, o reset é o botão de parada. O botão de reset da placa também funciona, porque reinicia tudo. Vale anotar isso no diagrama.
3. **Troca de trilho.** O botão do D4 só escolhe a rota. O servo se move ao entrar em NA ESTAÇÃO, com o trem parado, como está no diagrama e como pede a mitigação do risco R09.
4. **Servo e áudio.** O servo recebe pulsos só por 400 ms, o tempo de se mover; depois ele é solto (`detach`). O comando de áudio sai 450 ms depois da chegada, com o servo já solto. O motivo é que a SoftwareSerial do DFPlayer pausa as interrupções e faria o servo tremer. A meta do KPI de no máximo 1 s continua atendida.
5. **Partida em cima de um LDR.** Se o trem partir parado sobre o LDR de uma estação, esse LDR é ignorado até o trem sair de cima dele. Assim o trem não para de novo na mesma estação. Isso não muda nenhuma seta do diagrama.
6. **Sugestão:** acrescentar RETOMANDO → FALHA por timeout, para o caso de o trem travar e não sair da estação. Hoje, nesse caso, o operador para o trem pelo botão.
7. **DFPlayer sem biblioteca.** O protocolo serial do DFPlayer está implementado na classe `AudioDFPlayer`, então a biblioteca DFRobotDFPlayerMini não é necessária.

## Cuidados na montagem

- **Resistor de 10 kΩ entre o D5 e o GND.** Enquanto o Arduino liga, o D5 fica solto por cerca de 1 s, e o motor poderia dar um tranco (RF01).
- **Áudios de chegada com até 5 s.** O som de partida toca 6 s depois da chegada e cortaria um anúncio mais longo. Se precisar de áudios maiores, aumente `TEMPO_PARADA_MS`.
- **`PWM_MINIMO` calibrado com o trem parado.** O valor precisa ser o menor que ainda tira o trem do lugar. Se for baixo demais, o motor fica forçando sem andar.
- **Agulha com batentes.** Com o servo solto, a agulha fica no lugar pelo atrito das engrenagens. Os batentes do risco R09 garantem a posição.
- **Fio TX do DFPlayer (para o D10) é opcional.** O código não usa o que o DFPlayer responde. Sem esse fio, não há nenhum risco de interferência no servo.
- **LED de aviso piscando ao ligar.** É normal o LED do D13 piscar rápido ao ligar: quem faz isso é o carregador da placa (bootloader).

## Testes no computador (opcional)

A pasta `testes_pc` compila o mesmo firmware junto com um simulador do Arduino e confere 108 itens. Entre eles estão as 11 setas do diagrama, a rampa do motor, o atraso do áudio, os quadros enviados ao DFPlayer e se o servo está solto quando o DFPlayer recebe comandos. Os arquivos dessa pasta **não vão para o Arduino**.

```bash
./testes_pc/rodar_testes.sh
```

O comando precisa de um compilador C++; no macOS, ele vem nas Command Line Tools do Xcode. Com `-v`, ele também mostra o Monitor Serial simulado.
