// ============================================================================
// Estados.h — os 7 estados do Diagrama de Máquina de Estados (Semana 4).
//
// Padrão State:     cada estado é uma classe; o Trem (contexto) delega a ele.
// Padrão Singleton: cada estado tem uma única instância (instancia()), sem "new".
// ============================================================================
#ifndef ESTADOS_H
#define ESTADOS_H

#include "Estado.h"

// Base dos estados em que o trem está em operação (EM MOVIMENTO, DETECTANDO,
// PARANDO, NA ESTAÇÃO e RETOMANDO). Nesses estados o botão de parada leva a
// PARADO, como exigem o RNF02 e a HU07. No diagrama, só EM MOVIMENTO tem essa
// transição desenhada; ver "Ajustes sugeridos no diagrama" no README.md.
class EstadoEmOperacao : public Estado {
public:
  void aoComando(Trem& trem, Comando comando) override;

protected:
  EstadoEmOperacao() = default;
  ~EstadoEmOperacao() = default;
};

// PARADO — estado inicial.
class EstadoParado final : public Estado {
public:
  static EstadoParado& instancia();
  const __FlashStringHelper* nome() const override;
  void entrar(Trem& trem) override;
  void aoComando(Trem& trem, Comando comando) override;

private:
  EstadoParado() = default;
  EstadoParado(const EstadoParado&) = delete;
  EstadoParado& operator=(const EstadoParado&) = delete;
  static EstadoParado unica_;
};

// EM MOVIMENTO — motor em PWM (potenciômetro), monitora os 4 LDRs, farol aceso.
class EstadoEmMovimento final : public EstadoEmOperacao {
public:
  static EstadoEmMovimento& instancia();
  const __FlashStringHelper* nome() const override;
  void atualizar(Trem& trem) override;

private:
  EstadoEmMovimento() = default;
  EstadoEmMovimento(const EstadoEmMovimento&) = delete;
  EstadoEmMovimento& operator=(const EstadoEmMovimento&) = delete;
  static EstadoEmMovimento unica_;
};

// DETECTANDO — confirma se o pulso do LDR é mesmo uma estação.
class EstadoDetectando final : public EstadoEmOperacao {
public:
  static EstadoDetectando& instancia();
  const __FlashStringHelper* nome() const override;
  void atualizar(Trem& trem) override;

private:
  EstadoDetectando() = default;
  EstadoDetectando(const EstadoDetectando&) = delete;
  EstadoDetectando& operator=(const EstadoDetectando&) = delete;
  static EstadoDetectando unica_;
};

// PARANDO — reduz a velocidade até zero.
class EstadoParando final : public EstadoEmOperacao {
public:
  static EstadoParando& instancia();
  const __FlashStringHelper* nome() const override;
  void entrar(Trem& trem) override;
  void atualizar(Trem& trem) override;

private:
  EstadoParando() = default;
  EstadoParando(const EstadoParando&) = delete;
  EstadoParando& operator=(const EstadoParando&) = delete;
  static EstadoParando unica_;
};

// NA ESTAÇÃO — anúncio em áudio, LED da estação aceso, atualiza a próxima
// estação e posiciona o servo para a rota.
class EstadoNaEstacao final : public EstadoEmOperacao {
public:
  static EstadoNaEstacao& instancia();
  const __FlashStringHelper* nome() const override;
  void entrar(Trem& trem) override;
  void atualizar(Trem& trem) override;
  void sair(Trem& trem) override;

private:
  EstadoNaEstacao() = default;
  EstadoNaEstacao(const EstadoNaEstacao&) = delete;
  EstadoNaEstacao& operator=(const EstadoNaEstacao&) = delete;
  static EstadoNaEstacao unica_;
  bool audioEnviado_ = false;
};

// RETOMANDO — volta a andar até o trem sair de cima do LDR da estação.
class EstadoRetomando final : public EstadoEmOperacao {
public:
  static EstadoRetomando& instancia();
  const __FlashStringHelper* nome() const override;
  void entrar(Trem& trem) override;
  void atualizar(Trem& trem) override;

private:
  EstadoRetomando() = default;
  EstadoRetomando(const EstadoRetomando&) = delete;
  EstadoRetomando& operator=(const EstadoRetomando&) = delete;
  static EstadoRetomando unica_;
  bool sensorLivre_ = false;
  unsigned long livreDesde_ = 0;
};

// FALHA — motor desligado, LED de aviso piscando, aguarda o reset.
class EstadoFalha final : public Estado {
public:
  static EstadoFalha& instancia();
  const __FlashStringHelper* nome() const override;
  void entrar(Trem& trem) override;
  void aoComando(Trem& trem, Comando comando) override;
  void sair(Trem& trem) override;

private:
  EstadoFalha() = default;
  EstadoFalha(const EstadoFalha&) = delete;
  EstadoFalha& operator=(const EstadoFalha&) = delete;
  static EstadoFalha unica_;
};

#endif
