/**
 * =============================================================================
 * BOX-COUNTING PARA DIMENSÃO FRACTAL
 * Implementação baseada em: Liebovitch & Toth (1989)
 * "A Fast Algorithm to Determine Fractal Dimensions by Box Counting"
 * Physics Letters A, Vol. 141, No. 8,9
 *
 * Trabalho: Avaliação Assintótica de Métodos de Estimação de Dimensão Fractal:
 *           Box Counting vs. Gliding Box
 * Disciplina: Análise e Projeto de Algoritmos
 * =============================================================================
 *
 * ANÁLISE ASSINTÓTICA (resumo para referência rápida):
 *
 * Seja N = número de pontos, d_e = dimensão de embedding, k = bits de precisão.
 *
 * TEMPO:
 *   - Normalização dos pontos:          O(N * d_e)
 *   - Construção dos hashes Z_n:        O(N * d_e)   [por escala; k escalas no total]
 *   - Ordenação (heapsort/quicksort):   O(N log N)   [dominante]
 *   - Contagem de caixas distintas:     O(N)
 *   - Loop sobre k escalas:             O(k)
 *   -------------------------------------------------------
 *   TOTAL:  T(N, d_e, k) = O(k * N * d_e + k * N log N)
 *                        = O(k * N * (d_e + log N))
 *   Como k e d_e são constantes para um dataset fixo:
 *                        = Θ(N log N)  ← caso médio e pior caso
 *
 * ESPAÇO AUXILIAR (excluindo entrada):
 *   - Array de hashes Z_n:              O(N)
 *   - Array auxiliar de sort:           O(N)         [quicksort in-place: O(log N) pilha]
 *   - Vetores de NB e epsilon:          O(k)
 *   -------------------------------------------------------
 *   TOTAL:  S(N) = O(N)   (≈ 2 * N * d_e bytes conforme artigo)
 *
 * MELHOR CASO:     Ω(N log N)  — sort é gargalo mesmo no melhor caso
 * PIOR CASO:       O(N²)       — se std::sort degradar (improvável com introsort)
 * CASO MÉDIO:      Θ(N log N)
 *
 * CONSTANTES OCULTAS: Cada ponto gera uma string de d_e inteiros concatenados.
 * Para d_e grande, o custo de comparação de strings é O(d_e), tornando o sort
 * efetivamente O(d_e * N log N). Isso é a "constante oculta" discutida no artigo.
 * =============================================================================
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <chrono>
#include <iomanip>
#include <cassert>
#include <random>
#include <sstream>
#include <functional>

// ─────────────────────────────────────────────────────────────────────────────
// ESTRUTURAS DE DADOS
// ─────────────────────────────────────────────────────────────────────────────

struct Point {
    std::vector<double> coords;
    explicit Point(int dim) : coords(dim, 0.0) {}
};

/**
 * Contadores de operações para análise assintótica empírica.
 * Cada campo registra exatamente quantas vezes aquela operação foi executada,
 * permitindo validar experimentalmente as classes O() derivadas analiticamente.
 */
struct OpsCounter {
    long long normalizacoes   = 0;  // atribuições durante normalização
    long long hash_ops        = 0;  // operações AND + concatenação por escala
    long long comparacoes     = 0;  // comparações durante ordenação
    long long contagens       = 0;  // iterações na contagem de caixas distintas
    long long total_escalas   = 0;  // número de escalas processadas
    double    tempo_ms        = 0.0;

    void reset() { *this = OpsCounter{}; }

    void imprimir(const std::string& prefixo = "") const {
        std::cout << prefixo << "┌─ Contagem de Operações Básicas ──────────────────\n";
        std::cout << prefixo << "│  Normalizações:          " << std::setw(12) << normalizacoes << "\n";
        std::cout << prefixo << "│  Ops de hash (por escala):" << std::setw(11) << hash_ops << "\n";
        std::cout << prefixo << "│  Comparações (sort):     " << std::setw(12) << comparacoes << "\n";
        std::cout << prefixo << "│  Iterações de contagem:  " << std::setw(12) << contagens << "\n";
        std::cout << prefixo << "│  Escalas processadas:    " << std::setw(12) << total_escalas << "\n";
        std::cout << prefixo << "│  Tempo total:            " << std::setw(11) << std::fixed
                  << std::setprecision(3) << tempo_ms << " ms\n";
        std::cout << prefixo << "└──────────────────────────────────────────────────\n";
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// GERAÇÃO DE FRACTAIS DE TESTE (Seção 3 do artigo)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Conjunto de Cantor do meio-terço via Sistema de Funções Iteradas (IFS).
 * Dimensão exata: log(2)/log(3) ≈ 0.6309
 * Eq. (6) do artigo: x_{i+1} = x_i/3  ou  x_{i+1} = 2/3 + x_i/3
 */
std::vector<Point> gerar_cantor_set(int N) {
    std::vector<Point> pontos;
    pontos.reserve(N);

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> coin(0, 1);

    double x = 0.5;
    // Burn-in: descartar primeiras iterações para convergir ao atrator
    for (int i = 0; i < 1000; ++i) {
        x = (coin(rng) == 0) ? x / 3.0 : 2.0 / 3.0 + x / 3.0;
    }
    for (int i = 0; i < N; ++i) {
        x = (coin(rng) == 0) ? x / 3.0 : 2.0 / 3.0 + x / 3.0;
        Point p(1);
        p.coords[0] = x;
        pontos.push_back(p);
    }
    return pontos;
}

/**
 * Triângulo de Sierpinski Modificado — IFS com 3 transformações.
 * Dimensão exata: log(3)/log(2) ≈ 1.5849
 * Eq. (7) do artigo: x_{i+1} = y_i/2 + a_k, y_{i+1} = x_i/2 + b_k
 */
std::vector<Point> gerar_sierpinski_modificado(int N) {
    std::vector<Point> pontos;
    pontos.reserve(N);

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dado(0, 2);

    // Constantes do artigo: a1=a2=b1=1, a3=b2=b3=0.125 (≈ 1/8, não 125!)
    // Nota: o artigo lista "125" que é um typo; o valor correto para produzir
    // o fractal com dim≈1.58 é 0.125 (ver Barnsley, Fractals Everywhere, p.93)
    const double a[3] = {1.0, 1.0, 0.125};
    const double b[3] = {1.0, 0.125, 0.125};

    double x = 0.5, y = 0.5;
    for (int i = 0; i < 1000; ++i) {
        int k = dado(rng);
        double nx = y / 2.0 + a[k];
        double ny = x / 2.0 + b[k];
        x = nx; y = ny;
    }
    for (int i = 0; i < N; ++i) {
        int k = dado(rng);
        double nx = y / 2.0 + a[k];
        double ny = x / 2.0 + b[k];
        x = nx; y = ny;
        Point p(2);
        p.coords[0] = x;
        p.coords[1] = y;
        pontos.push_back(p);
    }
    return pontos;
}

/**
 * Atrator de Hénon — sistema dinâmico determinístico.
 * Dimensão estimada ≈ 1.26 (Tabela 1 do artigo)
 * Eq. (5): x_{i+1} = 1 + y_i - 1.4*x_i², y_{i+1} = 0.3*x_i
 */
std::vector<Point> gerar_henon(int N) {
    std::vector<Point> pontos;
    pontos.reserve(N);

    double x = 0.0, y = 0.0;
    for (int i = 0; i < 1000; ++i) {  // burn-in
        double nx = 1.0 + y - 1.4 * x * x;
        double ny = 0.3 * x;
        x = nx; y = ny;
    }
    for (int i = 0; i < N; ++i) {
        double nx = 1.0 + y - 1.4 * x * x;
        double ny = 0.3 * x;
        x = nx; y = ny;
        Point p(2);
        p.coords[0] = x;
        p.coords[1] = y;
        pontos.push_back(p);
    }
    return pontos;
}

// ─────────────────────────────────────────────────────────────────────────────
// ALGORITMO PRINCIPAL: BOX-COUNTING (Liebovitch & Toth, 1989)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Implementa o algoritmo de box-counting descrito nas páginas 386-388 do artigo.
 *
 * PASSOS DO ALGORITMO (mapeados diretamente do artigo):
 *
 * 1. Normalização: mapeia cada coordenada X_i para o intervalo [0, 2^k - 1]
 *    (inteiros de k bits). Custo: O(N * d_e).
 *
 * 2. Para cada escala m = k, k-1, ..., 0:
 *    a. Forma Y_i = (X_i AND M), onde M = máscara com 1s nos primeiros k-m bits.
 *       Pontos na mesma caixa de tamanho 2^m terão Y_i idênticos.
 *    b. Constrói Z_n = Y_1 || Y_2 || ... || Y_{d_e} (concatenação = hash único da caixa).
 *    c. Ordena os Z_n com heapsort/quicksort: O(N log N).
 *    d. Percorre a lista ordenada contando transições (= número de caixas distintas NB).
 *
 * 3. Regressão linear de log(NB(ε)) vs log(1/ε) para estimar d_B (Eq. 4).
 *    Critério de filtragem: ignora m=k e m=k-1 (resolução pobre) e
 *    ignora NB(ε) > N/5 (saturação por dados insuficientes).
 *
 * @param pontos    Conjunto de pontos do fractal
 * @param k         Número de bits de precisão (número de escalas = k+1)
 * @param ops       [saída] Contadores de operações para análise assintótica
 * @return          Dimensão fractal estimada d_B
 */
double box_counting(
    const std::vector<Point>& pontos,
    int k,
    OpsCounter& ops
) {
    ops.reset();
    if (pontos.empty()) return 0.0;

    const int N    = static_cast<int>(pontos.size());
    const int d_e  = static_cast<int>(pontos[0].coords.size());

    // ─── Passo 1: Normalização ─────────────────────────────────────────────
    // "The values of X_i are normalized to cover the range (0, 2^k - 1)"
    // Complexidade: O(N * d_e) — loop duplo sobre pontos e dimensões
    // Espaço: O(N * d_e) inteiros de 2 bytes ≈ 2*N*d_e bytes (conforme artigo)
    const long long MAX_VAL = (1LL << k) - 1;

    // Calcular min/max por dimensão
    std::vector<double> min_d(d_e,  1e18);
    std::vector<double> max_d(d_e, -1e18);
    for (const auto& p : pontos) {
        for (int d = 0; d < d_e; ++d) {
            min_d[d] = std::min(min_d[d], p.coords[d]);
            max_d[d] = std::max(max_d[d], p.coords[d]);
        }
    }

    // Normalizar para inteiros em [0, 2^k - 1]
    // Operação básica contada: cada atribuição de X_norm[i][d]
    std::vector<std::vector<long long>> X_norm(N, std::vector<long long>(d_e));
    for (int i = 0; i < N; ++i) {
        for (int d = 0; d < d_e; ++d) {
            double range = max_d[d] - min_d[d];
            if (range < 1e-15) range = 1.0;
            X_norm[i][d] = static_cast<long long>(
                (pontos[i].coords[d] - min_d[d]) / range * MAX_VAL
            );
            X_norm[i][d] = std::min(X_norm[i][d], MAX_VAL);
            ++ops.normalizacoes;  // ← conta operação básica de normalização
        }
    }

    // ─── Passo 2: Loop sobre escalas ──────────────────────────────────────
    // Escalas: m = k, k-1, ..., 0  →  tamanho da caixa = 2^m
    // Artigo: "The procedure is then repeated for different boxes of edge size 2^m,
    //          where m = k, k-1, ..., 0"
    std::vector<double> log_NB;    // log(NB(ε))
    std::vector<double> log_inv_e; // log(1/ε)

    // Vetor de hashes reutilizado a cada escala — O(N) espaço auxiliar
    std::vector<std::string> Z(N);

    for (int m = k; m >= 0; --m) {
        ++ops.total_escalas;

        // Máscara M: k-m bits mais significativos em 1, resto em 0
        // "M is a binary number with 1's in the first k-m places and 0's the remainder"
        long long M = 0;
        if (k - m > 0) {
            M = MAX_VAL & ~((1LL << m) - 1);  // zera os m bits menos significativos
        }

        // ─── Passo 2a-2b: Formar Y_i e construir Z_n ──────────────────────
        // "Y_i = (X_i AND M)" — operação AND bit a bit
        // "Z_n = Y_1 + Y_2 + ... + Y_{d_e}" — concatenação de strings
        // Operação básica: cada AND + append → O(d_e) por ponto, O(N*d_e) total
        for (int i = 0; i < N; ++i) {
            std::ostringstream oss;
            for (int d = 0; d < d_e; ++d) {
                long long Y = X_norm[i][d] & M;
                oss << Y;
                if (d < d_e - 1) oss << '|';  // separador para evitar colisões
                ++ops.hash_ops;  // ← conta AND + escrita
            }
            Z[i] = oss.str();
        }

        // ─── Passo 2c: Ordenar os Z_n ─────────────────────────────────────
        // "We count the number of distinct Z_n efficiently by ordering them
        //  using a quicksort or heapsort"
        // Complexidade: O(N log N) comparações de strings
        // Esta é a operação DOMINANTE do algoritmo.
        //
        // Para instrumentar comparações dentro do sort, usamos um comparador
        // com contador. Nota: isso adiciona overhead constante por comparação
        // mas não altera a classe assintótica.
        long long cmp_local = 0;
        std::sort(Z.begin(), Z.end(),
            [&cmp_local](const std::string& a, const std::string& b) {
                ++cmp_local;
                return a < b;
            }
        );
        ops.comparacoes += cmp_local;

        // ─── Passo 2d: Contar caixas distintas ────────────────────────────
        // "walk down the list once to count the number of times the values change"
        // Complexidade: O(N) — percurso linear
        long long NB = 1;  // pelo menos 1 caixa
        for (int i = 1; i < N; ++i) {
            if (Z[i] != Z[i - 1]) ++NB;
            ++ops.contagens;  // ← conta iteração de contagem
        }

        // ─── Passo 3: Filtragem (critérios do artigo, pg. 387) ─────────────
        // Ignora: (a) m=k e m=k-1 (resolução grosseira)
        // Ignora: (b) NB(ε) > N/5 (saturação por pontos insuficientes)
        if (m >= k - 1) continue;           // (a) escalas grosseiras
        if (NB > static_cast<long long>(N) / 5) continue;  // (b) saturação

        // ε = 2^m / 2^k = 2^(m-k)  →  log(1/ε) = (k-m) * log(2)
        double eps      = std::pow(2.0, m - k);
        double log_e    = std::log(NB);
        double log_ie   = std::log(1.0 / eps);

        log_NB.push_back(log_e);
        log_inv_e.push_back(log_ie);
    }

    // ─── Passo 4: Regressão linear (mínimos quadrados) ────────────────────
    // "We then use a least squares fit to determine d_B as the slope of
    //  log NB(ε) versus log(1/ε)"  — Eq. (4)
    // Complexidade: O(P) onde P = número de pontos válidos ≤ k
    if (log_NB.size() < 2) {
        std::cerr << "[AVISO] Pontos insuficientes para regressão. "
                  << "Aumente N ou k.\n";
        return -1.0;
    }

    const int P = static_cast<int>(log_NB.size());
    double sum_x  = 0, sum_y = 0, sum_xx = 0, sum_xy = 0;
    for (int i = 0; i < P; ++i) {
        sum_x  += log_inv_e[i];
        sum_y  += log_NB[i];
        sum_xx += log_inv_e[i] * log_inv_e[i];
        sum_xy += log_inv_e[i] * log_NB[i];
    }
    // Coeficiente angular = d_B (inclinação da reta de regressão)
    double denom = P * sum_xx - sum_x * sum_x;
    if (std::abs(denom) < 1e-15) return -1.0;
    double d_B = (P * sum_xy - sum_x * sum_y) / denom;

    return d_B;
}

// ─────────────────────────────────────────────────────────────────────────────
// FUNÇÃO AUXILIAR: RECONSTRUÇÃO DO ESPAÇO DE EMBEDDING
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Reconstrói trajetória no espaço de embedding a partir de série temporal.
 * Usado para análise de sistemas dinâmicos (Hénon).
 * "the time series of only one of the variables is known. The attractor can
 *  then be reconstructed, in an embedding space of dimension d_e"
 * Complexidade: O(N * d_e)
 */
std::vector<Point> reconstruir_embedding(
    const std::vector<double>& serie,
    int d_e,
    int lag = 1
) {
    const int N_pts = static_cast<int>(serie.size()) - (d_e - 1) * lag;
    std::vector<Point> pontos;
    pontos.reserve(N_pts);
    for (int i = 0; i < N_pts; ++i) {
        Point p(d_e);
        for (int d = 0; d < d_e; ++d) {
            p.coords[d] = serie[i + d * lag];
        }
        pontos.push_back(p);
    }
    return pontos;
}

// ─────────────────────────────────────────────────────────────────────────────
// BENCHMARK: ANÁLISE EMPÍRICA DA COMPLEXIDADE
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Executa o algoritmo para diferentes valores de N e mede:
 *   - Tempo de execução (ms)
 *   - Número de comparações do sort
 *   - Número total de operações básicas
 *
 * Objetivo: validar empiricamente que T(N) ≈ c * N * log(N)
 * confirmando a classe assintótica Θ(N log N) derivada analiticamente.
 */
void benchmark_complexidade(int k = 10, int d_e = 2) {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║    BENCHMARK DE COMPLEXIDADE ASSINTÓTICA — BOX COUNTING      ║\n";
    std::cout << "║    k=" << k << " bits, d_e=" << d_e << " dimensões                              ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";

    std::cout << std::left
              << std::setw(8)  << "N"
              << std::setw(12) << "Tempo(ms)"
              << std::setw(16) << "Comparações"
              << std::setw(16) << "N*log2(N)"
              << std::setw(12) << "Razão"
              << std::setw(10) << "d_B"
              << "\n";
    std::cout << std::string(74, '-') << "\n";

    std::vector<int> tamanhos = {500, 1000, 2000, 5000, 10000, 20000, 50000};

    std::ofstream log_bc("bc_n_log.csv");
    log_bc << "N,Tempo_ms,Comparacoes,N_logN,Razao\n";

    for (int N : tamanhos) {
        auto pontos = gerar_henon(N);

        OpsCounter ops;
        auto t0 = std::chrono::high_resolution_clock::now();
        double dB = box_counting(pontos, k, ops);
        auto t1 = std::chrono::high_resolution_clock::now();

        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        double N_logN = N * std::log2(static_cast<double>(N));
        double razao  = ops.comparacoes / N_logN;

        // Salva no CSV
        log_bc << N << "," << ms << "," << ops.comparacoes << "," << N_logN << "," << razao << "\n";

        // Imprime no console
        std::cout << std::left << std::setw(8) << N << std::setw(12) << std::fixed 
                  << std::setprecision(3) << ms << std::setw(16) << ops.comparacoes
                  << std::setw(16) << std::setprecision(0) << N_logN << std::setw(12) 
                  << std::setprecision(3) << razao << std::setw(10) << std::setprecision(3) << dB << "\n";
    }
    log_bc.close();

    std::cout << "\n  INTERPRETAÇÃO:\n";
    std::cout << "  • Se a coluna 'Razão' (Comparações / N·log₂N) converge para\n";
    std::cout << "    uma constante, confirma-se Θ(N log N) empiricamente.\n";
    std::cout << "  • A constante oculta ≈ k * d_e (escalas × dimensão),\n";
    std::cout << "    pois o sort é executado k vezes com comparações de\n";
    std::cout << "    strings de comprimento proporcional a d_e.\n\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// TESTES DE VALIDAÇÃO (replicando Tabela 1 do artigo)
// ─────────────────────────────────────────────────────────────────────────────

struct ResultadoTeste {
    std::string nome;
    double dim_exata;
    double dim_N1000;
    double dim_N10000;
    double tempo_N1000_ms;
    double tempo_N10000_ms;
};

ResultadoTeste testar_fractal(
    const std::string& nome,
    double dim_exata,
    std::function<std::vector<Point>(int)> gerador,
    int k
) {
    ResultadoTeste res;
    res.nome = nome;
    res.dim_exata = dim_exata;

    for (int N : {1000, 10000}) {
        auto pontos = gerador(N);
        OpsCounter ops;

        auto t0 = std::chrono::high_resolution_clock::now();
        double dB = box_counting(pontos, k, ops);
        auto t1 = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

        if (N == 1000)  { res.dim_N1000  = dB; res.tempo_N1000_ms  = ms; }
        if (N == 10000) { res.dim_N10000 = dB; res.tempo_N10000_ms = ms; }
    }
    return res;
}

void validacao_tabela1() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║   VALIDAÇÃO — REPLICAÇÃO DA TABELA 1 (Liebovitch & Toth, 1989)  ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════╝\n\n";

    // Cantor: dimensão d_e=1, embedding 1D
    auto r_cantor = testar_fractal(
        "Conjunto de Cantor (1/3)", std::log(2.0)/std::log(3.0),
        [](int N) { return gerar_cantor_set(N); }, 12
    );

    // Hénon: pontos 2D diretamente
    auto r_henon = testar_fractal(
        "Atrator de Hénon", 1.26,
        [](int N) { return gerar_henon(N); }, 10
    );

    // Sierpinski: pontos 2D
    auto r_sierpinski = testar_fractal(
        "Sierpinski Modificado", std::log(3.0)/std::log(2.0),
        [](int N) { return gerar_sierpinski_modificado(N); }, 10
    );

    std::vector<ResultadoTeste> resultados = {r_henon, r_sierpinski, r_cantor};

    std::cout << std::left
              << std::setw(30) << "Fractal"
              << std::setw(10) << "Dim.Exata"
              << std::setw(12) << "dB(N=1k)"
              << std::setw(14) << "Tempo(N=1k)"
              << std::setw(12) << "dB(N=10k)"
              << std::setw(14) << "Tempo(N=10k)"
              << "\n";
    std::cout << std::string(92, '-') << "\n";

    for (const auto& r : resultados) {
        std::cout << std::left
                  << std::setw(30) << r.nome
                  << std::setw(10)  << std::fixed << std::setprecision(3) << r.dim_exata
                  << std::setw(12)  << r.dim_N1000
                  << std::setw(14)  << std::setprecision(1) << r.tempo_N1000_ms << " ms"
                  << std::setw(12)  << std::setprecision(3) << r.dim_N10000
                  << std::setw(14)  << r.tempo_N10000_ms << " ms"
                  << "\n";
    }

    std::cout << "\n  COMPARAÇÃO COM TABELA 1 DO ARTIGO:\n";
    std::cout << "  • Hénon:      artigo reporta 1.27±0.02 (N=10000)\n";
    std::cout << "  • Sierpinski: artigo reporta 1.63±0.04 (N=10000)\n";
    std::cout << "  • Cantor:     artigo reporta 0.639±0.007 (N=10000)\n\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// ANÁLISE DE MEMÓRIA (Seção de Espaço Auxiliar)
// ─────────────────────────────────────────────────────────────────────────────

void analisar_memoria(int N_max = 50000, int d_e = 2) {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║              ANÁLISE DE COMPLEXIDADE DE ESPAÇO              ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";

    std::cout << "  Estruturas de dados alocadas pelo algoritmo:\n\n";
    std::cout << "  ┌─────────────────────────────┬──────────────┬─────────────────────┐\n";
    std::cout << "  │ Estrutura                   │ Tamanho      │ Classe Assintótica  │\n";
    std::cout << "  ├─────────────────────────────┼──────────────┼─────────────────────┤\n";

    std::cout << "  │ X_norm[N][d_e]  (int64)     │ "
              << std::setw(7) << (N_max * d_e * 8 / 1024) << " KB      │ O(N · d_e)          │\n";
    std::cout << "  │ Z[N]  (strings)             │ "
              << std::setw(7) << (N_max * (d_e * 8 + 4) / 1024) << " KB      │ O(N · d_e)          │\n";
    std::cout << "  │ log_NB, log_inv_e  (double) │ "
              << std::setw(7) << 1 << " KB      │ O(k)                │\n";
    std::cout << "  │ Pilha de recursão (sort)    │ "
              << std::setw(7) << 1 << " KB      │ O(log N)  esperado  │\n";
    std::cout << "  ├─────────────────────────────┼──────────────┼─────────────────────┤\n";
    std::cout << "  │ TOTAL AUXILIAR              │ "
              << std::setw(7) << (N_max * d_e * 2 * 8 / 1024) << " KB      │ O(N · d_e) = O(N)   │\n";
    std::cout << "  └─────────────────────────────┴──────────────┴─────────────────────┘\n\n";

    std::cout << "  Nota do artigo (pg. 387): \"the memory required is approximately\n";
    std::cout << "  2*N*d_e bytes\" — confirmado acima para inteiros de 2 bytes.\n";
    std::cout << "  Com int64 moderno, o fator é 8, mas a classe O(N) se mantém.\n\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// ANÁLISE DE CASOS (MELHOR / MÉDIO / PIOR)
// ─────────────────────────────────────────────────────────────────────────────

void analisar_casos() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║         ANÁLISE DE CASOS — BOX COUNTING                     ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";

    std::cout << "  ┌──────────────┬─────────────────────────────────────────────────┐\n";
    std::cout << "  │ Caso         │ Descrição e Complexidade                        │\n";
    std::cout << "  ├──────────────┼─────────────────────────────────────────────────┤\n";
    std::cout << "  │ MELHOR CASO  │ Todos os N pontos já estão ordenados.           │\n";
    std::cout << "  │ Ω(N log N)   │ std::sort (introsort) ainda executa O(N log N). │\n";
    std::cout << "  │              │ O gargalo não é eliminável — é Ω(N log N).      │\n";
    std::cout << "  ├──────────────┼─────────────────────────────────────────────────┤\n";
    std::cout << "  │ CASO MÉDIO   │ Distribuição aleatória dos pontos.              │\n";
    std::cout << "  │ Θ(N log N)   │ Quicksort em média: Θ(N log N).                │\n";
    std::cout << "  │              │ Constante oculta: c ≈ k * d_e.                 │\n";
    std::cout << "  ├──────────────┼─────────────────────────────────────────────────┤\n";
    std::cout << "  │ PIOR CASO    │ Entrada patológica para quicksort puro (ex:     │\n";
    std::cout << "  │ O(N log N)   │ todos Z_n idênticos). std::sort usa introsort,  │\n";
    std::cout << "  │              │ garantindo O(N log N) mesmo no pior caso.       │\n";
    std::cout << "  │              │ DIFERENTE do quicksort puro: O(N²) no pior.     │\n";
    std::cout << "  └──────────────┴─────────────────────────────────────────────────┘\n\n";

    // Demonstração empírica do caso extremo: todos pontos no mesmo hash
    std::cout << "  EXPERIMENTO — Caso de saturação total (NB=1 em todas as escalas):\n";
    std::cout << "  (todos os pontos colocados na origem — caso degenerado)\n\n";

    std::vector<Point> pontos_degenerate(10000, Point(2));
    OpsCounter ops;
    auto t0 = std::chrono::high_resolution_clock::now();
    double dB = box_counting(pontos_degenerate, 10, ops);
    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    std::cout << "  N=10000, todos pontos em (0,0): d_B=" << dB
              << "  Tempo=" << std::fixed << std::setprecision(2) << ms << " ms\n";
    ops.imprimir("  ");
}

// ─────────────────────────────────────────────────────────────────────────────
// FUNÇÃO PRINCIPAL
// ─────────────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    std::cout << "=================================================================\n";
    std::cout << "  BOX-COUNTING — Liebovitch & Toth (1989)\n";
    std::cout << "  Análise Assintótica para Trabalho de APA\n";
    std::cout << "  Compilado com C++17\n";
    std::cout << "=================================================================\n";

    // ── MODO 1: Validação (replica Tabela 1 do artigo) ──────────────────────
    validacao_tabela1();

    // ── MODO 2: Benchmark de complexidade empírica ──────────────────────────
    benchmark_complexidade(10, 2);

    // ── MODO 3: Análise de memória ───────────────────────────────────────────
    analisar_memoria(50000, 2);

    // ── MODO 4: Análise de casos ─────────────────────────────────────────────
    analisar_casos();

    // ── MODO 5: Exemplo detalhado com contagem de ops ───────────────────────
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║         EXEMPLO DETALHADO — HÉNON N=5000, k=10, d_e=2       ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";

    auto pontos = gerar_henon(5000);
    OpsCounter ops;
    auto t0 = std::chrono::high_resolution_clock::now();
    double dB = box_counting(pontos, 10, ops);
    auto t1 = std::chrono::high_resolution_clock::now();
    ops.tempo_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    std::cout << "  Dimensão fractal estimada: d_B = " << std::fixed
              << std::setprecision(4) << dB << "\n";
    std::cout << "  (Valor de referência do artigo: ~1.26 para o atrator de Hénon)\n\n";
    ops.imprimir("  ");

    double N_logN = 5000.0 * std::log2(5000.0);
    std::cout << "  Verificação assintótica:\n";
    std::cout << "    N·log₂(N) = " << std::setprecision(0) << N_logN << "\n";
    std::cout << "    Comparações = " << ops.comparacoes << "\n";
    std::cout << "    Razão = " << std::setprecision(3)
              << ops.comparacoes / N_logN << "  (≈ k·d_e = 10·2 = 20 esperado)\n\n";

    std::cout << "=================================================================\n";
    std::cout << "  SUMÁRIO DAS CLASSES ASSINTÓTICAS\n";
    std::cout << "=================================================================\n";
    std::cout << "  Tempo  — Melhor caso:  Ω(N log N)\n";
    std::cout << "         — Caso médio:   Θ(N log N)\n";
    std::cout << "         — Pior caso:    O(N log N)   [com introsort]\n";
    std::cout << "  Espaço — Auxiliar:     O(N · d_e) = O(N)  para d_e constante\n";
    std::cout << "  Constante oculta ≈ k · d_e  (número de escalas × dimensão)\n";
    std::cout << "=================================================================\n\n";

    return 0;
}
