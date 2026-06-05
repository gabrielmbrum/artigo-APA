import pandas as pd
import matplotlib.pyplot as plt

# Estilo global para gráficos científicos
plt.style.use('seaborn-v0_8-whitegrid')
plt.rcParams.update({'font.size': 12, 'axes.titlesize': 16, 'axes.labelsize': 14})

def plot_box_counting_N():
    """Gera o Gráfico para o Slide 1: Escalabilidade do Box Counting"""
    df = pd.read_csv('bc_n_log.csv')
    
    fig, ax1 = plt.subplots(figsize=(8, 6))
    
    # Eixo de Tempo
    ax1.plot(df['N'], df['Tempo_ms'], marker='o', color='#1f77b4', linewidth=2.5, label='Tempo (ms)')
    ax1.set_xlabel('Tamanho da Nuvem de Pontos (N)')
    ax1.set_ylabel('Tempo de Execução (ms)', color='#1f77b4', fontweight='bold')
    ax1.tick_params(axis='y', labelcolor='#1f77b4')
    
    # Eixo Secundário: Operações vs N log N
    ax2 = ax1.twinx()
    ax2.plot(df['N'], df['Comparacoes'], marker='s', color='#d62728', linestyle='--', linewidth=2, label='Comparações Reais')
    ax2.set_ylabel('Nº de Comparações (Escala Algorítmica)', color='#d62728', fontweight='bold')
    ax2.tick_params(axis='y', labelcolor='#d62728')
    
    plt.title('Box Counting: Crescimento Assintótico O(N log N)', pad=15)
    fig.tight_layout()
    plt.savefig('grafico_slide1_box_counting_N.png', dpi=300)
    print("Salvo: grafico_slide1_box_counting_N.png")

def plot_gliding_box_N():
    """Gera o Gráfico para o Slide 2: Dependência do Tamanho da Imagem (N)"""
    df = pd.read_csv('gb_n_log.csv')
    
    plt.figure(figsize=(9, 6))
    plt.plot(df['N'], df['Original'], marker='o', color='#ff7f0e', linewidth=2.5, label='Original (Força Bruta)')
    plt.plot(df['N'], df['Convolution'], marker='^', color='#9467bd', linewidth=2.5, label='Convolução (FFT)')
    plt.plot(df['N'], df['Integral'], marker='s', color='#2ca02c', linewidth=2.5, label='Imagem Integral (Estado da Arte)')
    
    plt.title('Gliding Box: Impacto do Tamanho da Imagem (N)', fontweight='bold', pad=15)
    plt.xlabel('Dimensão da Imagem (N x N pixels)')
    plt.ylabel('Tempo de Execução (ms)')
    plt.legend(frameon=True, shadow=True)
    plt.tight_layout()
    plt.savefig('grafico_slide2_gliding_box_N.png', dpi=300)
    print("Salvo: grafico_slide2_gliding_box_N.png")

def plot_gliding_box_eps():
    """Gera o Gráfico para o Slide 3: O Gargalo da Escala (Epsilon)"""
    df = pd.read_csv('gb_eps_log.csv')
    
    plt.figure(figsize=(9, 6))
    plt.plot(df['Epsilon'], df['Original'], marker='o', color='#d62728', linewidth=2.5, label='Método Original O(N^2 x epsilon^2)')
    plt.plot(df['Epsilon'], df['Sides'], marker='v', color='#ff7f0e', linewidth=2, linestyle='--', label='Método Sides O(N^2 x epsilon)')
    plt.plot(df['Epsilon'], df['Integral'], marker='s', color='#2ca02c', linewidth=3, label='Imagem Integral Theta(N^2)')
    
    # Anotação apontando a linha constante
    plt.annotate('Imune ao epsilon', xy=(64, df['Integral'].iloc[-2]), xytext=(32, 20),
                 arrowprops=dict(facecolor='black', shrink=0.05), fontsize=12, fontweight='bold', color='#2ca02c')

    plt.title('A Quebra da Força Bruta: Crescimento da Caixa (epsilon)', fontweight='bold', pad=15)
    plt.xlabel('Tamanho da Caixa Deslizante (epsilon)')
    plt.ylabel('Tempo de Execução (ms)')
    plt.legend(frameon=True, shadow=True, loc='upper left')
    plt.tight_layout()
    plt.savefig('grafico_slide3_gliding_box_eps.png', dpi=300)
    print("Salvo: grafico_slide3_gliding_box_eps.png")

if __name__ == '__main__':
    try:
        plot_box_counting_N()
        plot_gliding_box_N()
        plot_gliding_box_eps()
        print("\nTodos os gráficos foram gerados com sucesso e estão prontos para os slides!")
    except FileNotFoundError as e:
        print(f"Erro: Arquivo CSV não encontrado. Tem a certeza que correu o C++ atualizado primeiro? Detalhe: {e}")