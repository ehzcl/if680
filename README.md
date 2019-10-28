# IF680 - Processamento Gráfico
Repositório do projeto da disciplina de IF680 - Processamento Gráfico.


## Funções importantes
Operação | Função |
-|-|
Produto vetorial | cross(vetor1, vetor2) |
Produto interno  | dot(vetor1, vetor2) |
Normalização | vetor1.make_unit_vector(vetor2) |

## Atenção
 *  Se você não entendeu ou não lembra das funções, você deve (buscar lembrar)[https://www.scratchapixel.com/lessons/mathematics-physics-for-computer-graphics/geometry].

* Sempre se pergunte conscientemente se esse vetor é / não é ou não deve / deve ser normalizado.

* Não confunda ponto com vetor.

* Não confunda produto vetorial com produto interno.

## Arquivos Importantes 
Camera.h
main.cpp

## Da teoria ao C ++
Em nosso código C ++, não faremos a distinção entre pontos, vetores e normais; nós representamos os três com um Vec3classe (um modelo de classe para que possamos criar versões float, int ou double, conforme necessário). Alguns desenvolvedores preferem diferenciá-los. Isso limita claramente a possibilidade de cometer erros. Por experiência, achamos mais eficiente (menos código escrever em primeiro lugar) lidar apenas com uma classe única (como a biblioteca OpenEXR). No entanto, ainda teremos que chamar algumas funções específicas com cuidado, dependendo se o Vec3 com o qual estamos lidando representa ou não um ponto, um vetor ou um normal. Como você deve se lembrar, isso é particularmente crítico quando usamos [transformações em pontos e vetores](https://www.scratchapixel.com/lessons/mathematics-physics-for-computer-graphics/geometry/transforming-points-and-vectors) e [transformações normais](https://www.scratchapixel.com/lessons/mathematics-physics-for-computer-graphics/geometry/transforming-normals).
