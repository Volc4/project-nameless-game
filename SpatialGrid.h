// ===========================================================================
//  SpatialGrid.h — Grade espacial para aceleração de colisões
//
//  Divide a arena em células de TAMANHO_CELULA x TAMANHO_CELULA.
//  Cada frame: construirGrade() indexa zumbis vivos; obterInimigosVizinhos()
//  retorna apenas os índices das 9 células vizinhas ao ponto consultado.
// ===========================================================================

#ifndef SPATIAL_GRID_UPDATED_H
#define SPATIAL_GRID_UPDATED_H

#include "Entities.h"
#include "SkillManager.h"   // ← único include novo
#include <vector>
#include <cmath>

const float TAMANHO_CELULA = 4.0f;
const float LIMITE_GRADE   = 180.0f;
const int   NUM_CELULAS    = 90;

struct CelulaEspacial {
    std::vector<int> inimigos;  // índices em EstadoDoJogo::horda
};

struct GradeEspacial {
    CelulaEspacial celulas[NUM_CELULAS][NUM_CELULAS];

    // Clamp de coordenada de mundo para índice de célula [0, NUM_CELULAS).
    int coordParaCelula(float coord) const {
        float normalizado = (coord + LIMITE_GRADE) / TAMANHO_CELULA;
        int   idx         = (int)floorf(normalizado);
        if (idx < 0)             idx = 0;
        if (idx >= NUM_CELULAS)  idx = NUM_CELULAS - 1;
        return idx;
    }

    // Esvazia todos os buckets — chamada antes de construirGrade() a cada frame.
    void limparGrade() {
        for (int l = 0; l < NUM_CELULAS; ++l)
            for (int c = 0; c < NUM_CELULAS; ++c) {
                celulas[l][c].inimigos.clear();
            }
    }

    // Reconstrói a grade do zero com os zumbis vivos do frame atual.
    void construirGrade(const EstadoDoJogo& jogo) {
        limparGrade();
        for (int i = 0; i < (int)jogo.horda.size(); ++i) {
            const Zumbi& z = jogo.horda[i];
            if (!z.vivo) continue;
            int col = coordParaCelula(z.posicao.x);
            int lin = coordParaCelula(z.posicao.z);
            celulas[lin][col].inimigos.push_back(i);
        }
    }

    // Coleta índices das 9 células ao redor de (posX, posZ) sem testar distância real.
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
                for (int k = 0; k < (int)bucket.size(); ++k)
                    saida.push_back(bucket[k]);
            }
        }
    }
};

// True se idx já está no array arr[0..qtd-1] — evita acertar o mesmo zumbi duas vezes por frame.
inline bool jaAcertouEsteZumbi_sg(const int* arr, int qtd, int idx) {
    for (int i = 0; i < qtd; ++i)
        if (arr[i] == idx) return true;
    return false;
}

// Declaração; definida em Main.cpp porque acessa g_jogoTerminado (global de TU).
void processarColisaoZumbiJogador_Grade(EstadoDoJogo& jogo,
                                        GradeEspacial& grade,
                                        float deltaTime);

#endif // SPATIAL_GRID_UPDATED_H
