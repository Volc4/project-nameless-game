#ifndef SKILL_VALIDATOR_H
#define SKILL_VALIDATOR_H

// ===========================================================================
//  SkillValidator.h — Validação e compatibilidade de builds
//
//  Impede builds incoerentes ANTES de chegarem ao executor: falha de dados
//  nunca vira crash de runtime. Ver documento de arquitetura, Parte 6.
//
//  Conteúdo:
//   - EstadoDesbloqueio helpers (bitmasks de capacidade).
//   - Matriz de compatibilidade Forma × Movimento.
//   - validarSkill(): aplica matriz + regras de recursão + desbloqueio.
//   - rebaixarParaCompativel(): fallback garantido (tiro base sempre válido).
//
//  C++98: arrays estáticos const, sem STL associativa.
// ===========================================================================

#include "SkillTypes.h"
#include "Entities.h"   // EstadoDesbloqueio

// ---------------------------------------------------------------------------
// Bitmask helpers de desbloqueio (EstadoDesbloqueio está em Entities.h).
// ---------------------------------------------------------------------------
inline void inicializarDesbloqueio(EstadoDesbloqueio& d) {
    // Base sempre disponível: o tiro simples (Projectile + Linear + Stand + Damage).
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

// ===========================================================================
//  MATRIZ DE COMPATIBILIDADE FORMA × MOVIMENTO
//
//  1 = combinação permitida, 0 = bloqueada.
//  Linhas indexadas por FormaType, colunas por MovimentoType — NA ORDEM REAL
//  dos enums em SkillTypes.h (corrigida).
//  Ordem das colunas: LIN HOM ORB BOO BOU SPI FAL TEL RND STA
//
//  ODR: declarada extern aqui; definição ÚNICA em SkillTables.cpp.
// ===========================================================================
extern const bool COMPAT_FORMA_MOV[FORMA_TOTAL][MOV_TOTAL];

inline bool formaMovCompat(FormaType f, MovimentoType m) {
    return COMPAT_FORMA_MOV[f][m];
}

// ---------------------------------------------------------------------------
// Resultado de validação.
// ---------------------------------------------------------------------------
enum ResultadoValidacao {
    VALID_OK = 0,
    VALID_FORMA_MOV_INCOMPAT,
    VALID_RECURSAO_SEM_LIMITE,
    VALID_NAO_LIBERADO,
    VALID_EFEITO_DEMAIS
};

// ---------------------------------------------------------------------------
// validarSkill — checa a build contra matriz, recursão e desbloqueio.
//   Não muta nada; só diagnostica. O fallback é responsabilidade de
//   rebaixarParaCompativel(), chamado pelo Build Assembler em caso de erro.
// ---------------------------------------------------------------------------
inline ResultadoValidacao validarSkill(const SkillData& s,
                                       const EstadoDesbloqueio& d) {
    // R1: Forma × Movimento.
    if (!formaMovCompat(s.forma.tipo, s.movimento.tipo))
        return VALID_FORMA_MOV_INCOMPAT;

    // R2: recursão (SpawnSkill/ChainExplosion/Prism) precisa de limite > 0 e
    //     não pode auto-referenciar a própria skill sem limite.
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

    // R3: capacidades têm de estar liberadas pelos upgrades adquiridos.
    if (!estaLiberado(d.formasLiberadas,     s.forma.tipo))     return VALID_NAO_LIBERADO;
    if (!estaLiberado(d.movimentosLiberados, s.movimento.tipo)) return VALID_NAO_LIBERADO;
    if (!estaLiberado(d.origensLiberadas,    s.origem.tipo))    return VALID_NAO_LIBERADO;

    // R4: limite de efeitos.
    if (s.numEfeitos > MAX_EFEITOS_POR_SKILL) return VALID_EFEITO_DEMAIS;

    return VALID_OK;
}

// ---------------------------------------------------------------------------
// primeiroMovimentoCompativelLiberado — para fallback de movimento.
//   Retorna o primeiro MovimentoType que é compatível com a forma E está
//   liberado. Se nenhum, retorna MOV_STATIONARY como último recurso seguro
//   (toda forma estática aceita Stationary; Projectile cai para Linear via R-final).
// ---------------------------------------------------------------------------
inline MovimentoType primeiroMovimentoCompativelLiberado(FormaType f,
                                                         const EstadoDesbloqueio& d) {
    for (int m = 0; m < MOV_TOTAL; ++m) {
        if (COMPAT_FORMA_MOV[f][m] && estaLiberado(d.movimentosLiberados, m))
            return (MovimentoType)m;
    }
    return MOV_STATIONARY;
}

// ---------------------------------------------------------------------------
// rebaixarParaCompativel — fallback em escada (Parte 6.4):
//   1) tenta consertar o Movimento mantendo a Forma;
//   2) se a Forma não está liberada, rebaixa para Projectile;
//   3) garante o tiro base { Projectile, Linear, Stand, Damage } — sempre válido.
//   O jogo NUNCA fica sem skill disparável.
// ---------------------------------------------------------------------------
inline void rebaixarParaCompativel(SkillData& s, const EstadoDesbloqueio& d) {
    // (2) Forma não liberada → Projectile (sempre liberado).
    if (!estaLiberado(d.formasLiberadas, s.forma.tipo)) {
        s.forma.tipo = FORMA_PROJECTILE;
    }

    // (1) Movimento incompatível/não liberado → primeiro compatível liberado.
    if (!formaMovCompat(s.forma.tipo, s.movimento.tipo) ||
        !estaLiberado(d.movimentosLiberados, s.movimento.tipo)) {
        s.movimento.tipo = primeiroMovimentoCompativelLiberado(s.forma.tipo, d);
    }

    // Origem não liberada → Stand (sempre liberado).
    if (!estaLiberado(d.origensLiberadas, s.origem.tipo)) {
        s.origem.tipo = ORIG_STAND;
    }

    // (3) Rede de segurança final: se ainda inconsistente, força o tiro base.
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
