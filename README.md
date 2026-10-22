# Quebra-cabeça com rotação de mapa

## Compilar e rodar

```
g++ -std=c++17 -o jogo jogo.cpp
./jogo
```

Rode sempre da pasta onde estão os arquivos `mapaN.csv`. O jogo cria `jogo_salvo.csv`,
`recordes.csv` e (se faltar) `mapaN.sol` nessa mesma pasta.

## Mapas incluídos

| Arquivo | Título | Tamanho |
|---|---|---|
| `mapa1.csv` | Fase 1 | 9 |
| `mapa2.csv` | Fase 2 | 13 |
| `mapa3.csv` | Fase 3 | 20 |
| `mapa4.csv` | Fase 4 | 20 |
| `mapa5.csv` | Fase 5 | 20 |

Todos exigem girar o mapa pelo menos uma vez. Para criar mais, basta adicionar
`mapa6.csv`, `mapa7.csv`... na pasta.

## Controles

| Tecla | Ação |
|---|---|
| `w` `a` `s` `d` | mover |
| `q` / `e` | girar o mapa para a esquerda / direita (só sobre a alavanca) |
| `Enter` | concluir o mapa (jogador sobre a saída) |
| `r` | reiniciar o mapa |
| `m` | voltar ao menu (salva o jogo para o "Continuar") |

No menu e nas listas: `w`/`s` movem, `Enter` escolhe.

## Arquivos

| Arquivo | Conteúdo | Escrito por |
|---|---|---|
| `mapaN.csv` | cenário inicial do mapa N | a equipe, à mão |
| `mapaN.sol` | teclas que resolvem o mapa N | a equipe ou o jogo |
| `jogo_salvo.csv` | partida interrompida | o jogo |
| `recordes.csv` | melhores resultados | o jogo |

## mapaN.csv

O jogo descobre os mapas tentando abrir `mapa1.csv`, `mapa2.csv`, `mapa3.csv`... até um
não existir. Não há número fixo de mapas.

```
11                      <- linha 1: dimensão n (de 3 a 30)
Fase 1                  <- linha 2: título (texto livre, sem vírgulas)
1,1,1,1,1,1,1,1,1,1,1   <- linhas 3 a n+2: n linhas com n códigos de 0 a 7,
...                        separados por vírgula, sem espaços
```

| Código | Significado |
|---|---|
| 0 | vazio |
| 1 | parede |
| 2 | posição inicial do jogador (sempre sobre célula vazia) |
| 3 | bloco (cai com a gravidade depois de cada giro) |
| 4 | alavanca (é preciso estar sobre ela para girar) |
| 5 | saída |
| 6 | obstáculo que abre a cada giro |
| 7 | passagem que fecha a cada giro |

Validações feitas ao abrir o jogo (qualquer erro mostra o arquivo e o motivo):
dimensão entre 3 e 30, título não vazio e sem vírgulas, exatamente n linhas com
exatamente n códigos, só dígitos de 0 a 7, exatamente um código 2 e ao menos uma saída (5).
Ao concluir o último mapa, o jogo termina.

## mapaN.sol

Uma sequência de teclas (`w a s d q e`), em uma ou mais linhas. Espaços e quebras de
linha são ignorados.

Em "Ver solução", o jogo mostra a sequência do `.sol` uma única vez na tela. Se o `.sol`
não existir ou estiver vazio, o jogo procura uma solução aleatória e grava o arquivo. Ela
nunca é a mais curta possível, mas fica com no máximo 1,5 vez o tamanho da mais curta (mais 2 teclas).

## jogo_salvo.csv

Formato livre, escrito pelo jogo:

```
mapa,contador,movimentos,rerun1,giro,n
<n linhas com n números separados por vírgula: a matriz do jogo naquele momento>
```

- `mapa`: número do mapa em andamento
- `contador`: rotações feitas; `movimentos`: passos dados
- `rerun1`: o que há embaixo do jogador (0, 3, 5, 6 ou 8)
- `giro`: 1 ou 0 (alterna a cada giro)
- `n`: dimensão do mapa
- a matriz usa os valores internos do jogo: 0 vazio, 1 parede, 2 bloco, 3 alavanca,
  4/6 e 7/8 terrenos que alternam a cada giro, 5 saída, 9 jogador

O arquivo é validado ao ler; se estiver corrompido, o botão "Continuar" fica desabilitado.
"Novo jogo" e a vitória final esvaziam o arquivo.

## recordes.csv

```
mapa,movimentos,rotacoes
1,22,13
...
```

Uma linha por resultado, com os 5 melhores de cada mapa, ordenados por mapa e depois pelo
menor total de movimentos + rotações (em empate, quem chegou primeiro fica na frente).
