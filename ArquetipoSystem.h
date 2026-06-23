#ifndef ARQUETIPO_SYSTEM_H
#define ARQUETIPO_SYSTEM_H

// ===========================================================================
//  ArquetipoSystem.h — Especialização irreversível da arma base "Disparo"
//
//  REGRAS (spec "Sistema de Evolução da Arma Base"):
//   - 4 atributos: Cadência, Dano, Perfuração, Quantidade (níveis 0..3).
//   - Quando DOIS atributos diferentes atingem nível 2, desbloqueia um
//     Arquétipo. A build fica PERMANENTEMENTE especializada nesses dois.
//   - Os outros dois atributos ficam BLOQUEADOS para o resto da partida.
//   - Quando os dois principais chegam a nível 3 → Evolução Final.
//
//  Este módulo é puro estado + regras (sem render/GL). O menu de level-up
//  consulta atributoBloqueado() para filtrar opções; aplicarEscolhaLevelUp()
//  chama verificarEspecializacao() após cada upgrade.
//
//  C++98: enums simples, structs POD, sem STL associativa.
// ===========================================================================

#include "Entities.h"   // TipoUpgrade, AtributoArma, Arquetipo, EstadoArquetipo

// ---------------------------------------------------------------------------
// Mapeamento AtributoArma <-> TipoUpgrade.
//   Os 4 atributos da arma são um subconjunto de TipoUpgrade.
// ---------------------------------------------------------------------------
inline TipoUpgrade atributoParaUpgrade(int atr) {
    switch (atr) {
        case ATR_CADENCIA:   return CADENCIA;
        case ATR_DANO:       return DANO;
        case ATR_PERFURACAO: return PERFURACAO;
        case ATR_QUANTIDADE: return QUANTIDADE;
        default:             return CADENCIA;
    }
}

inline int upgradeParaAtributo(TipoUpgrade up) {
    switch (up) {
        case CADENCIA:   return ATR_CADENCIA;
        case DANO:       return ATR_DANO;
        case PERFURACAO: return ATR_PERFURACAO;
        case QUANTIDADE: return ATR_QUANTIDADE;
        default:         return -1;   // não é atributo de arma
    }
}

// ---------------------------------------------------------------------------
// nivelAtributo — nível atual (0..3) de um AtributoArma, lendo SistemaUpgrades.
// ---------------------------------------------------------------------------
inline int nivelAtributo(const SistemaUpgrades& up, int atr) {
    return up.niveis[atributoParaUpgrade(atr)];
}

// ---------------------------------------------------------------------------
// arquetipoDe — dado o par de atributos (A,B), retorna o Arquétipo.
//   A ordem de A,B não importa (normaliza internamente).
// ---------------------------------------------------------------------------
inline Arquetipo arquetipoDe(int a, int b) {
    // normaliza para (menor, maior)
    int lo = (a < b) ? a : b;
    int hi = (a < b) ? b : a;

    if (lo == ATR_CADENCIA && hi == ATR_DANO)       return ARQ_METRALHADORA_PESADA;
    if (lo == ATR_CADENCIA && hi == ATR_PERFURACAO) return ARQ_CANHAO_ROTATIVO;
    if (lo == ATR_CADENCIA && hi == ATR_QUANTIDADE) return ARQ_METRALHADORA_LEVE;
    if (lo == ATR_DANO     && hi == ATR_PERFURACAO) return ARQ_RIFLE_LASER;
    if (lo == ATR_DANO     && hi == ATR_QUANTIDADE) return ARQ_ESPINGARDA_TATICA;
    if (lo == ATR_PERFURACAO && hi == ATR_QUANTIDADE) return ARQ_CANHAO_FRAGMENTACAO;
    return ARQ_NENHUM;
}

// ---------------------------------------------------------------------------
// inicializarArquetipo — estado limpo no começo da partida.
// ---------------------------------------------------------------------------
inline void inicializarArquetipo(EstadoArquetipo& e) {
    e.arquetipo     = ARQ_NENHUM;
    e.especializado = false;
    e.atributoA     = -1;
    e.atributoB     = -1;
    e.evolucaoFinal = false;
}

// ---------------------------------------------------------------------------
// atributoBloqueado — true se este AtributoArma NÃO pode mais ser melhorado.
//   Antes da especialização: nada bloqueado.
//   Depois: tudo que não for atributoA/atributoB fica bloqueado.
// ---------------------------------------------------------------------------
inline bool atributoBloqueado(const EstadoArquetipo& e, int atr) {
    if (!e.especializado) return false;
    return (atr != e.atributoA && atr != e.atributoB);
}

// ---------------------------------------------------------------------------
// upgradeBloqueado — mesma checagem, porém recebendo um TipoUpgrade.
//   Upgrades que não são atributos de arma (TENSAO_UP/VELOCIDADE/VIDA) nunca
//   são bloqueados por arquétipo.
// ---------------------------------------------------------------------------
inline bool upgradeBloqueado(const EstadoArquetipo& e, TipoUpgrade up) {
    int atr = upgradeParaAtributo(up);
    if (atr < 0) return false;          // não é atributo de arma
    return atributoBloqueado(e, atr);
}

// ---------------------------------------------------------------------------
// verificarEspecializacao — chamado após CADA upgrade de atributo de arma.
//   Detecta quando dois atributos atingem nível 2 e trava o arquétipo.
//   Idempotente: uma vez especializado, não muda mais (irreversível).
//   Também marca a Evolução Final quando os dois principais chegam a nível 3.
// ---------------------------------------------------------------------------
inline void verificarEspecializacao(EstadoArquetipo& e,
                                    const SistemaUpgrades& up) {
    if (!e.especializado) {
        // procura os DOIS primeiros atributos com nível >= 2
        int primeiro = -1, segundo = -1;
        for (int a = 0; a < TOTAL_ATRIBUTOS_ARMA; ++a) {
            if (nivelAtributo(up, a) >= 2) {
                if (primeiro < 0)      primeiro = a;
                else if (segundo < 0) { segundo = a; break; }
            }
        }
        if (primeiro >= 0 && segundo >= 0) {
            e.atributoA     = primeiro;
            e.atributoB     = segundo;
            e.arquetipo     = arquetipoDe(primeiro, segundo);
            e.especializado = true;
        }
    }

    // Evolução final: ambos os principais em nível 3.
    if (e.especializado && !e.evolucaoFinal) {
        if (nivelAtributo(up, e.atributoA) >= 3 &&
            nivelAtributo(up, e.atributoB) >= 3) {
            e.evolucaoFinal = true;
        }
    }
}

// ---------------------------------------------------------------------------
// podeEvoluirAtributo — regra de elegibilidade p/ o menu de level-up:
//   um atributo de arma só aparece se NÃO está bloqueado e ainda < nível 3.
// ---------------------------------------------------------------------------
inline bool podeEvoluirAtributo(const EstadoArquetipo& e,
                                const SistemaUpgrades& up, int atr) {
    if (atributoBloqueado(e, atr)) return false;
    return nivelAtributo(up, atr) < 3;
}

// ---------------------------------------------------------------------------
// Nomes legíveis (HUD / menu). Sem alocação; retorna literal estático.
// ---------------------------------------------------------------------------
inline const char* nomeArquetipo(Arquetipo a) {
    switch (a) {
        case ARQ_METRALHADORA_PESADA: return "Metralhadora Pesada";
        case ARQ_METRALHADORA_LEVE:   return "Metralhadora Leve";
        case ARQ_CANHAO_ROTATIVO:     return "Canhao Rotativo";
        case ARQ_RIFLE_LASER:         return "Rifle Laser";
        case ARQ_ESPINGARDA_TATICA:   return "Espingarda Tatica";
        case ARQ_CANHAO_FRAGMENTACAO: return "Canhao de Fragmentacao";
        default:                      return "Disparo";
    }
}

inline const char* nomeEvolucaoFinal(Arquetipo a) {
    switch (a) {
        case ARQ_METRALHADORA_PESADA: return "Devastadora";
        case ARQ_METRALHADORA_LEVE:   return "Tempestade de Chumbo";
        case ARQ_CANHAO_ROTATIVO:     return "Vulcan";
        case ARQ_RIFLE_LASER:         return "Lanca de Luz";
        case ARQ_ESPINGARDA_TATICA:   return "Juizo Final";
        case ARQ_CANHAO_FRAGMENTACAO: return "Tempestade de Estilhacos";
        default:                      return "Disparo";
    }
}

// ---------------------------------------------------------------------------
// nomeAtivoArma — nome a exibir: evolução final tem precedência sobre arquétipo.
// ---------------------------------------------------------------------------
inline const char* nomeAtivoArma(const EstadoArquetipo& e) {
    if (e.evolucaoFinal) return nomeEvolucaoFinal(e.arquetipo);
    if (e.especializado) return nomeArquetipo(e.arquetipo);
    return "Disparo";
}

#endif // ARQUETIPO_SYSTEM_H
