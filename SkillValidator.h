#ifndef SKILL_VALIDATOR_H
#define SKILL_VALIDATOR_H

// ===========================================================================
//  SkillValidator.h — Validação de builds antes de chegarem ao executor.
//
//  validarSkill() checa matriz Forma×Movimento, recursão e desbloqueios.
//  rebaixarParaCompativel() garante fallback ao tiro base quando inválido.
// ===========================================================================

#include "SkillTypes.h"
#include "Entities.h"   // EstadoDesbloqueio

// Libera apenas o tiro base (Projectile+Linear+Stand+Damage); upgrades expandem via liberar().
inline void inicializarDesbloqueio(EstadoDesbloqueio& d) {
    d.formasLiberadas     = (1UL << FORMA_PROJECTILE);
    d.movimentosLiberados = (1UL << MOV_LINEAR);
    d.origensLiberadas    = (1UL << ORIG_STAND);
    d.efeitosLiberados    = (1UL << EFE_DAMAGE);
}

inline bool estaLiberado(unsigned long mask, int tipo) {
    return (mask & (1UL << tipo)) != 0;
}

inline void liberar(unsigned long& mask, int tipo) {
    mask |= (1UL << tipo);
}

// Matriz Forma×Movimento (1=permitida). Linhas=FormaType, colunas=MovimentoType.
// Colunas: LIN HOM ORB BOO BOU SPI FAL TEL RND STA — mesma ordem que SkillTypes.h.
// Definida em SkillTables.h (ODR única).
extern const bool COMPAT_FORMA_MOV[FORMA_TOTAL][MOV_TOTAL];

inline bool formaMovCompat(FormaType f, MovimentoType m) {
    return COMPAT_FORMA_MOV[f][m];
}

enum ResultadoValidacao {
    VALID_OK = 0,
    VALID_FORMA_MOV_INCOMPAT,
    VALID_RECURSAO_SEM_LIMITE,
    VALID_NAO_LIBERADO,
    VALID_EFEITO_DEMAIS
};

// Diagnóstico puro (não muta); rebaixarParaCompativel() aplica o fallback.
inline ResultadoValidacao validarSkill(const SkillData& s,
                                       const EstadoDesbloqueio& d) {
    if (!formaMovCompat(s.forma.tipo, s.movimento.tipo))
        return VALID_FORMA_MOV_INCOMPAT;

    // SpawnSkill/ChainExplosion auto-referenciando sem limite geram loop infinito.
    for (int i = 0; i < s.numEfeitos; ++i) {
        const EfeitoData& e = s.efeitos[i];
        if (e.tipo == EFE_SPAWNSKILL || e.tipo == EFE_CHAINEXPLOSION) {
            if (e.idSkillSpawn == s.id) return VALID_RECURSAO_SEM_LIMITE;
            if (e.tipo == EFE_CHAINEXPLOSION && e.propagacaoMax <= 0)
                return VALID_RECURSAO_SEM_LIMITE;
        }
    }
    if (s.forma.tipo == FORMA_PRISM && s.forma.geracoesMax <= 0)
        return VALID_RECURSAO_SEM_LIMITE;

    if (!estaLiberado(d.formasLiberadas,     s.forma.tipo))     return VALID_NAO_LIBERADO;
    if (!estaLiberado(d.movimentosLiberados, s.movimento.tipo)) return VALID_NAO_LIBERADO;
    if (!estaLiberado(d.origensLiberadas,    s.origem.tipo))    return VALID_NAO_LIBERADO;
    if (s.numEfeitos > MAX_EFEITOS_POR_SKILL) return VALID_EFEITO_DEMAIS;

    return VALID_OK;
}

// Primeiro movimento compatível+liberado para a forma dada; MOV_STATIONARY se nenhum.
inline MovimentoType primeiroMovimentoCompativelLiberado(FormaType f,
                                                         const EstadoDesbloqueio& d) {
    for (int m = 0; m < MOV_TOTAL; ++m) {
        if (COMPAT_FORMA_MOV[f][m] && estaLiberado(d.movimentosLiberados, m))
            return (MovimentoType)m;
    }
    return MOV_STATIONARY;
}

// Fallback em escada: corrige Movimento → Forma → tiro base garantido (sem crash).
inline void rebaixarParaCompativel(SkillData& s, const EstadoDesbloqueio& d) {
    if (!estaLiberado(d.formasLiberadas, s.forma.tipo))
        s.forma.tipo = FORMA_PROJECTILE;

    if (!formaMovCompat(s.forma.tipo, s.movimento.tipo) ||
        !estaLiberado(d.movimentosLiberados, s.movimento.tipo))
        s.movimento.tipo = primeiroMovimentoCompativelLiberado(s.forma.tipo, d);

    if (!estaLiberado(d.origensLiberadas, s.origem.tipo))
        s.origem.tipo = ORIG_STAND;

    // Rede de segurança final se ainda inconsistente.
    if (validarSkill(s, d) != VALID_OK) {
        s.forma.tipo     = FORMA_PROJECTILE;
        s.movimento.tipo = MOV_LINEAR;
        s.origem.tipo    = ORIG_STAND;
        if (s.numEfeitos <= 0) {
            s.efeitos[0].tipo  = EFE_DAMAGE;
            s.efeitos[0].valor = 1;
            s.numEfeitos       = 1;
        }
    }
}

#endif // SKILL_VALIDATOR_H
