#!/bin/bash

# Limpa o terminal
clear

echo "=========================================="
echo " Compilando os algoritmos (C++17, -O3)..."
echo "=========================================="

# Compila o Box Counting
# -std=c++17 : Usa o padrão moderno do C++
# -O3        : Otimização máxima de performance (MUITO importante para benchmarks)
# -Wall      : Mostra todos os avisos (warnings) do código
g++ box_counting.cpp -std=c++17 -O3 -Wall -o box_counting_exec

# Compila o Gliding Box
g++ gliding_box.cpp -std=c++17 -O3 -Wall -o gliding_box_exec

echo "Compilação finalizada com sucesso!"
echo ""

echo "=========================================="
echo " Executando Box Counting..."
echo "=========================================="
./box_counting_exec

echo ""
echo "=========================================="
echo " Executando Gliding Box..."
echo "=========================================="
./gliding_box_exec

echo ""
echo "Testes concluídos!"

echo ""
echo "=========================================="
echo " Plotando gráficos..."
echo "=========================================="
python3 plotar_resultados.py

echo ""
echo "Gráficos gerados com sucesso!"