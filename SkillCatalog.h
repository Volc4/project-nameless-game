#ifndef SKILL_CATALOG_H
#define SKILL_CATALOG_H

// ===========================================================================
//  SkillCatalog.h — Desbloqueios de capacidade e aplicação de upgrades
//
//  Traduz SistemaUpgrades → campos de SkillData (dano, cadência, perfuração,
//  quantidade). Não conhece skills concretas — opera só sobre SkillData/Entities.
// ===========================================================================

#include "SkillTypes.h"
#include "Entities.h"
#include <vector>
#include <cstring>

// No-op: desbloqueios são imediatos neste build; reservado para expansão futura.
inline void liberarPorUpgrade(EstadoDesbloqueio& d,
                               TipoUpgrade tipo, int /*nivel*/) {
    (void)d;
    (void)tipo;
}

// Aplica SistemaUpgrades a uma cópia de SkillData e a retorna modificada.
// VELOCIDADE/VIDA/TENSAO_UP não alteram SkillData — afetam o Jogador diretamente.
inline SkillData aplicarUpgradesNaSkill(SkillData s,
                                         const SistemaUpgrades& upgrades,
                                         const EstadoDesbloqueio& /*d*/) {
    int nivelDano = upgrades.niveis[DANO];
    if (nivelDano > 0) {
        float fatorDano    = fatorDanoDoNivel(nivelDano);
        float fatorTamanho = 1.0f + 0.20f * (float)nivelDano;
        s.forma.raioColisao *= fatorTamanho;
        for (int i = 0; i < s.numEfeitos; ++i) {
            if (s.efeitos[i].tipo == EFE_DAMAGE ||
                s.efeitos[i].tipo == EFE_SHOCK) {
                s.efeitos[i].valor = (int)((float)s.efeitos[i].valor * fatorDano + 0.5f);
            }
        }
    }

    int nivelCadencia = upgrades.niveis[CADENCIA];
    if (nivelCadencia > 0) {
        s.cooldown *= fatorCadenciaDoNivel(nivelCadencia);
        float fatorTensao = 1.0f - 0.10f * (float)nivelCadencia;
        if (fatorTensao < 0.1f) fatorTensao = 0.1f;
        s.custoTensao *= fatorTensao;
        float fatorDano = 1.0f - 0.10f * (float)nivelCadencia;
        for (int i = 0; i < s.numEfeitos; ++i) {
            if (s.efeitos[i].tipo == EFE_DAMAGE || s.efeitos[i].tipo == EFE_SHOCK) {
                s.efeitos[i].valor = (int)((float)s.efeitos[i].valor * fatorDano + 0.5f);
                if (s.efeitos[i].valor < 1) s.efeitos[i].valor = 1;
            }
        }
    }

    int nivelPerf = upgrades.niveis[PERFURACAO];
    if (nivelPerf > 0) {
        s.forma.perfuracao += perfuracaoDoNivel(nivelPerf);
        s.forma.alcanceMax += 10.0f * (float)nivelPerf;
    }

    int nivelQtd = upgrades.niveis[QUANTIDADE];
    if (nivelQtd > 0) {
        s.forma.quantidade   = quantidadeDoNivel(nivelQtd);
        s.forma.spreadAngulo = 0.20f;  // spread fixo por bala; cone abre com a quantidade
    }

    return s;
}

#endif // SKILL_CATALOG_H
