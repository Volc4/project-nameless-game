#ifndef SKILL_CATALOG_H
#define SKILL_CATALOG_H

// ===========================================================================
//  SkillCatalog.h — Desbloqueios de capacidade e aplicação de upgrades
//
//  Responsabilidades:
//    - EstadoDesbloqueio: inicialização e preenchimento por upgrade.
//    - aplicarUpgradesNaSkill(): traduz SistemaUpgrades → modificações
//      em SkillData (dano, cadência, perfuração, etc.).
//    - contarAtivos(): conta RuntimeSkill ativos num pool.
//    - zerarRuntime(): zera um RuntimeSkill.
//
//  NÃO conhece skills concretas — apenas os campos de SkillData e os
//  valores dos TipoUpgrade definidos em Entities.h.
// ===========================================================================

#include "SkillTypes.h"
#include "Entities.h"
#include <vector>
#include <cstring>

// ---------------------------------------------------------------------------
// Bitmasks de capacidade (para EstadoDesbloqueio)
// Cada bit representa uma FormaType, MovimentoType, OrigemType ou EfeitoType
// liberada. Por ora todas as capacidades usadas pelos builds de SkillRegistry.h
// estão sempre liberadas — o sistema de bitmask está preparado para Fase 7+.
// ---------------------------------------------------------------------------

/*inline void inicializarDesbloqueio(EstadoDesbloqueio& d) {
    // Libera tudo por padrão (bitmask com todos os bits 1)
    d.formasLiberadas     = ~0UL;
    d.movimentosLiberados = ~0UL;
    d.origensLiberadas    = ~0UL;
    d.efeitosLiberados    = ~0UL;
}*/

inline void liberarPorUpgrade(EstadoDesbloqueio& d,
                               TipoUpgrade tipo, int /*nivel*/) {
    // Placeholder: todos os desbloqueios são imediatos na Fase 6.
    // Na Fase 7 este switch condicionará quais FormaType/EfeitoType ficam
    // disponíveis conforme o nível de upgrade.
    (void)d;
    (void)tipo;
}

// ---------------------------------------------------------------------------
// aplicarUpgradesNaSkill — aplica os upgrades clássicos (SistemaUpgrades)
//   a uma SkillData, devolvendo a versão modificada.
//
//   Tabela de conversão (espelha GameLogic.h legado):
//     DANO        → +10% dano por nível (efeitos EFE_DAMAGE)
//     CADENCIA    → −5% custo de tensão por nível
//     PERFURACAO  → +1 perfuração por nível
//     TENSAO_UP   → −10% custo de tensão por nível (cumulativo com CADENCIA)
//     VELOCIDADE  → sem efeito direto na SkillData (afeta o Jogador)
//     VIDA        → sem efeito direto na SkillData (afeta o Jogador)
// ---------------------------------------------------------------------------
inline SkillData aplicarUpgradesNaSkill(SkillData s,
                                         const SistemaUpgrades& upgrades,
                                         const EstadoDesbloqueio& /*d*/) {
    // DANO: +10% por nível
    int nivelDano = upgrades.niveis[DANO];
    if (nivelDano > 0) {
        float fator = 1.0f + 0.10f * nivelDano;
        for (int i = 0; i < s.numEfeitos; ++i) {
            if (s.efeitos[i].tipo == EFE_DAMAGE ||
                s.efeitos[i].tipo == EFE_SHOCK) {
                s.efeitos[i].valor = (int)(s.efeitos[i].valor * fator + 0.5f);
            }
        }
    }

    // CADENCIA: −5% custo tensão por nível (torna o disparo mais barato)
    int nivelCadencia = upgrades.niveis[CADENCIA];
    if (nivelCadencia > 0) {
        float fator = 1.0f - 0.05f * nivelCadencia;
        if (fator < 0.1f) fator = 0.1f;
        s.custoTensao *= fator;
        if (s.custoTensao < 1.0f) s.custoTensao = 1.0f;
    }

    // PERFURACAO: +1 por nível
    int nivelPerf = upgrades.niveis[PERFURACAO];
    if (nivelPerf > 0) {
        s.forma.perfuracao += nivelPerf;
    }

    // TENSAO_UP: −10% custo tensão por nível
    int nivelTensao = upgrades.niveis[TENSAO_UP];
    if (nivelTensao > 0) {
        float fator = 1.0f - 0.10f * nivelTensao;
        if (fator < 0.05f) fator = 0.05f;
        s.custoTensao *= fator;
        if (s.custoTensao < 1.0f) s.custoTensao = 1.0f;
    }

    return s;
}

// ---------------------------------------------------------------------------
// Utilitários de pool
// ---------------------------------------------------------------------------

/* Conta quantas instâncias ativas existem num pool.
inline int contarAtivos(const std::vector<RuntimeSkill>& pool) {
    int c = 0;
    for (int i = 0; i < (int)pool.size(); ++i)
        if (pool[i].ativo) c++;
    return c;
}*/

/* Zera todos os campos de um RuntimeSkill (memset seguro — POD puro).
inline void zerarRuntime(RuntimeSkill& r) {
    char* p = (char*)&r;
    for (int i = 0; i < (int)sizeof(RuntimeSkill); ++i) p[i] = 0;
}*/

/* Versão de jaAcertouEsteZumbi para uso fora de SpatialGrid.h
inline bool jaAcertouEsteZumbi_sg(const int* arr, int qtd, int idx) {
    for (int i = 0; i < qtd; ++i)
        if (arr[i] == idx) return true;
    return false;
}*/

#endif // SKILL_CATALOG_H
