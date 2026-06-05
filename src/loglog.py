import numpy as np
import matplotlib.pyplot as plt
from PIL import Image

def plot_box_counting(image_path):
    print(f"Carregando imagem: {image_path}...")
    
    # 1. Carregar a imagem e converter para tons de cinza (L)
    img = Image.open(image_path).convert('L')
    img_array = np.array(img)
    
    # 2. Binarizar a imagem
    # Assumimos que o fractal é a parte escura (ex: traços pretos em fundo branco).
    # O valor 128 é o limiar (threshold).
    binary_img = img_array < 128 
    
    # Extrair as coordenadas espaciais dos pixels que compõem o objeto (valor True)
    pixels_objeto = np.column_stack(np.where(binary_img > 0))
    if len(pixels_objeto) == 0:
        print("Erro: Nenhum pixel válido encontrado na imagem.")
        return
        
    print(f"Total de pixels do objeto: {len(pixels_objeto)}")

    # 3. Definir os tamanhos das caixas (epsilon)
    # A escala de epsilon (e) cresce em potências de 2
    min_dim = min(binary_img.shape)
    max_power = int(np.floor(np.log2(min_dim)))
    
    # Criamos caixas de tamanho 2, 4, 8, 16... até o limite da imagem
    epsilons = [2**i for i in range(1, max_power)]
    contagens = []
    
    print("Iniciando a contagem de caixas...")
    # 4. Processo de Box Counting
    for eps in epsilons:
        # Dividir as coordenadas pelo tamanho da caixa mapeia os pixels para a grade
        caixas_atingidas = pixels_objeto // eps
        
        # Encontrar apenas as caixas únicas (eliminando sobreposição de pontos na mesma caixa)
        caixas_unicas = np.unique(caixas_atingidas, axis=0)
        
        # N(e) é o número de caixas únicas que contêm pelo menos um ponto
        contagens.append(len(caixas_unicas))
        print(f"  - Tamanho da caixa (\u03B5={eps}): {len(caixas_unicas)} caixas ocupadas")

    # 5. Preparar os dados para o gráfico Log-Log (Base E natural, conforme fórmula do limite)
    # Eixo X = log(1 / epsilon) = -log(epsilon)
    # Eixo Y = log( N(epsilon) )
    x_log_inv_eps = np.log(1.0 / np.array(epsilons))
    y_log_N = np.log(contagens)
    
    # 6. Regressão Linear: log(N(e)) = -D * log(e) + K  =>  y = D * x + K
    # Encontramos os coeficientes da reta
    coeficientes = np.polyfit(x_log_inv_eps, y_log_N, 1)
    D = coeficientes[0] # A inclinação (Slope) é a Dimensão Fractal!
    K = coeficientes[1] # A constante (Intercept)
    
    reta_regressao = D * x_log_inv_eps + K
    
    print(f"\nResultado Matemático:")
    print(f"  Dimensão Fractal Estimada (D): {D:.4f}")

    # 7. Plotagem Profissional para o Slide
    plt.figure(figsize=(9, 6))
    
    # Plotar os pontos medidos
    plt.scatter(x_log_inv_eps, y_log_N, color='#1f77b4', s=80, zorder=5, 
                label='Amostras do Algoritmo: $\log(N(\epsilon))$')
    
    # Plotar a reta da regressão
    plt.plot(x_log_inv_eps, reta_regressao, color='#d62728', linestyle='--', linewidth=2.5, 
             label=f'Regressão Linear (Slope = {D:.3f})\nDimensão Fractal ($D$) $\\approx$ {D:.3f}')
    
    # Customização visual do Gráfico
    plt.title('Avaliação Assintótica: Método de Box Counting', fontsize=16, fontweight='bold', pad=15)
    plt.xlabel('$\log(1/\epsilon)$', fontsize=14)
    plt.ylabel('$\log(N(\epsilon))$', fontsize=14)
    plt.grid(True, which="both", ls="--", alpha=0.5)
    plt.legend(fontsize=12, loc='upper left', frameon=True, shadow=True)
    
    # Ajustar layout e salvar imagem de alta resolução para o Powerpoint/LaTeX
    plt.tight_layout()
    nome_arquivo = 'grafico_slide_box_counting.png'
    plt.savefig(nome_arquivo, dpi=300)
    print(f"\nGráfico salvo com sucesso: '{nome_arquivo}'. Pronto para inserir no slide!")
    
    # Mostrar na tela
    plt.show()

# ==========================================
# EXECUÇÃO DO SCRIPT
# ==========================================
if __name__ == "__main__":
    # Substitua pelo nome de alguma imagem fractal ou textura que você tenha no computador
    # Ex: 'costa_britanica.png', 'sierpinski.png' ou 'imagem_medica.jpg'
    
    # Para teste, estou colocando um nome genérico
    caminho_da_imagem = '../images/folha.png' 
    
    try:
        plot_box_counting(caminho_da_imagem)
    except FileNotFoundError:
        print(f"ERRO: A imagem '{caminho_da_imagem}' não foi encontrada.")
        print("Por favor, coloque uma imagem na mesma pasta do script e atualize o nome no código.")