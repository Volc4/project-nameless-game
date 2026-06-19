#ifndef SPATIAL_GRID_H
#define SPATIAL_GRID_H

// ===========================================================================
//  SpatialGrid.h — Particionamento Espacial por Grade Uniforme
//
//  ARQUITETURA
//  -----------
//  O espaço de jogo é dividido em células quadradas de TAMANHO_CELULA
//  unidades de mundo. Cada célula armazena apenas os ÍNDICES dos zumbis e
//  projéteis que a habitam, mantendo os vetores originais de EstadoDoJogo
//  como fonte de verdade.
//
//  O sistema é reconstruído do zero a cada frame (construirGrade + consultas),
//  logo nunca acumula estado obsoleto. O custo de construção é O(N+M) e o
//  de consulta é O(células_vizinhas × entidades_nelas), tipicamente O(1)
//  para densidades normais de jogo.
//
//  COMPATIBILIDADE
//  ---------------
//  C++98 puro: sem auto, sem lambda, sem unordered_map, sem nullptr,
//  sem enum class, sem smart pointers, sem bibliotecas externas.
//  Usa apenas <vector>, <cmath> e <cstdlib> da STL padrão.
//
//  LIMITES
//  -------
//  TAMANHO_CELULA  — lado de cada célula em unidades de mundo.
//                    Deve ser >= 2× o maior raioColisao de qualquer entidade
//                    para garantir que toda colisão seja detectada consultando
//                    apenas as 9 células vizinhas (3×3).
//
//  LIMITE_GRADE    — metade do lado total da grade quadrada em unidades de
//                    mundo. Entidades fora deste limite são inseridas na
//                    célula de borda mais próxima (clamp), nunca descartadas.
//
//  NUM_CELULAS     — número de células por eixo = (2*LIMITE_GRADE)/TAMANHO_CELULA.
//                    Deve ser inteiro exato; ajuste LIMITE_GRADE se necessário.
// ===========================================================================

#include "Entities.h"
#include <vector>
#include <cmath>   // floorf

// ---------------------------------------------------------------------------
// Parâmetros da grade — ajuste conforme o mundo do jogo
// ---------------------------------------------------------------------------
//
// Raio máximo de colisão no projeto:
//   Zumbi Tank      raioColisao = 1.10f
//   Projétil máximo raioColisao ≈ 0.42 × 3.5 (DANO3) ≈ 1.47f
//
// TAMANHO_CELULA = 4.0f garante que qualquer par (tiro, zumbi) com colisão
// AABB seja encontrado nas 9 células vizinhas, pois:
//   raioTiro + raioZumbi ≤ 1.47 + 1.10 = 2.57 < TAMANHO_CELULA (4.0)
//
// LIMITE_GRADE = 180.0f cobre a arena (LIMITE_ARENA = 150.0f) com margem
// suficiente para spawn externo (raioSpawn = 30.0f).
//
const float TAMANHO_CELULA = 4.0f;
const float LIMITE_GRADE   = 180.0f;  // cobre -180..+180 em X e Z
const int   NUM_CELULAS    = 90;      // (2 * 180) / 4 = 90 células por eixo

// ---------------------------------------------------------------------------
// CelulaEspacial
//   Armazena índices (em jogo.horda / jogo.tirosNaTela) das entidades
//   cujo centro cai dentro desta célula.
// ---------------------------------------------------------------------------
struct CelulaEspacial {
    std::vector<int> inimigos;   // índices em EstadoDoJogo::horda
    std::vector<int> projeteis;  // índices em EstadoDoJogo::tirosNaTela
};

// ---------------------------------------------------------------------------
// GradeEspacial
//   Contém a matriz de células e as funções de construção + consulta.
//   Instanciar uma vez (p.ex. como variável local em processarColisoes)
//   e chamar construirGrade() a cada frame antes das consultas.
// ---------------------------------------------------------------------------
struct GradeEspacial {
    // grade[linha][coluna], linha = eixo Z, coluna = eixo X
    CelulaEspacial celulas[NUM_CELULAS][NUM_CELULAS];

    // -----------------------------------------------------------------------
    // coordParaCelula
    //   Converte uma coordenada de mundo (x ou z) para o índice inteiro da
    //   célula correspondente, com clamp para não sair dos limites.
    // -----------------------------------------------------------------------
    int coordParaCelula(float coord) const {
        // Desloca para o sistema de indexação [0, NUM_CELULAS)
        float normalizado = (coord + LIMITE_GRADE) / TAMANHO_CELULA;
        int   idx         = (int)floorf(normalizado);

        if (idx < 0)             idx = 0;
        if (idx >= NUM_CELULAS)  idx = NUM_CELULAS - 1;

        return idx;
    }

    // -----------------------------------------------------------------------
    // limparGrade
    //   Esvazia todos os vetores sem desalocar memória (clear() mantém
    //   capacidade, minimizando realocações nos frames seguintes).
    // -----------------------------------------------------------------------
    void limparGrade() {
        for (int l = 0; l < NUM_CELULAS; ++l) {
            for (int c = 0; c < NUM_CELULAS; ++c) {
                celulas[l][c].inimigos.clear();
                celulas[l][c].projeteis.clear();
            }
        }
    }

    // -----------------------------------------------------------------------
    // construirGrade
    //   Reconstrói a grade do zero a partir do estado atual do jogo.
    //   Deve ser chamada UMA VEZ por frame, antes de qualquer consulta.
    //   Complexidade: O(N_zumbis + N_projeteis).
    // -----------------------------------------------------------------------
    void construirGrade(const EstadoDoJogo& jogo) {
        limparGrade();

        // Insere zumbis vivos
        for (int i = 0; i < (int)jogo.horda.size(); ++i) {
            const Zumbi& z = jogo.horda[i];
            if (!z.vivo) continue;

            int col = coordParaCelula(z.posicao.x);
            int lin = coordParaCelula(z.posicao.z);
            celulas[lin][col].inimigos.push_back(i);
        }

        // Insere projéteis ativos
        for (int i = 0; i < (int)jogo.tirosNaTela.size(); ++i) {
            const Projetil& p = jogo.tirosNaTela[i];
            if (!p.ativo) continue;

            int col = coordParaCelula(p.posicao.x);
            int lin = coordParaCelula(p.posicao.z);
            celulas[lin][col].projeteis.push_back(i);
        }
    }

    // -----------------------------------------------------------------------
    // obterInimigosVizinhos
    //   Dado um ponto de consulta (posX, posZ), preenche o vetor de saída
    //   com os índices de todos os zumbis presentes nas 9 células ao redor
    //   (3×3, incluindo a própria célula). Índices duplicados não ocorrem
    //   porque cada zumbi pertence a exatamente uma célula.
    //   Complexidade: O(entidades_nas_9_células).
    // -----------------------------------------------------------------------
    void obterInimigosVizinhos(float posX, float posZ,
                                std::vector<int>& saida) const {
        saida.clear();

        int colCentro = coordParaCelula(posX);
        int linCentro = coordParaCelula(posZ);

        for (int dl = -1; dl <= 1; ++dl) {
            int lin = linCentro + dl;
            if (lin < 0 || lin >= NUM_CELULAS) continue;

            for (int dc = -1; dc <= 1; ++dc) {
                int col = colCentro + dc;
                if (col < 0 || col >= NUM_CELULAS) continue;

                const std::vector<int>& bucket = celulas[lin][col].inimigos;
                for (int k = 0; k < (int)bucket.size(); ++k) {
                    saida.push_back(bucket[k]);
                }
            }
        }
    }

    // -----------------------------------------------------------------------
    // obterProjeteisvizinhos
    //   Equivalente a obterInimigosVizinhos, mas para projéteis.
    //   Usado em processarColisaoZumbiJogador_Grade se quisermos futuramente
    //   checar se o zumbi foi atingido (não necessário agora, mas disponível).
    // -----------------------------------------------------------------------
    void obterProjeteisvizinhos(float posX, float posZ,
                                 std::vector<int>& saida) const {
        saida.clear();

        int colCentro = coordParaCelula(posX);
        int linCentro = coordParaCelula(posZ);

        for (int dl = -1; dl <= 1; ++dl) {
            int lin = linCentro + dl;
            if (lin < 0 || lin >= NUM_CELULAS) continue;

            for (int dc = -1; dc <= 1; ++dc) {
                int col = colCentro + dc;
                if (col < 0 || col >= NUM_CELULAS) continue;

                const std::vector<int>& bucket = celulas[lin][col].projeteis;
                for (int k = 0; k < (int)bucket.size(); ++k) {
                    saida.push_back(bucket[k]);
                }
            }
        }
    }
};

// ===========================================================================
//  SUBSTITUIÇÕES DE processarColisoesTiros e processarColisaoZumbiJogador
//
//  Assinaturas IDÊNTICAS às originais em GameLogic.h — bastará substituir
//  as chamadas existentes em timer() por estas versões aceleradas.
//
//  Regra de uso em Main.cpp / timer():
//
//    // (1) Declare a grade uma única vez fora do timer, ou como static:
//    static GradeEspacial gradeGlobal;
//
//    // (2) Dentro do timer(), ANTES das colisões:
//    gradeGlobal.construirGrade(jogo);
//
//    // (3) Substitua as chamadas originais:
//    processarColisoesTiros_Grade(jogo, gradeGlobal);
//    processarColisaoZumbiJogador_Grade(jogo, gradeGlobal, deltaTime);
//
// ===========================================================================

// ---------------------------------------------------------------------------
// Auxiliar — sem alteração de lógica, só replicado aqui para que
// SpatialGrid.h seja auto-contido sem incluir GameLogic.h inteiro.
// ---------------------------------------------------------------------------
inline bool jaAcertouEsteZumbi_sg(const int* arr, int qtd, int idx) {
    for (int i = 0; i < qtd; ++i) {
        if (arr[i] == idx) return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// processarColisoesTiros_Grade
//   Versão acelerada de processarColisoesTiros().
//   Para cada projétil ativo, consulta apenas as ~9 células ao redor dele
//   em vez de iterar por toda a horda. Toda a lógica de dano, perfuração,
//   efeitos especiais e morte é preservada bit-a-bit.
// ---------------------------------------------------------------------------
inline void processarColisoesTiros_Grade(EstadoDoJogo& jogo,
                                          GradeEspacial& grade) {
    const int MAX_ZUMBIS_POR_TIRO = 32;

    // Vetor de candidatos reutilizado para cada tiro (evita realocações)
    std::vector<int> candidatos;
    candidatos.reserve(64);

    for (int i = 0; i < (int)jogo.tirosNaTela.size(); ++i) {
        Projetil& tiro = jogo.tirosNaTela[i];
        if (!tiro.ativo) continue;

        // Consulta apenas as células vizinhas ao projétil
        grade.obterInimigosVizinhos(tiro.posicao.x, tiro.posicao.z, candidatos);

        // Array local de índices já acertados por ESTE tiro NESTE frame
        int jaAcertados[MAX_ZUMBIS_POR_TIRO];
        int qtdAcertados = 0;

        for (int k = 0; k < (int)candidatos.size(); ++k) {
            int j = candidatos[k];
            Zumbi& z = jogo.horda[j];
            if (!z.vivo) continue;
            if (jaAcertouEsteZumbi_sg(jaAcertados, qtdAcertados, j)) continue;

            if (verificarColisao(tiro.posicao, tiro.raioColisao,
                                 z.posicao,    z.raioColisao)) {
                if (qtdAcertados < MAX_ZUMBIS_POR_TIRO)
                    jaAcertados[qtdAcertados++] = j;

                int danoEfetivo = tiro.dano;
                if (jogo.stand.emSobrecarga)
                    danoEfetivo = (int)(tiro.dano * FATOR_DANO_SOBRECARGA);

                z.vida -= danoEfetivo;

                // NOVO: cria o número de dano flutuante no ponto de impacto
                criarFloatingDamage(jogo, z.posicao, danoEfetivo);

                // Efeitos especiais — lógica idêntica à original
                if (jogo.stand.tipoDisparoAtual == DISPARO_CADENCIA_VIDA) {
                    if ((rand() % 100) < 30) {
                        if (jogo.protagonista.hp < jogo.protagonista.hpMaximo)
                            jogo.protagonista.hp++;
                    }
                } else if (jogo.stand.tipoDisparoAtual == DISPARO_TENSAO_VIDA) {
                    if (!jogo.stand.emSobrecarga) {
                        jogo.stand.tensaoAtual -= 3.0f;
                        if (jogo.stand.tensaoAtual < 0.0f)
                            jogo.stand.tensaoAtual = 0.0f;
                    }
                }

                if (z.vida <= 0) {
                    // NOVO: explosão de partículas na cor do inimigo morto
                    float corR, corG, corB;
                    obterCorBaseZumbi(z.tipo, corR, corG, corB);
                    criarParticulasMorte(jogo, z.posicao, corR, corG, corB);

                    processarMorteZumbi(jogo, z);
                }

                if (tiro.perfuracaoRestante <= 0) {
                    tiro.ativo = false;
                    break;
                } else {
                    tiro.perfuracaoRestante--;
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// processarColisaoZumbiJogador_Grade
//   Versão acelerada de processarColisaoZumbiJogador().
//   Consulta apenas as células vizinhas à posição do jogador.
//   Toda a lógica de iframe e morte é preservada.
// ---------------------------------------------------------------------------
inline void processarColisaoZumbiJogador_Grade(EstadoDoJogo& jogo,
                                                GradeEspacial& grade,
                                                float deltaTime) {
    Jogador& jog = jogo.protagonista;
    if (!jog.vivo) return;

    if (jog.temporizadorIframe > 0.0f) {
        jog.temporizadorIframe -= deltaTime;
        if (jog.temporizadorIframe < 0.0f) jog.temporizadorIframe = 0.0f;
    }

    std::vector<int> candidatos;
    candidatos.reserve(32);
    grade.obterInimigosVizinhos(jog.posicao.x, jog.posicao.z, candidatos);

    for (int k = 0; k < (int)candidatos.size(); ++k) {
        int i = candidatos[k];
        Zumbi& z = jogo.horda[i];
        if (!z.vivo) continue;

        if (verificarColisao(jog.posicao, jog.raioColisao,
                             z.posicao,   z.raioColisao)) {
            if (jog.temporizadorIframe <= 0.0f) {
                jog.hp -= 1;
                jog.temporizadorIframe = jog.duracaoIframe;
                if (jog.hp <= 0) jog.vivo = false;
            }
        }
    }
}

// ===========================================================================
//  GUIA DE INTEGRAÇÃO — resumo das 4 alterações em Main.cpp
//
//  1. No topo de Main.cpp, inclua após GameLogic.h:
//       #include "SpatialGrid.h"
//
//  2. Declare a grade como variável global (junto a "EstadoDoJogo jogo;"):
//       GradeEspacial gradeEspacial;
//
//  3. Dentro de timer(), imediatamente antes das chamadas de colisão,
//     adicione a construção da grade:
//       gradeEspacial.construirGrade(jogo);
//
//  4. Substitua as duas chamadas originais:
//       // Antes:
//       processarColisoesTiros(jogo);
//       processarColisaoZumbiJogador(jogo, deltaTime);
//
//       // Depois:
//       processarColisoesTiros_Grade(jogo, gradeEspacial);
//       processarColisaoZumbiJogador_Grade(jogo, gradeEspacial, deltaTime);
//
//  Nenhuma outra alteração é necessária em qualquer outro arquivo.
// ===========================================================================

#endif // SPATIAL_GRID_H