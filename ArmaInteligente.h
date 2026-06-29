#ifndef ARMA_INTELIGENTE_H
#define ARMA_INTELIGENTE_H

// ===========================================================================
//  ArmaInteligente.h — Três armas automáticas guiadas (homing)
//
//  REGRAS:
//   - Apenas UMA arma inteligente equipada por partida (exclusividade).
//   - Cada arma tem 3 níveis; nível n = n projéteis guiados por disparo.
//   - Cooldown fixo: AI_COOLDOWN (1.5 s) em todas as armas.
//   - Armas são autônomas (cooldown > 0): NÃO precisam do clique do mouse.
//   - Isentas de upgrades clássicos de atributo (DANO/CADÊNCIA/PERFURAÇÃO/
//     QUANTIDADE) — progressão própria via nível. Ver ProgressionSystem.h.
//
//  TUNING — altere aqui para ajustar todas as armas de uma vez:
// ===========================================================================

#include "SkillTypes.h"
#include "SkillFactory.h"
#include "SkillRegistry.h"
#include "SkillInventory.h"
#include "Entities.h"

// ---------------------------------------------------------------------------
// Constantes de tuning — centralizadas para fácil ajuste
// ---------------------------------------------------------------------------
static const float AI_COOLDOWN       = 1.5f;   // intervalo de auto-disparo (s)
static const float AI_VELOCIDADE     = 22.0f;  // velocidade dos mísseis (u/s)
static const float AI_TAXA_CORRECAO  = 0.60f;  // fator de correção de rota homing [0,1]
static const float AI_RAIO_COLISAO   = 0.25f;  // raio de colisão dos projéteis

static const int   AI_DANO_VAMPIRICO = 1;      // dano por acerto do Vampírico (~1)
static const int   AI_DANO_GUIADO    = 2;      // dano por acerto do Guiado (~2)
static const int   AI_DANO_BOMBA     = 3;      // dano da explosão da Bomba (~3)
static const float AI_RAIO_EXPLOSAO  = 2.5f;   // raio AoE da Bomba Guiada

// ---------------------------------------------------------------------------
// Identificadores das três armas inteligentes
// ---------------------------------------------------------------------------
enum ArmaInteligenteId {
    ARMA_MISSIL_VAMPIRICO = 0,
    ARMA_MISSIL_GUIADO,
    ARMA_BOMBA_GUIADA,
    TOTAL_ARMAS_INTELIGENTES
};

#define NIVEL_MAX_ARMA_INTELIGENTE 3

// Chave textual na factory
inline const char* nomeArmaInteligente(int arma) {
    switch (arma) {
        case ARMA_MISSIL_VAMPIRICO: return "MissilVampirico";
        case ARMA_MISSIL_GUIADO:    return "MissilGuiado";
        case ARMA_BOMBA_GUIADA:     return "BombaGuiada";
        default:                    return "Disparo";
    }
}

// Nome exibido no menu de level-up
inline const char* tituloArmaInteligente(int arma) {
    switch (arma) {
        case ARMA_MISSIL_VAMPIRICO: return "Missil Vampirico";
        case ARMA_MISSIL_GUIADO:    return "Missil Guiado";
        case ARMA_BOMBA_GUIADA:     return "Bomba Guiada";
        default:                    return "Disparo";
    }
}

// ===========================================================================
//  BUILDS POR NÍVEL
//  nivel 1 → 1 projétil | nivel 2 → 2 | nivel 3 → 3 (forma.quantidade = nivel)
// ===========================================================================

// ---------------------------------------------------------------------------
// 1. MÍSSIL VAMPÍRICO (VERMELHO) — homing, alvo único, cura +1 ao acertar
//    forma.perfuracao = 0: some no primeiro zumbi e cura a protagonista.
// ---------------------------------------------------------------------------
inline SkillData buildMissilVampiricoNivel(int nivel) {
    SkillData s = buildBase();
    s.forma.tipo         = FORMA_PROJECTILE;
    s.forma.raioColisao  = AI_RAIO_COLISAO;
    s.forma.quantidade   = nivel;           // escala 1/2/3
    s.forma.perfuracao   = 0;              // alvo único: cura e some
    s.forma.corR = 1.0f; s.forma.corG = 0.1f; s.forma.corB = 0.1f;

    s.movimento.tipo        = MOV_HOMING;
    s.movimento.velocidade  = AI_VELOCIDADE;
    s.movimento.taxaCorrecao = AI_TAXA_CORRECAO;
    s.origem.tipo           = ORIG_NEARESTENEMY;
    s.cooldown              = AI_COOLDOWN;
    s.custoTensao           = 0.0f;

    s.efeitos[0].tipo  = EFE_DAMAGE; s.efeitos[0].valor = AI_DANO_VAMPIRICO;
    s.efeitos[1].tipo  = EFE_HEAL;   s.efeitos[1].valor = 1;
    s.numEfeitos = 2;
    return s;
}

// ---------------------------------------------------------------------------
// 2. MÍSSIL GUIADO (AZUL) — homing, perfura até 3 zumbis (perfuracao = 2)
//    Motor: hits = perfuracao + 1, portanto perfuracao=2 → 3 acertos.
// ---------------------------------------------------------------------------
inline SkillData buildMissilGuiadoNivel(int nivel) {
    SkillData s = buildBase();
    s.forma.tipo         = FORMA_PROJECTILE;
    s.forma.raioColisao  = AI_RAIO_COLISAO;
    s.forma.quantidade   = nivel;
    s.forma.perfuracao   = 2;              // atravessa 3 zumbis
    s.forma.corR = 0.2f; s.forma.corG = 0.4f; s.forma.corB = 1.0f;

    s.movimento.tipo        = MOV_HOMING;
    s.movimento.velocidade  = AI_VELOCIDADE;
    s.movimento.taxaCorrecao = AI_TAXA_CORRECAO;
    s.origem.tipo           = ORIG_NEARESTENEMY;
    s.cooldown              = AI_COOLDOWN;
    s.custoTensao           = 0.0f;

    s.efeitos[0].tipo  = EFE_DAMAGE; s.efeitos[0].valor = AI_DANO_GUIADO;
    s.numEfeitos = 1;
    return s;
}

// ---------------------------------------------------------------------------
// 3. BOMBA GUIADA (LARANJA) — homing, explode em área ao acertar
//    EFE_EXPLOSION: valor = dano, magnitude = raio AoE.
//    perfuracao = 0: para no primeiro zumbi e a explosão irradia.
// ---------------------------------------------------------------------------
inline SkillData buildBombaGuiadaNivel(int nivel) {
    SkillData s = buildBase();
    s.forma.tipo         = FORMA_PROJECTILE;
    s.forma.raioColisao  = AI_RAIO_COLISAO;
    s.forma.quantidade   = nivel;
    s.forma.perfuracao   = 0;
    s.forma.corR = 1.0f; s.forma.corG = 0.55f; s.forma.corB = 0.1f;

    s.movimento.tipo        = MOV_HOMING;
    s.movimento.velocidade  = AI_VELOCIDADE;
    s.movimento.taxaCorrecao = AI_TAXA_CORRECAO;
    s.origem.tipo           = ORIG_NEARESTENEMY;
    s.cooldown              = AI_COOLDOWN;
    s.custoTensao           = 0.0f;

    s.efeitos[0].tipo      = EFE_EXPLOSION;
    s.efeitos[0].valor     = AI_DANO_BOMBA;
    s.efeitos[0].magnitude = AI_RAIO_EXPLOSAO;
    s.numEfeitos = 1;
    return s;
}

// ---------------------------------------------------------------------------
// buildArmaInteligente — dispatcher (arma, nivel) → SkillData
// ---------------------------------------------------------------------------
inline SkillData buildArmaInteligente(int arma, int nivel) {
    if (nivel < 1) nivel = 1;
    if (nivel > NIVEL_MAX_ARMA_INTELIGENTE) nivel = NIVEL_MAX_ARMA_INTELIGENTE;
    switch (arma) {
        case ARMA_MISSIL_VAMPIRICO: return buildMissilVampiricoNivel(nivel);
        case ARMA_MISSIL_GUIADO:    return buildMissilGuiadoNivel(nivel);
        case ARMA_BOMBA_GUIADA:     return buildBombaGuiadaNivel(nivel);
        default:                    return buildBase();
    }
}

// ===========================================================================
//  CLASSIFICAÇÃO E EXCLUSIVIDADE
// ===========================================================================

inline bool ehArmaInteligente(int idFactory) {
    for (int a = 0; a < TOTAL_ARMAS_INTELIGENTES; ++a) {
        if (SkillFactory::id(nomeArmaInteligente(a)) == idFactory)
            return true;
    }
    return false;
}

inline int armaInteligenteDeId(int idFactory) {
    for (int a = 0; a < TOTAL_ARMAS_INTELIGENTES; ++a) {
        if (SkillFactory::id(nomeArmaInteligente(a)) == idFactory)
            return a;
    }
    return -1;
}

inline int armaInteligenteEquipada(const InventarioSkills& inv) {
    for (int i = 0; i < inv.numEquipadas; ++i) {
        int id = inv.equipadas[i];
        if (ehArmaInteligente(id)) return id;
    }
    return -1;
}

// Pode equipar esta arma inteligente? Só se NENHUMA outra estiver equipada.
inline bool podeEquiparArmaInteligente(const InventarioSkills& inv, int idFactory) {
    int atual = armaInteligenteEquipada(inv);
    return (atual < 0 || atual == idFactory);
}

// Já chegou ao nível máximo? (não deve mais ser oferecida para evoluir)
inline bool armaInteligenteNoMaximo(const InventarioSkills& inv, int idFactory) {
    if (idFactory < 0) return false;
    return inv.nivelSkill[idFactory] >= NIVEL_MAX_ARMA_INTELIGENTE;
}

// ---------------------------------------------------------------------------
// registrarArmasInteligentes — registra as 3 armas (nível 1) na factory.
// ---------------------------------------------------------------------------
inline void registrarArmasInteligentes() {
    RegistrarSkill("MissilVampirico", buildArmaInteligente(ARMA_MISSIL_VAMPIRICO, 1));
    RegistrarSkill("MissilGuiado",    buildArmaInteligente(ARMA_MISSIL_GUIADO, 1));
    RegistrarSkill("BombaGuiada",     buildArmaInteligente(ARMA_BOMBA_GUIADA, 1));
}

#endif // ARMA_INTELIGENTE_H
