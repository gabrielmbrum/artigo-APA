/**
 * =============================================================================
 * GLIDING BOX — CÁLCULO DE LACUNARIDADE
 * Implementação baseada em: Kovács & Erdélyi (2024)
 * "Methods for calculating gliding-box lacunarity efficiently on large datasets"
 * Pattern Analysis and Applications, 27:113
 * https://doi.org/10.1007/s10044-024-01332-6
 *
 * Trabalho: Avaliação Assintótica de Métodos de Estimação de Dimensão Fractal:
 *           Box Counting vs. Gliding Box
 * Disciplina: Análise e Projeto de Algoritmos
 * =============================================================================
 *
 * ANÁLISE ASSINTÓTICA CONSOLIDADA (Tabela 3 do artigo, pg. 9):
 *
 * Seja N = lado da imagem quadrada (N×N pixels), ε = tamanho da caixa,
 *      C = número de pixels objeto (C ≤ N²)
 *
 * ┌─────────────────┬──────────────────────────────────┬──────────────────┐
 * │ Método          │ Complexidade de Tempo             │ Complexidade     │
 * │                 │                                  │ de Espaço        │
 * ├─────────────────┼──────────────────────────────────┼──────────────────┤
 * │ Original        │ O((N-ε+1)² · ε²) ≈ O(N² · ε²)  │ O(N²)            │
 * │ Sides           │ O((N-ε+1)² · 2ε) ≈ O(N² · ε)   │ O(N²)            │
 * │ Integral        │ O((N-ε+1)² · 4)  ≈ O(N²)        │ O(N²)            │
 * │ Trace           │ O(C · ε²)                        │ O(N²)            │
 * │ Convolution     │ O(N² · log₂N) independente de ε  │ O(N² · log₂N)   │
 * └─────────────────┴──────────────────────────────────┴──────────────────┘
 *
 * NOTA SOBRE CASOS:
 *   Original/Sides:   pior caso = ε = N/2 (máximo de operações)
 *   Integral:         independe de ε → Θ(N²) sempre
 *   Trace (melhor):   C → 0 (imagem quase vazia) → O(1)
 *   Trace (pior):     C = N² (imagem cheia)       → O(N² · ε²) = Original
 *   Convolution:      Θ(N² log N) independente de ε e C
 *
 * COMPARAÇÃO COM BOX COUNTING (Liebovitch & Toth, 1989):
 *   Box Counting:     Θ(N_pts · log N_pts) em pontos, não pixels
 *   Gliding Box:      O(N² · ε²) a O(N² log N) em pixels de imagem
 *   → São medidas DIFERENTES: Box Counting → dimensão fractal (d_B)
 *                              Gliding Box  → lacunaridade Λ(ε)
 * =============================================================================
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <iomanip>
#include <numeric>
#include <random>
#include <functional>
#include <complex>
#include <cassert>
#include <sstream>

// ─────────────────────────────────────────────────────────────────────────────
// TIPOS E ESTRUTURAS
// ─────────────────────────────────────────────────────────────────────────────

using Image2D  = std::vector<std::vector<double>>;
using Masses2D = std::vector<std::vector<double>>;
using Complex  = std::complex<double>;

/**
 * Contadores de operações básicas — instrumento central para validação
 * empírica das classes assintóticas derivadas analiticamente.
 */
struct OpsCounter {
    long long ops_massa    = 0;  // operações de soma/subtração no cálculo de massas
    long long ops_leitura  = 0;  // leituras de pixel (acesso a array)
    long long ops_escrita  = 0;  // escritas no array de massas
    long long ops_total    = 0;  // soma de todas as operações
    double    tempo_ms     = 0.0;

    void reset() { *this = OpsCounter{}; }

    long long total() const {
        return ops_massa + ops_leitura + ops_escrita;
    }

    void imprimir(const std::string& prefixo = "") const {
        std::cout << prefixo << "┌─ Operações Básicas ──────────────────────────────\n";
        std::cout << prefixo << "│  Somas/subtrações (massas): "
                  << std::setw(14) << ops_massa << "\n";
        std::cout << prefixo << "│  Leituras de pixel:         "
                  << std::setw(14) << ops_leitura << "\n";
        std::cout << prefixo << "│  Escritas em array:         "
                  << std::setw(14) << ops_escrita << "\n";
        std::cout << prefixo << "│  TOTAL:                     "
                  << std::setw(14) << total() << "\n";
        std::cout << prefixo << "│  Tempo:                     "
                  << std::setw(13) << std::fixed << std::setprecision(3)
                  << tempo_ms << " ms\n";
        std::cout << prefixo << "└──────────────────────────────────────────────────\n";
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// GERAÇÃO DE IMAGENS DE TESTE
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Gera imagem binária aleatória com densidade p de pixels objeto (valor=1).
 * Artigo usa p=0.25 como padrão (C = N²/4) para testes comparativos.
 */
Image2D gerar_imagem_binaria(int N, double densidade = 0.25, int seed = 42) {
    Image2D img(N, std::vector<double>(N, 0.0));
    std::mt19937 rng(seed);
    std::bernoulli_distribution dist(densidade);
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            img[i][j] = dist(rng) ? 1.0 : 0.0;
    return img;
}

/**
 * Gera imagem com padrão fractal de Sierpinski (autossimilar).
 * Útil para validar que Λ(ε) decresce monotonicamente com ε em fractais.
 */
Image2D gerar_sierpinski_imagem(int N) {
    Image2D img(N, std::vector<double>(N, 0.0));
    // Preenche pixel (i,j) se (i AND j) == 0 (padrão de Sierpinski)
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            if ((i & j) == 0) img[i][j] = 1.0;
    return img;
}

/** Conta pixels objeto (valor > 0) */
int contar_pixels_objeto(const Image2D& img) {
    int C = 0;
    for (const auto& row : img)
        for (double v : row)
            if (v > 0.0) ++C;
    return C;
}

// ─────────────────────────────────────────────────────────────────────────────
// PASSO 3 — CÁLCULO DA LACUNARIDADE (Equação 1 do artigo, Tolle 2008)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Calcula Λ(ε) a partir do array de massas M(i,j).
 * Eq. (1): Λ(ε) = B(N,ε) · Σ M(i,j)² / (Σ M(i,j))²
 * onde B(N,ε) = (N-ε+1)²  (número total de caixas)
 *
 * Esta é a fórmula otimizada de Tolle [17] que evita calcular
 * a distribuição de massas explicitamente.
 * Complexidade: O((N-ε+1)²) — percurso linear sobre as massas
 */
double calcular_lacunaridade(const Masses2D& M, int N, int eps) {
    int M_size = N - eps + 1;
    double sum_m  = 0.0;
    double sum_m2 = 0.0;

    for (int i = 0; i < M_size; ++i) {
        for (int j = 0; j < M_size; ++j) {
            double m = M[i][j];
            sum_m  += m;
            sum_m2 += m * m;
        }
    }

    if (sum_m < 1e-15) return 1.0;  // imagem vazia → lacunaridade máxima = 1

    long long B = (long long)M_size * M_size;  // B(N,ε) = (N-ε+1)²
    return (B * sum_m2) / (sum_m * sum_m);
}

// ─────────────────────────────────────────────────────────────────────────────
// MÉTODO 1 — ORIGINAL (Alain & Cloitre, 1991)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Método original: para cada caixa, soma todos os ε² pixels dentro dela.
 *
 * ANÁLISE (Eq. 3 do artigo):
 *   O_Original(N,ε) ~ (N-ε+1)² · ε²
 *
 * Para ε fixo: O(N²)     — cresce com o quadrado do tamanho da imagem
 * Para N fixo: O(ε²)     — cresce quadraticamente com o tamanho da caixa
 * Pior caso:   ε = N/2   → O(N² · N²/4) = O(N⁴/4)
 * Big-O geral: O(N² · ε²)
 *
 * Operação básica contada: cada leitura de pixel dentro da caixa.
 */
Masses2D gliding_box_original(const Image2D& S, int eps, OpsCounter& ops) {
    const int N      = static_cast<int>(S.size());
    const int M_size = N - eps + 1;
    Masses2D M(M_size, std::vector<double>(M_size, 0.0));

    // Loop externo: itera sobre todas as (N-ε+1)² posições de caixa
    for (int i = 0; i < M_size; ++i) {
        for (int j = 0; j < M_size; ++j) {
            double massa = 0.0;

            // Loop interno: soma os ε² pixels dentro da caixa
            // Esta é a OPERAÇÃO BÁSICA — O(ε²) por caixa
            for (int di = 0; di < eps; ++di) {
                for (int dj = 0; dj < eps; ++dj) {
                    massa += S[i + di][j + dj];
                    ++ops.ops_leitura;   // leitura de pixel
                    ++ops.ops_massa;     // soma
                }
            }
            M[i][j] = massa;
            ++ops.ops_escrita;
        }
    }
    return M;
}

// ─────────────────────────────────────────────────────────────────────────────
// MÉTODO 2 — SIDES (Backes, 2013)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Método Sides: reutiliza a massa da caixa anterior subtraindo a coluna
 * que saiu e adicionando a coluna que entrou (sliding window).
 *
 * ANÁLISE (Eq. 5 do artigo):
 *   O_Sides(N,ε) ~ (N-ε+1)² · 2ε
 *
 * Redução em relação ao original: ε² → 2ε operações por caixa
 * Para ε = N/2: fator de melhoria ≈ ε/2 = N/4
 * Big-O: O(N² · ε)
 *
 * Operação básica: adição/subtração de coluna (ε operações, não ε²).
 */
Masses2D gliding_box_sides(const Image2D& S, int eps, OpsCounter& ops) {
    const int N      = static_cast<int>(S.size());
    const int M_size = N - eps + 1;
    Masses2D M(M_size, std::vector<double>(M_size, 0.0));

    // ── Primeira linha de caixas: calcula do zero (custo ε²) ───────────────
    for (int j = 0; j < M_size; ++j) {
        double massa = 0.0;
        for (int di = 0; di < eps; ++di)
            for (int dj = 0; dj < eps; ++dj) {
                massa += S[di][j + dj];
                ++ops.ops_leitura;
                ++ops.ops_massa;
            }
        M[0][j] = massa;
        ++ops.ops_escrita;
    }

    // ── Linhas seguintes: subtrai linha de cima, adiciona linha de baixo ───
    // Custo por caixa: 2·ε operações (uma coluna de ε pixels entra, outra sai)
    for (int i = 1; i < M_size; ++i) {
        // Primeira coluna: calcula do zero a partir da linha acima
        double massa = M[i - 1][0];
        for (int dj = 0; dj < eps; ++dj) {
            massa -= S[i - 1][dj];      // remove linha que saiu pelo topo
            massa += S[i + eps - 1][dj]; // adiciona linha que entrou pela base
            ++ops.ops_leitura; ++ops.ops_leitura;
            ++ops.ops_massa;   ++ops.ops_massa;
        }
        M[i][0] = massa;
        ++ops.ops_escrita;

        // Colunas seguintes: subtrai coluna esquerda, adiciona coluna direita
        for (int j = 1; j < M_size; ++j) {
            massa = M[i][j - 1];
            for (int di = 0; di < eps; ++di) {
                massa -= S[i + di][j - 1];       // remove coluna esquerda
                massa += S[i + di][j + eps - 1]; // adiciona coluna direita
                ++ops.ops_leitura; ++ops.ops_leitura;
                ++ops.ops_massa;   ++ops.ops_massa;
            }
            M[i][j] = massa;
            ++ops.ops_escrita;
        }
    }
    return M;
}

// ─────────────────────────────────────────────────────────────────────────────
// MÉTODO 3 — INTEGRAL (Williams, 2015 / Viola & Jones, 2004)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Método Integral: constrói a imagem integral I onde I[i][j] = soma de
 * todos os pixels com índices ≤ (i,j). Cada massa é calculada com
 * exatamente 4 operações (4 acessos + 3 somas/subtrações).
 *
 * ANÁLISE (Eq. 7 do artigo):
 *   O_Integral(N,ε) ~ (N-ε+1)² · 4
 *
 * Independente de ε! → Θ(N²) para qualquer tamanho de caixa.
 * Esta é a propriedade mais importante: elimina a dependência em ε.
 * Big-O: O(N²)
 *
 * Custo adicional: construção da imagem integral = O(N²).
 * Trade-off de espaço: O(N²) memória extra para I.
 */
Masses2D gliding_box_integral(const Image2D& S, int eps, OpsCounter& ops) {
    const int N      = static_cast<int>(S.size());
    const int M_size = N - eps + 1;

    // ── Construção da imagem integral estendida I (tamanho N+1 × N+1) ──────
    // I[i][j] = Σ S[r][c]  para r < i, c < j
    // A linha e coluna 0 são zeros (extensão para simplificar fórmula de borda)
    // Custo: O(N²)
    std::vector<std::vector<double>> I(N + 1, std::vector<double>(N + 1, 0.0));
    for (int i = 1; i <= N; ++i) {
        for (int j = 1; j <= N; ++j) {
            I[i][j] = S[i-1][j-1]
                    + I[i-1][j]
                    + I[i][j-1]
                    - I[i-1][j-1];
            ops.ops_leitura += 4;  // 1 leitura de S + 3 leituras de I
            ops.ops_massa   += 3;  // 2 somas + 1 subtração
        }
    }

    // ── Cálculo das massas usando os 4 cantos da imagem integral ─────────
    // M(i,j) = I[i+ε][j+ε] - I[i][j+ε] - I[i+ε][j] + I[i][j]
    // Exatamente 4 leituras + 3 operações aritméticas por caixa → O(4) por caixa
    Masses2D M(M_size, std::vector<double>(M_size, 0.0));
    for (int i = 0; i < M_size; ++i) {
        for (int j = 0; j < M_size; ++j) {
            M[i][j] = I[i + eps][j + eps]  // canto inferior direito (+)
                    - I[i      ][j + eps]  // canto superior direito (-)
                    - I[i + eps][j      ]  // canto inferior esquerdo (-)
                    + I[i      ][j      ]; // canto superior esquerdo (+)
            ops.ops_leitura += 4;  // 4 acessos à imagem integral
            ops.ops_massa   += 3;  // 2 subtrações + 1 soma
            ++ops.ops_escrita;
        }
    }
    return M;
}

// ─────────────────────────────────────────────────────────────────────────────
// MÉTODO 4 — TRACE (Kovács & Erdélyi, 2024 — NOVO)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Método Trace (rastreamento reverso): em vez de iterar sobre caixas e
 * somar pixels, itera sobre pixels objeto e incrementa todas as caixas
 * que os contêm.
 *
 * ANÁLISE (Eq. 10 do artigo):
 *   O_Trace(N,ε) ~ C · ε²
 *   onde C = número de pixels objeto
 *
 * Para imagem esparsa (C << N²): muito mais rápido que Original.
 * Para imagem densa (C = N²):    mesmo custo que Original.
 * Para C = N²/4 (25%, padrão do artigo): 4× mais rápido que Original.
 *
 * Melhor caso:  C → 0  → O(1)   (imagem quase vazia)
 * Pior caso:    C = N² → O(N² · ε²) = Original
 * Dependência de textura: único método sensível à densidade da imagem.
 */
Masses2D gliding_box_trace(const Image2D& S, int eps, OpsCounter& ops) {
    const int N      = static_cast<int>(S.size());
    const int M_size = N - eps + 1;

    // Coleta índices e valores dos pixels objeto
    struct Pixel { int r, c; double v; };
    std::vector<Pixel> pixels_obj;
    pixels_obj.reserve(N * N / 4);  // reserva para 25% de densidade

    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            if (S[i][j] > 0.0)
                pixels_obj.push_back({i, j, S[i][j]});

    // Array de massas inicializado com zeros
    Masses2D M(M_size, std::vector<double>(M_size, 0.0));

    // Para cada pixel objeto, incrementa todas as caixas que o contêm
    // Uma caixa (r,c) contém o pixel (i,j) se: r ≤ i < r+ε  e  c ≤ j < c+ε
    // Ou seja: max(0, i-ε+1) ≤ r ≤ min(M_size-1, i)
    for (const auto& px : pixels_obj) {
        int r_min = std::max(0, px.r - eps + 1);
        int r_max = std::min(M_size - 1, px.r);
        int c_min = std::max(0, px.c - eps + 1);
        int c_max = std::min(M_size - 1, px.c);

        for (int r = r_min; r <= r_max; ++r) {
            for (int c = c_min; c <= c_max; ++c) {
                M[r][c] += px.v;
                ++ops.ops_massa;   // incremento da massa
                ++ops.ops_escrita; // escrita na caixa
            }
        }
        ++ops.ops_leitura;  // leitura do pixel objeto
    }
    return M;
}

// ─────────────────────────────────────────────────────────────────────────────
// FFT — Implementação DIT Cooley-Tukey (base 2, in-place)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * FFT 1D in-place — Decimation In Time (Cooley-Tukey, radix-2).
 * Requer que N seja potência de 2.
 * Complexidade: O(N log₂ N) — conforme Cochran [21] citado no artigo.
 */
void fft_1d(std::vector<Complex>& a, bool inversa = false) {
    int n = static_cast<int>(a.size());
    // Bit-reversal permutation
    for (int i = 1, j = 0; i < n; ++i) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(a[i], a[j]);
    }
    // Butterfly loops
    for (int len = 2; len <= n; len <<= 1) {
        double ang = 2 * M_PI / len * (inversa ? -1 : 1);
        Complex wlen(std::cos(ang), std::sin(ang));
        for (int i = 0; i < n; i += len) {
            Complex w(1);
            for (int j = 0; j < len / 2; ++j) {
                Complex u = a[i + j], v = a[i + j + len/2] * w;
                a[i + j]         = u + v;
                a[i + j + len/2] = u - v;
                w *= wlen;
            }
        }
    }
    if (inversa)
        for (auto& x : a) x /= n;
}

/**
 * FFT 2D separável: aplica FFT 1D em cada linha, depois em cada coluna.
 * Complexidade: O(N² log₂ N) para imagem N×N.
 */
std::vector<std::vector<Complex>> fft_2d(const Image2D& S, int fft_N) {
    std::vector<std::vector<Complex>> F(fft_N, std::vector<Complex>(fft_N, 0));

    // Copia imagem (zero-padding se necessário para potência de 2)
    int img_N = static_cast<int>(S.size());
    for (int i = 0; i < img_N; ++i)
        for (int j = 0; j < img_N; ++j)
            F[i][j] = S[i][j];

    // FFT nas linhas
    for (int i = 0; i < fft_N; ++i) fft_1d(F[i]);

    // FFT nas colunas
    std::vector<Complex> col(fft_N);
    for (int j = 0; j < fft_N; ++j) {
        for (int i = 0; i < fft_N; ++i) col[i] = F[i][j];
        fft_1d(col);
        for (int i = 0; i < fft_N; ++i) F[i][j] = col[i];
    }
    return F;
}

/** IFFT 2D: aplica FFT 2D inversa */
std::vector<std::vector<Complex>> ifft_2d(std::vector<std::vector<Complex>>& F, int fft_N) {
    // IFFT nas linhas
    for (int i = 0; i < fft_N; ++i) fft_1d(F[i], true);
    // IFFT nas colunas
    std::vector<Complex> col(fft_N);
    for (int j = 0; j < fft_N; ++j) {
        for (int i = 0; i < fft_N; ++i) col[i] = F[i][j];
        fft_1d(col, true);
        for (int i = 0; i < fft_N; ++i) F[i][j] = col[i];
    }
    return F;
}

// ─────────────────────────────────────────────────────────────────────────────
// MÉTODO 5 — CONVOLUTION (Kovács & Erdélyi, 2024 — NOVO)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Método de Convolução: usa o Teorema da Convolução para calcular todas
 * as massas em O(N² log N), independente de ε.
 *
 * Fundamentação (Eq. 12 do artigo):
 *   M̂_c(i,j) = Ŝ*(i,j) · Ê(i) · Ê(j)
 *   onde Ŝ = FFT 2D de S,  Ê = FFT 1D do kernel E_ε
 *   E_ε = [1,1,...,1,0,0,...,0]  (ε uns seguidos de zeros)
 *
 * Custo (Eq. 13):
 *   O_Fourier(N,ε) ~ 4·N²·log₂N + 7·N² + N·log₂N
 *                  = O(N² log₂ N)  ← independente de ε!
 *
 * Trade-off de espaço: O(N² · log₂ N) — usa números complexos de 128 bits.
 * (Maior uso de memória entre todos os métodos — Tabela 4 do artigo)
 *
 * Vantagem única: suporta kernels de qualquer forma (circular, etc.)
 * substituindo o kernel retangular E por um kernel arbitrário.
 */
Masses2D gliding_box_convolution(const Image2D& S, int eps, OpsCounter& ops) {
    const int N = static_cast<int>(S.size());

    // Encontra a menor potência de 2 ≥ N (necessário para FFT radix-2)
    int fft_N = 1;
    while (fft_N < N) fft_N <<= 1;

    // ── Passo 1: FFT 2D da imagem (Ŝ) ──────────────────────────────────────
    // Custo: O(N² log N)
    auto F_S = fft_2d(S, fft_N);
    ops.ops_massa += static_cast<long long>(2) * fft_N * fft_N * 
                     static_cast<int>(std::log2(fft_N));

    // ── Passo 2: Kernel 1D E_ε = [1,...,1, 0,...,0] e sua FFT (Ê) ──────────
    // Custo: O(N log N)
    std::vector<Complex> kernel(fft_N, 0);
    for (int i = 0; i < eps && i < fft_N; ++i) {
        kernel[i] = 1.0;
        ++ops.ops_escrita;
    }
    fft_1d(kernel);
    ops.ops_massa += fft_N * static_cast<int>(std::log2(std::max(2, fft_N)));

    // ── Passo 3: Multiplicação pontual M̂_c(i,j) = Ŝ*(i,j) · Ê(i) · Ê(j) ──
    // Custo: O(N²) — multiplicação elemento a elemento
    // Nota: conjungado complexo de S, multiplicado pelo kernel separável
    for (int i = 0; i < fft_N; ++i) {
        for (int j = 0; j < fft_N; ++j) {
            F_S[i][j] = F_S[i][j] * std::conj(kernel[i]) * std::conj(kernel[j]);
            ops.ops_massa   += 2;  // 2 multiplicações complexas
            ops.ops_leitura += 2;
        }
    }

    // ── Passo 4: IFFT 2D para obter massas complexas M_c ──────────────────
    // Custo: O(N² log N)
    ifft_2d(F_S, fft_N);
    ops.ops_massa += static_cast<long long>(2) * fft_N * fft_N *
                     static_cast<int>(std::log2(fft_N));

    // ── Passo 5: Extrai amplitudes (parte real ≈ massa real) ─────────────
    // Corta o canto superior esquerdo (N-ε+1) × (N-ε+1)
    int M_size = N - eps + 1;
    Masses2D M(M_size, std::vector<double>(M_size, 0.0));
    for (int i = 0; i < M_size; ++i) {
        for (int j = 0; j < M_size; ++j) {
            M[i][j] = std::abs(F_S[i][j]);  // amplitude = |parte real| para imagens reais
            ++ops.ops_escrita;
            ++ops.ops_leitura;
        }
    }
    return M;
}

// ─────────────────────────────────────────────────────────────────────────────
// VALIDAÇÃO CRUZADA — verifica que todos os métodos produzem mesmas massas
// ─────────────────────────────────────────────────────────────────────────────

bool validar_massas(const Masses2D& ref, const Masses2D& teste,
                    const std::string& nome, double tol = 1e-4) {
    if (ref.size() != teste.size()) return false;
    double max_err = 0;
    for (size_t i = 0; i < ref.size(); ++i)
        for (size_t j = 0; j < ref[i].size(); ++j)
            max_err = std::max(max_err, std::abs(ref[i][j] - teste[i][j]));

    bool ok = max_err < tol;
    std::cout << "  Validação " << std::left << std::setw(16) << nome
              << ": " << (ok ? "✓ OK" : "✗ FALHOU")
              << "  (erro máx = " << std::scientific << std::setprecision(2)
              << max_err << ")\n";
    return ok;
}

// ─────────────────────────────────────────────────────────────────────────────
// BENCHMARK — análise empírica da complexidade
// ─────────────────────────────────────────────────────────────────────────────

struct ResultadoBenchmark {
    int    N;
    int    eps;
    int    C;
    double tempo_original_ms;
    double tempo_sides_ms;
    double tempo_integral_ms;
    double tempo_trace_ms;
    double tempo_conv_ms;
    long long ops_original;
    long long ops_sides;
    long long ops_integral;
    long long ops_trace;
    long long ops_conv;
    double lac_original;
};

ResultadoBenchmark executar_benchmark(int N, int eps, double densidade = 0.25) {
    auto img = gerar_imagem_binaria(N, densidade);
    int C = contar_pixels_objeto(img);
    ResultadoBenchmark res{N, eps, C, 0,0,0,0,0, 0,0,0,0,0, 0};

    OpsCounter ops;
    Masses2D M;

    // Original
    ops.reset();
    auto t0 = std::chrono::high_resolution_clock::now();
    M = gliding_box_original(img, eps, ops);
    auto t1 = std::chrono::high_resolution_clock::now();
    res.tempo_original_ms = std::chrono::duration<double,std::milli>(t1-t0).count();
    res.ops_original = ops.total();
    res.lac_original = calcular_lacunaridade(M, N, eps);

    // Sides
    ops.reset();
    t0 = std::chrono::high_resolution_clock::now();
    M = gliding_box_sides(img, eps, ops);
    t1 = std::chrono::high_resolution_clock::now();
    res.tempo_sides_ms = std::chrono::duration<double,std::milli>(t1-t0).count();
    res.ops_sides = ops.total();

    // Integral
    ops.reset();
    t0 = std::chrono::high_resolution_clock::now();
    M = gliding_box_integral(img, eps, ops);
    t1 = std::chrono::high_resolution_clock::now();
    res.tempo_integral_ms = std::chrono::duration<double,std::milli>(t1-t0).count();
    res.ops_integral = ops.total();

    // Trace
    ops.reset();
    t0 = std::chrono::high_resolution_clock::now();
    M = gliding_box_trace(img, eps, ops);
    t1 = std::chrono::high_resolution_clock::now();
    res.tempo_trace_ms = std::chrono::duration<double,std::milli>(t1-t0).count();
    res.ops_trace = ops.total();

    // Convolution
    ops.reset();
    t0 = std::chrono::high_resolution_clock::now();
    M = gliding_box_convolution(img, eps, ops);
    t1 = std::chrono::high_resolution_clock::now();
    res.tempo_conv_ms = std::chrono::duration<double,std::milli>(t1-t0).count();
    res.ops_conv = ops.total();

    return res;
}

// ─────────────────────────────────────────────────────────────────────────────
// COMPARAÇÃO BOX COUNTING vs GLIDING BOX
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Tabela comparativa direta entre as duas abordagens para o trabalho de APA.
 * Documenta as diferenças fundamentais de propósito, entrada e saída.
 */
void imprimir_comparacao_algoritmos() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          COMPARAÇÃO ASSINTÓTICA: BOX COUNTING vs. GLIDING BOX               ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════════════════╝\n\n";

    std::cout << "  ┌──────────────────────┬───────────────────────────┬──────────────────────────┐\n";
    std::cout << "  │ Critério             │ Box Counting              │ Gliding Box (Integral)    │\n";
    std::cout << "  │                     │ (Liebovitch & Toth, 1989) │ (Kovács & Erdélyi, 2024)  │\n";
    std::cout << "  ├──────────────────────┼───────────────────────────┼──────────────────────────┤\n";
    std::cout << "  │ PROPÓSITO            │ Dimensão fractal d_B      │ Lacunaridade Λ(ε)         │\n";
    std::cout << "  │ ENTRADA              │ Nuvem de N pontos         │ Imagem N×N pixels         │\n";
    std::cout << "  │ SAÍDA                │ Escalar d_B ∈ [0, d_e]    │ Curva Λ(ε) por escala     │\n";
    std::cout << "  ├──────────────────────┼───────────────────────────┼──────────────────────────┤\n";
    std::cout << "  │ TEMPO (caso médio)   │ Θ(N·log N)                │ Θ(N²)                     │\n";
    std::cout << "  │ TEMPO (pior caso)    │ O(N·log N) [introsort]    │ O(N²·ε²) [Original]       │\n";
    std::cout << "  │ TEMPO (melhor caso)  │ Ω(N·log N)               │ Ω(N²) [Integral]          │\n";
    std::cout << "  ├──────────────────────┼───────────────────────────┼──────────────────────────┤\n";
    std::cout << "  │ ESPAÇO AUXILIAR      │ O(N)                      │ O(N²) [Original/Integral] │\n";
    std::cout << "  │ ESPAÇO (pior caso)   │ O(N)                      │ O(N²·log N) [Convolução]  │\n";
    std::cout << "  ├──────────────────────┼───────────────────────────┼──────────────────────────┤\n";
    std::cout << "  │ OPERAÇÃO BÁSICA      │ Comparação de string      │ Soma/subtração de pixel   │\n";
    std::cout << "  │ GARGALO              │ Ordenação (sort)          │ Soma de caixas            │\n";
    std::cout << "  │ CONSTANTE OCULTA     │ c ≈ k·d_e (escalas×dim)  │ c ≈ ε² (tamanho da caixa) │\n";
    std::cout << "  ├──────────────────────┼───────────────────────────┼──────────────────────────┤\n";
    std::cout << "  │ MELHOR PARA          │ Dados esparsos (pontos)   │ Imagens densas (pixels)   │\n";
    std::cout << "  │ PIOR PARA            │ Dimensão alta (d_e >> 1)  │ Imagens grandes + ε grande│\n";
    std::cout << "  │ PARALELIZAÇÃO        │ Sort paralelo: O(log²N)   │ Trivial: por linha/coluna │\n";
    std::cout << "  └──────────────────────┴───────────────────────────┴──────────────────────────┘\n\n";

    std::cout << "  NOTA FUNDAMENTAL: Os dois métodos NÃO são alternativas diretas.\n";
    std::cout << "  Box Counting mede GEOMETRIA (dimensão fractal = como o conjunto\n";
    std::cout << "  preenche o espaço). Gliding Box mede TEXTURA (lacunaridade =\n";
    std::cout << "  heterogeneidade na distribuição de vazios). São complementares.\n\n";
}

// ─────────────────────────────────────────────────────────────────────────────
// FUNÇÃO PRINCIPAL
// ─────────────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=================================================================\n";
    std::cout << "  GLIDING BOX — 5 Métodos (Kovács & Erdélyi, 2024)\n";
    std::cout << "  Análise Assintótica para Trabalho de APA\n";
    std::cout << "=================================================================\n";

    // ── SEÇÃO 1: Validação cruzada ─────────────────────────────────────────
    std::cout << "\n╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║  SEÇÃO 1 — VALIDAÇÃO CRUZADA (N=16, ε=3)            ║\n";
    std::cout << "║  Todos os métodos devem produzir massas idênticas    ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n\n";

    {
        int N_val = 16, eps_val = 3;
        auto img = gerar_imagem_binaria(N_val, 0.25, 42);
        OpsCounter ops;

        auto M_orig  = gliding_box_original   (img, eps_val, ops); ops.reset();
        auto M_sides = gliding_box_sides       (img, eps_val, ops); ops.reset();
        auto M_int   = gliding_box_integral    (img, eps_val, ops); ops.reset();
        auto M_trace = gliding_box_trace       (img, eps_val, ops); ops.reset();
        auto M_conv  = gliding_box_convolution (img, eps_val, ops);

        validar_massas(M_orig, M_sides, "Sides");
        validar_massas(M_orig, M_int,   "Integral");
        validar_massas(M_orig, M_trace, "Trace");
        validar_massas(M_orig, M_conv,  "Convolution");

        double lac = calcular_lacunaridade(M_orig, N_val, eps_val);
        std::cout << "\n  Lacunaridade Λ(ε=" << eps_val << ") = "
                  << std::fixed << std::setprecision(4) << lac << "\n";
        std::cout << "  (Λ > 1 → heterogeneidade; Λ = 1 → distribuição uniforme)\n";
    }

    // ── SEÇÃO 2: Dependência em N (tamanho da imagem) ─────────────────────
    std::cout << "\n╔══════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  SEÇÃO 2 — DEPENDÊNCIA EM N (ε=8 fixo, densidade=25%)                  ║\n";
    std::cout << "║  Valida: Original=O(N²), Sides=O(N²), Integral=O(N²), Conv=O(N²logN)   ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════════════╝\n\n";

    std::cout << std::left
              << std::setw(6)  << "N"
              << std::setw(10) << "C(obj)"
              << std::setw(12) << "Orig(ms)"
              << std::setw(12) << "Sides(ms)"
              << std::setw(12) << "Integ(ms)"
              << std::setw(12) << "Trace(ms)"
              << std::setw(12) << "Conv(ms)"
              << std::setw(10) << "Λ(ε=8)"
              << "\n";
    std::cout << std::string(86, '-') << "\n";

    std::ofstream log_n("gb_n_log.csv");
    log_n << "N,Original,Sides,Integral,Trace,Convolution\n";

    for (int N : {32, 64, 128, 256, 512}) {
        auto r = executar_benchmark(N, 8, 0.25);
        
        // Salva no CSV
        log_n << N << "," << r.tempo_original_ms << "," << r.tempo_sides_ms << ","
              << r.tempo_integral_ms << "," << r.tempo_trace_ms << "," << r.tempo_conv_ms << "\n";

        // Imprime no console (mantido como original)
        std::cout << std::left << std::setw(6) << N << std::setw(10) << r.C
                  << std::setw(12) << std::fixed << std::setprecision(2) << r.tempo_original_ms
                  << std::setw(12) << r.tempo_sides_ms << std::setw(12) << r.tempo_integral_ms
                  << std::setw(12) << r.tempo_trace_ms << std::setw(12) << r.tempo_conv_ms
                  << std::setw(10) << std::setprecision(3) << r.lac_original << "\n";
    }
    log_n.close();

    // ── SEÇÃO 3: Dependência em ε (tamanho da caixa) ──────────────────────
    std::cout << "\n╔══════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  SEÇÃO 3 — DEPENDÊNCIA EM ε (N=256 fixo, densidade=25%)                ║\n";
    std::cout << "║  Valida: Integral e Conv independem de ε; Original/Sides crescem com ε ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════════════╝\n\n";

    std::cout << std::left
              << std::setw(6)  << "ε"
              << std::setw(12) << "Orig(ms)"
              << std::setw(12) << "Sides(ms)"
              << std::setw(12) << "Integ(ms)"
              << std::setw(12) << "Trace(ms)"
              << std::setw(12) << "Conv(ms)"
              << std::setw(14) << "Ops_Orig"
              << std::setw(14) << "Ops_Integ"
              << "\n";
    std::cout << std::string(94, '-') << "\n";

    std::ofstream log_eps("gb_eps_log.csv");
    log_eps << "Epsilon,Original,Sides,Integral,Trace,Convolution\n";

    for (int eps : {2, 4, 8, 16, 32, 64, 128}) {
        if (eps >= 256) continue;
        auto r = executar_benchmark(256, eps, 0.25);
        
        // Salva no CSV
        log_eps << eps << "," << r.tempo_original_ms << "," << r.tempo_sides_ms << ","
                << r.tempo_integral_ms << "," << r.tempo_trace_ms << "," << r.tempo_conv_ms << "\n";

        // Imprime no console
        std::cout << std::left << std::setw(6) << eps << std::setw(12) << std::fixed 
                  << std::setprecision(2) << r.tempo_original_ms << std::setw(12) << r.tempo_sides_ms
                  << std::setw(12) << r.tempo_integral_ms << std::setw(12) << r.tempo_trace_ms
                  << std::setw(12) << r.tempo_conv_ms << std::setw(14) << r.ops_original
                  << std::setw(14) << r.ops_integral << "\n";
    }
    log_eps.close();
    std::cout << "\n  NOTA: Integral e Conv devem mostrar tempo ~constante com ε.\n";
    std::cout << "        Original e Sides devem crescer com ε.\n";

    // ── SEÇÃO 4: Dependência em densidade (só Trace varia) ────────────────
    std::cout << "\n╔══════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  SEÇÃO 4 — DEPENDÊNCIA NA DENSIDADE (N=256, ε=16)                      ║\n";
    std::cout << "║  Valida: apenas Trace depende da textura da imagem (C)                  ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════════════╝\n\n";

    std::cout << std::left
              << std::setw(10) << "Densidade"
              << std::setw(10) << "C(obj)"
              << std::setw(12) << "Orig(ms)"
              << std::setw(12) << "Integ(ms)"
              << std::setw(12) << "Trace(ms)"
              << std::setw(10) << "Λ"
              << "\n";
    std::cout << std::string(66, '-') << "\n";

    for (double d : {0.05, 0.10, 0.25, 0.50, 0.75, 0.95}) {
        auto r = executar_benchmark(256, 16, d);
        std::cout << std::left
                  << std::setw(10) << std::fixed << std::setprecision(2) << d
                  << std::setw(10) << r.C
                  << std::setw(12) << std::setprecision(2) << r.tempo_original_ms
                  << std::setw(12) << r.tempo_integral_ms
                  << std::setw(12) << r.tempo_trace_ms
                  << std::setw(10) << std::setprecision(4) << r.lac_original
                  << "\n";
    }
    std::cout << "\n  OBSERVAÇÃO: Λ decresce com o aumento da densidade (imagem mais\n";
    std::cout << "  homogênea = menor lacunaridade). Trace fica mais lento com C.\n";

    // ── SEÇÃO 5: Comparação de complexidade empírica ──────────────────────
    std::cout << "\n╔══════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  SEÇÃO 5 — VERIFICAÇÃO ASSINTÓTICA EMPÍRICA (N=256, ε=16)               ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════════════╝\n\n";

    {
        auto r = executar_benchmark(256, 16, 0.25);
        int N = 256, eps = 16;
        long long M_size = (N - eps + 1) * (N - eps + 1);

        double pred_orig  = (double)M_size * eps * eps;
        double pred_sides = (double)M_size * 2 * eps;
        double pred_int   = (double)M_size * 4;
        double pred_trace = (double)r.C * eps * eps;

        std::cout << "  N=" << N << ", ε=" << eps << ", C=" << r.C
                  << ", (N-ε+1)²=" << M_size << "\n\n";
        std::cout << std::left
                  << std::setw(14) << "Método"
                  << std::setw(16) << "Ops Medidas"
                  << std::setw(16) << "Ops Previstas"
                  << std::setw(12) << "Razão"
                  << std::setw(20) << "Classe Assintótica"
                  << "\n";
        std::cout << std::string(78, '-') << "\n";

        auto linha = [&](const std::string& nome, long long ops_med, double ops_prev, const std::string& classe) {
            std::cout << std::left
                      << std::setw(14) << nome
                      << std::setw(16) << ops_med
                      << std::setw(16) << std::fixed << std::setprecision(0) << ops_prev
                      << std::setw(12) << std::setprecision(3) << (ops_med / ops_prev)
                      << std::setw(20) << classe << "\n";
        };
        linha("Original",  r.ops_original, pred_orig,  "O(N²·ε²)");
        linha("Sides",     r.ops_sides,    pred_sides, "O(N²·ε)");
        linha("Integral",  r.ops_integral, pred_int,   "O(N²)");
        linha("Trace",     r.ops_trace,    pred_trace, "O(C·ε²)");

        std::cout << "\n  Se a Razão ≈ constante, a previsão assintótica está correta.\n";
        std::cout << "  Desvios são devidos a overhead de acesso à memória e cache.\n";
    }

    // ── SEÇÃO 6: Curvas de lacunaridade para análise fractal ──────────────
    std::cout << "\n╔══════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  SEÇÃO 6 — CURVA DE LACUNARIDADE Λ(ε) — SIERPINSKI vs ALEATÓRIA        ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════════════╝\n\n";

    {
        int N = 128;
        auto img_frac = gerar_sierpinski_imagem(N);
        auto img_rand = gerar_imagem_binaria(N, 0.25, 42);
        OpsCounter ops;

        std::cout << std::left
                  << std::setw(6)  << "ε"
                  << std::setw(18) << "Λ(Sierpinski)"
                  << std::setw(18) << "Λ(Aleatória)"
                  << "\n";
        std::cout << std::string(42, '-') << "\n";

        for (int eps : {2, 4, 8, 16, 32, 64}) {
            if (eps >= N) continue;
            ops.reset();
            auto M_frac = gliding_box_integral(img_frac, eps, ops);
            double lac_frac = calcular_lacunaridade(M_frac, N, eps);

            ops.reset();
            auto M_rand = gliding_box_integral(img_rand, eps, ops);
            double lac_rand = calcular_lacunaridade(M_rand, N, eps);

            std::cout << std::left
                      << std::setw(6)  << eps
                      << std::setw(18) << std::fixed << std::setprecision(4) << lac_frac
                      << std::setw(18) << lac_rand
                      << "\n";
        }
        std::cout << "\n  Sierpinski tem lacunaridade sistematicamente MAIOR que aleatória\n";
        std::cout << "  para mesma escala → mais heterogêneo (mais 'lacunar').\n";
    }

    // ── SEÇÃO 7: Tabela comparativa final com o Box Counting ──────────────
    imprimir_comparacao_algoritmos();

    // ── SEÇÃO 8: Sumário de complexidade ──────────────────────────────────
    std::cout << "=================================================================\n";
    std::cout << "  SUMÁRIO DAS CLASSES ASSINTÓTICAS — GLIDING BOX\n";
    std::cout << "=================================================================\n";
    std::cout << "  ε = tamanho da caixa, N = lado da imagem, C = pixels objeto\n\n";
    std::cout << "  Original:    O(N²·ε²) tempo  |  O(N²) espaço\n";
    std::cout << "  Sides:       O(N²·ε)  tempo  |  O(N²) espaço  [2× melhor que Original]\n";
    std::cout << "  Integral:    O(N²)    tempo  |  O(N²) espaço  [independe de ε!]\n";
    std::cout << "  Trace:       O(C·ε²)  tempo  |  O(N²) espaço  [depende da textura]\n";
    std::cout << "  Convolution: O(N²logN)tempo  |  O(N²logN) esp [independe de ε; suporta qualquer forma]\n\n";
    std::cout << "  RECOMENDAÇÃO GERAL (artigo, pg. 11): Integral para uso geral.\n";
    std::cout << "  PARA COMPARAÇÃO COM BOX COUNTING: Box Counting é mais eficiente\n";
    std::cout << "  em N_pts ≪ N² (dados esparsos); Gliding Box necessário para\n";
    std::cout << "  lacunaridade (informação não obtida por Box Counting).\n";
    std::cout << "=================================================================\n\n";

    return 0;
}
