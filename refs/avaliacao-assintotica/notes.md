# pontos importantes da avalicao assintotica

## *A Guide to Experimental Algorithmics*

livro guia para realizar experimentos algoritimicos, nao eh apenas sobre uma avaliacao assintotica, eh mais geral

**objetivos da experimentação**
1. precisam ser reproduziveis (qualquer um que testar terá os mesmos resultados)
2. precisam ser eficientes (produz resultados sem perder tempo/recursoss)
3. *newssworthiness* (seja de interesse e úteis para a comunidade científica)

**dobrar os experimentos**
uma boa estratégia é dobrar os parametros do experimentos para observar o comportament

por exemplo, ir dobrando a entrada *n* e medir as medidas desejadas

## *An Experimental Evaluation of SimRank-based Similarity Search Algorithms*

aqui é feito uma revisão sistemática de metodos para calcular a medidad SimRank em grafos

separou em tres formas de calcular (algoritmos iterativos, nao-iterativos e randomicos), ai pegou de 3 a 4 implementacoes de cada um e testou

vamos se basear nesse trabalho

## *Comparison of gliding box and box-counting methods in river network analysis* (2007)

*river networks*
- estuda o comportamento dendritico dos rios
- depois de mandelbrot, aplicaram analise fractal nos rios
- foi concluido que os rios exibiam um comportamento com autoafinidade fractal

*fractal and multifractal*
- sao aplicados em vários campos da ciência
- uma geometria multifractal eh um fractal nao uniforme, que exibe flutuacoes locais de densidade
- esses multifractais precisam de uma sequencia de dimensoes fractais generalizadas
- uma MultiFractal Analysis (MFA) extrai essas dimensoes

*MFA*
- primeiro aplicados nas redes de rios em 1992
- alem da geometria de redes de rios, MFA ja foi aplicado em encostas, topografia, otimizacao de redes, disssipacao de energia...

*calculating the generalized dimensions*
- nao eh facil de ser calculada (tel 1989)
- vicsek 1990 propos metodos para computar
- meritos e limitacoes foram discutidos por varios trabalhos (1989~2006)
- as dificuldades estao na estimacao assintotica de valores relevantess na MFA, que é limitada pela resolucao das imagens
- o limite teorico nao consegue ser atingido na pratica (box-size tendendo a zero)
- pra superar essses limite, varios metodossforam propostos

**objetivos**
os objetivos desse artigo eh ver o efeito da escolha dos tamanhos de caixa no BC e o efeito do metodo da subdivisao no calculo da dimensao no BC e GB

## *Comparison of gliding box and box-counting methods in soil image analysis*

*exact value of the generalized dim*
- varios autores defendem que o exato valor nao pode ser calculado
- vicsek propos metodos praticos para computar essa dim gen, mostrando que alguns metodos padroes para determinar dim fractal precisavam de cautela no uso
- principal dificuldade eh que o limite ideal nao consegue ser atingido

**objetivo**
examinar o efeito da alteracao do tamanho da caixa e o metodo dea subdivisoa aplicada no calculo de dimensoes fractais generalizadas

*gliding box method*
- inicialmente usado para analise de lacunaridade
- adapatado por Cheng para MFA

## *Association of Fractal Geometry and Data Augmentation Through GANs and XAI for Classification of Histology Images*

utiliza de metodos fractais para extrair características complexas e utiliza-las na classificaco de imagens histopatologicas

utiliza a tecnica de extracao fractal baseada na geometria fractal de Roberto 2021

a abordagem multiescalar e multidimensional (proposta por Ivanovici e Richdard, 2010) eh utilizada, a multiescalar consiste na aplicacao do gliding-box para avaliar todas as regioes sda imagem em diferentes escalas, já a multidimensional consiste em avaliar a similiraidade de niveis de pixels dentro de uma reggiao LxL, considerando intensidade RGB e a distancia entre cada pixel e o pixel central da regiao