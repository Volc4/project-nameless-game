// ===========================================================================
//  SpatialGrid_updated.h — Versão com suporte ao SkillManager
//
//  ÚNICA MUDANÇA em relação ao original:
//    processarColisoesTiros_Grade() recebe `const SkillManager& skills`
//    e lê flags de efeitoCuraVida / efeitoDrenarTensao em vez de comparar
//    jogo.stand.tipoDisparoAtual.
//
//  Todo o resto — grade, células, construção, consulta — é idêntico.
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
    std::vector<int> inimigos;
    // (campo projeteis REMOVIDO — instancias vivem no pool do SkillManager)
};

struct GradeEspacial {
    CelulaEspacial celulas[NUM_CELULAS][NUM_CELULAS];

    int coordParaCelula(float coord) const {
        float normalizado = (coord + LIMITE_GRADE) / TAMANHO_CELULA;
        int   idx         = (int)floorf(normalizado);
        if (idx < 0)             idx = 0;
        if (idx >= NUM_CELULAS)  idx = NUM_CELULAS - 1;
        return idx;
    }

    void limparGrade() {
        for (int l = 0; l < NUM_CELULAS; ++l)
            for (int c = 0; c < NUM_CELULAS; ++c) {
                celulas[l][c].inimigos.clear();
            }
    }

    void construirGrade(const EstadoDoJogo& jogo) {
        limparGrade();
        for (int i = 0; i < (int)jogo.horda.size(); ++i) {
            const Zumbi& z = jogo.horda[i];
            if (!z.vivo) continue;
            int col = coordParaCelula(z.posicao.x);
            int lin = coordParaCelula(z.posicao.z);
            celulas[lin][col].inimigos.push_back(i);
        }
        // (indexação de projéteis REMOVIDA — o motor data-driven consulta a grade
        //  apenas por inimigos; as instâncias RuntimeSkill vivem no pool do
        //  SkillManager e são iteradas diretamente em processarColisoesSkills_Grade.)
    }

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

// ---------------------------------------------------------------------------
// Auxiliar interno
// ---------------------------------------------------------------------------
inline bool jaAcertouEsteZumbi_sg(const int* arr, int qtd, int idx) {
    for (int i = 0; i < qtd; ++i)
        if (arr[i] == idx) return true;
    return false;
}

// (processarColisoesTiros_Grade legado REMOVIDO — substituido por
//  processarColisoesSkills_Grade em SkillManager.h, motor data-driven.)

// ---------------------------------------------------------------------------
// processarColisaoZumbiJogador_Grade — SEM MUDANÇAS
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

#endif // SPATIAL_GRID_UPDATED_H
