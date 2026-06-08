# skeleton of the work

fazer uma revisao sistematica de metodos de gliding-box e box-counting

passos gerais do teste:
1. colher algumas implementacoes dos dois (min 3)
2. pegar dataset para testar elas
3. rodar 3x cada um e tirar a media das metricass

passos da escrita do artigo:
1. contextualização (introducao e relevancia dos metodos)
2. fundamentacao dos algoritmos encontrados
3. metodologia utililizada para os testes
4. discussão de resultados

## paper struct

1. introduction
    a. history of fractals (origin, simple concept definition, study areas that begin it)
    b. utility of fractals (begin of 00s, then 2010s and now)
    c. algorithm comparasion/analysis relevance (simrank, comparisons)
    d. relevance of the methods that will be studied (BC, GC, DFC, BM)
    e. objectives

2. theorical foundation
    a. fractal analysis
    b. box counting
    c. gliding box
    d. dfc (naive and optimized)
    e. bm (naive and optimized)

3. algorithm project and analysis
    a. pseudocodigo e analise assintotica BC
    b. pseudocodigo e analise assintotica GC
    c. pseudocodigo e analise assintotica DFC
    d. pseudocodigo e analise assintotica BM

4. methodology
    a. materials (dataset)
    b. how tests are done? (runs, enviroment...)
    c. metrics (time, memory, effectiveness - o algoritmo mais efetivo será aquele que gerar a maior diferença estatística entre as duas classes, ex.: positivo tem D = 1.4 e negativo tem D = 1.9)

5. results and discussions
    a. pre-processing (show the time spent by box couting and gliding box on binarization pre-process)
    b. graphics comparing the teorical/real time
    c. tradeoffs (tempo, ram, effectiveness)

6. conclusion
    