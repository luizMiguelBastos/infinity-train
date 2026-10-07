#!/bin/sh
# Compila o firmware com o simulador do Arduino e roda os testes no computador.
# Requer um compilador C++ (clang++ no macOS ou g++ no Linux/Windows com MinGW).
cd "$(dirname "$0")" || exit 1
CXX=${CXX:-c++}
$CXX -std=c++11 -Wall -Wextra -Werror -Isimulador -o testes \
  testes.cpp simulador/simulador.cpp \
  ../trem_interativo/Estados.cpp ../trem_interativo/Hardware.cpp ../trem_interativo/PainelControle.cpp \
  ../trem_interativo/RegistroSerial.cpp ../trem_interativo/Trem.cpp || exit 1
./testes "$@"
