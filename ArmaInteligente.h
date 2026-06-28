#ifndef ARMA_INTELIGENTE_H
#define ARMA_INTELIGENTE_H

// ===========================================================================
//  ArmaInteligente.h — Classe "Arma Inteligente"
//
//  REGRAS (spec "Classe: Arma Inteligente"):
//   - Apenas UMA arma inteligente equipada por partida (exclusividade).
//   - Cada arma tem 3 níveis (1=adquirida, 2, 3=máximo).
//   - Ao atingir nível 3, deixa de aparecer como opção de evolução.
//   - Cada nível é uma BUILD EXPLÍCITA (não escala genérica): o nível 3 muda
//     comportamento (ex.: 3 mísseis, explosão por impacto), fiel ao spec.
//
//  INTEGRAÇÃO COM O INVENTÁRIO EXISTENTE:
//   Reusa InventarioSkills (estado/nivelSkill/equipadas). Este módulo apenas
//   (1) classifica quais ids são armas inteligentes, (2) impõe exclusividade
//   no equipar, (3) fornece a SkillData correta por nível. O nível segue
//   vivendo em inv.nivelSkill[id]; aqui o teto efetivo é 3.
//
//  C++98: structs POD, switch sobre enum, sem STL associativa.
// ===========================================================================

#include "SkillTypes.h"
#include "SkillFactory.h"
#include "SkillRegistry.h"   // _novaSkill / buildBase
#include "SkillInventory.h"
#include "Entities.h"

// Identificadores das três armas inteligentes.
enum ArmaInteligenteId {
    ARMA_MISSIL_VAMPIRICO = 0,
    ARMA_MISSIL_GUIADO,
    ARMA_ESPIRITO_CACADOR,
    TOTAL_ARMAS_INTELIGENTES
};

// Nível máximo de uma arma inteligente (spec: 3).
#define NIVEL_MAX_ARMA_INTELIGENTE 3

// Nomes registrados na factory (chave textual).
inline const char* nomeArmaInteligente(int arma) {
    switch (arma) {
        case ARMA_MISSIL_VAMPIRICO: return "MissilVampirico";
        case ARMA_MISSIL_GUIADO:    return "MissilGuiado";
        case ARMA_ESPIRITO_CACADOR: return "EspiritoCacador";
        default:                    return "Disparo";
    }
}

inline const char* tituloArmaInteligente(int arma) {
    switch (arma) {
        case ARMA_MISSIL_VAMPIRICO: return "Missil Vampirico";
        case ARMA_MISSIL_GUIADO:    return "Missil Guiado";
        case ARMA_ESPIRITO_CACADOR: return "Espirito Cacador";
        default:                    return "Disparo";
    }
}

// ===========================================================================
//  BUILDS POR NÍVEL — cada arma, cada nível, é uma SkillData explícita.
//  nivel ∈ {1,2,3}. (nivel 0 = não adquirida; tratado pelo chamador.)
// ===========================================================================

// --- 1. MÍSSIL VAMPÍRICO: sobrevivência via roubo de vida ------------------
inline SkillData buildMissilVampiricoNivel(int nivel) {
    SkillData s = buildBase();
    s.forma.tipo            = FORMA_PROJECTILE;
    s.forma.raioColisao     = 0.22f;
    s.movimento.tipo        = MOV_HOMING;
    s.origem.tipo           = ORIG_NEARESTENEMY;  // persegue o mais próximo
    s.cooldown              = 1.10f;

    if (nivel <= 1) {
        s.movimento.velocidade   = 20.0f;
        s.movimento.taxaCorrecao = 0.55f;
        s.efeitos[0].tipo  = EFE_DAMAGE; s.efeitos[0].valor = 7;
        s.efeitos[1].tipo  = EFE_HEAL;   s.efeitos[1].magnitude = 0.05f; // 5% lifesteal
        s.numEfeitos = 2;
    } else if (nivel == 2) {
        s.movimento.velocidade   = 26.0f;             // perseguição mais rápida
        s.movimento.taxaCorrecao = 0.75f;
        s.efeitos[0].tipo  = EFE_DAMAGE; s.efeitos[0].valor = 10;        // +40%
        s.efeitos[1].tipo  = EFE_HEAL;   s.efeitos[1].magnitude = 0.10f; // 10%
        s.numEfeitos = 2;
    } else { // nivel 3: três mísseis, explosão por impacto, prioriza elites
        s.forma.tipo       = FORMA_CONE;   // múltiplos projéteis simultâneos
        s.forma.quantidade = 3;            // três mísseis, cada um homing
        s.forma.spreadAngulo = 0.25f;
        s.movimento.velocidade   = 28.0f;
        s.movimento.taxaCorrecao = 0.85f;
        s.efeitos[0].tipo  = EFE_DAMAGE;    s.efeitos[0].valor = 12;
        s.efeitos[1].tipo  = EFE_HEAL;      s.efeitos[1].magnitude = 0.10f;
        s.efeitos[2].tipo  = EFE_EXPLOSION; s.efeitos[2].magnitude = 1.4f; // pequena explosão
        s.numEfeitos = 3;
    }
    return s;
}

// --- 2. MÍSSIL GUIADO: eliminar alvos de alta vida -------------------------
inline SkillData buildMissilGuiadoNivel(int nivel) {
    SkillData s = buildBase();
    s.forma.tipo     = FORMA_PROJECTILE;
    s.forma.raioColisao = 0.24f;
    s.movimento.tipo = MOV_HOMING;
    s.origem.tipo    = ORIG_NEARESTENEMY;

    if (nivel <= 1) {
        s.movimento.velocidade   = 24.0f;
        s.movimento.taxaCorrecao = 0.50f;
        s.cooldown               = 1.40f;
        s.efeitos[0].tipo = EFE_DAMAGE; s.efeitos[0].valor = 14;
        s.numEfeitos = 1;
    } else if (nivel == 2) {
        s.movimento.velocidade   = 30.0f;
        s.movimento.taxaCorrecao = 0.80f;             // curvas muito mais rápidas
        s.cooldown               = 1.05f;             // menor recarga
        s.efeitos[0].tipo = EFE_DAMAGE;    s.efeitos[0].valor = 21; // +50%
        s.efeitos[1].tipo = EFE_EXPLOSION; s.efeitos[1].magnitude = 1.6f;
        s.numEfeitos = 2;
    } else { // nivel 3: dois mísseis pesados, grande explosão em área
        s.forma.tipo       = FORMA_CONE;
        s.forma.quantidade = 2;            // dois mísseis pesados
        s.forma.spreadAngulo = 0.18f;
        s.movimento.velocidade   = 30.0f;
        s.movimento.taxaCorrecao = 0.90f;
        s.cooldown               = 1.15f;
        s.efeitos[0].tipo = EFE_DAMAGE;    s.efeitos[0].valor = 28;
        s.efeitos[1].tipo = EFE_EXPLOSION; s.efeitos[1].magnitude = 3.0f; // grande área
        s.numEfeitos = 2;
    }
    return s;
}

// --- 3. ESPÍRITO CAÇADOR: dano automático constante, atravessa inimigos ----
inline SkillData buildEspiritoCacadorNivel(int nivel) {
    SkillData s = buildBase();
    s.forma.tipo     = FORMA_PROJECTILE;
    s.forma.raioColisao = 0.30f;
    s.movimento.tipo = MOV_HOMING;
    s.origem.tipo    = ORIG_NEARESTENEMY;

    if (nivel <= 1) {
        s.movimento.velocidade   = 10.0f;     // atravessa lentamente
        s.movimento.taxaCorrecao = 0.30f;
        s.forma.perfuracao       = 3;         // atravessa alguns
        s.cooldown               = 2.0f;
        s.efeitos[0].tipo = EFE_DAMAGE; s.efeitos[0].valor = 6;
        s.numEfeitos = 1;
    } else if (nivel == 2) {
        s.movimento.velocidade   = 18.0f;     // muito mais rápido
        s.movimento.taxaCorrecao = 0.55f;
        s.forma.perfuracao       = PERFURACAO_INFINITA;      // atravessa todos
        s.cooldown               = 1.7f;
        s.efeitos[0].tipo = EFE_DAMAGE; s.efeitos[0].valor = 8; // +40%
        s.numEfeitos = 1;
    } else { // nivel 3: três espíritos, cada um com alvo próprio
        s.forma.tipo       = FORMA_CONE;
        s.forma.quantidade = 3;               // três espíritos
        s.forma.spreadAngulo = 0.40f;
        s.forma.perfuracao = PERFURACAO_INFINITA;
        s.movimento.velocidade   = 20.0f;
        s.movimento.taxaCorrecao = 0.60f;
        s.cooldown               = 1.6f;
        s.efeitos[0].tipo = EFE_DAMAGE; s.efeitos[0].valor = 9;
        s.numEfeitos = 1;
    }
    return s;
}

// ---------------------------------------------------------------------------
// buildArmaInteligente — dispatcher: (arma, nivel) → SkillData explícita.
// ---------------------------------------------------------------------------
inline SkillData buildArmaInteligente(int arma, int nivel) {
    if (nivel < 1) nivel = 1;
    if (nivel > NIVEL_MAX_ARMA_INTELIGENTE) nivel = NIVEL_MAX_ARMA_INTELIGENTE;
    switch (arma) {
        case ARMA_MISSIL_VAMPIRICO: return buildMissilVampiricoNivel(nivel);
        case ARMA_MISSIL_GUIADO:    return buildMissilGuiadoNivel(nivel);
        case ARMA_ESPIRITO_CACADOR: return buildEspiritoCacadorNivel(nivel);
        default:                    return buildBase();
    }
}

// ===========================================================================
//  CLASSIFICAÇÃO E EXCLUSIVIDADE
// ===========================================================================

// É este id (na factory) uma arma inteligente? Resolve por nome registrado.
inline bool ehArmaInteligente(int idFactory) {
    for (int a = 0; a < TOTAL_ARMAS_INTELIGENTES; ++a) {
        if (SkillFactory::id(nomeArmaInteligente(a)) == idFactory)
            return true;
    }
    return false;
}

// Mapeia id da factory → ArmaInteligenteId (ou -1 se não for).
inline int armaInteligenteDeId(int idFactory) {
    for (int a = 0; a < TOTAL_ARMAS_INTELIGENTES; ++a) {
        if (SkillFactory::id(nomeArmaInteligente(a)) == idFactory)
            return a;
    }
    return -1;
}

// Já existe uma arma inteligente equipada? Retorna o id da factory ou -1.
inline int armaInteligenteEquipada(const InventarioSkills& inv) {
    for (int i = 0; i < inv.numEquipadas; ++i) {
        int id = inv.equipadas[i];
        if (ehArmaInteligente(id)) return id;
    }
    return -1;
}

// Pode equipar esta arma inteligente? Só se NENHUMA outra estiver equipada
// (exclusividade: uma por partida). Se já é a mesma, também é permitido
// (para evoluir).
inline bool podeEquiparArmaInteligente(const InventarioSkills& inv, int idFactory) {
    int atual = armaInteligenteEquipada(inv);
    return (atual < 0 || atual == idFactory);
}

// Esta arma inteligente já está no nível máximo? (não deve mais ser oferecida)
inline bool armaInteligenteNoMaximo(const InventarioSkills& inv, int idFactory) {
    if (idFactory < 0) return false;
    return inv.nivelSkill[idFactory] >= NIVEL_MAX_ARMA_INTELIGENTE;
}

// ---------------------------------------------------------------------------
// registrarArmasInteligentes — registra as 3 armas (nível 1) na factory.
//   Os níveis 2 e 3 são aplicados dinamicamente via buildArmaInteligente()
//   quando a build é remontada. Chamado em registrarSkillsPadrao().
// ---------------------------------------------------------------------------
inline void registrarArmasInteligentes() {
    RegistrarSkill("MissilVampirico", buildArmaInteligente(ARMA_MISSIL_VAMPIRICO, 1));
    RegistrarSkill("MissilGuiado",    buildArmaInteligente(ARMA_MISSIL_GUIADO, 1));
    RegistrarSkill("EspiritoCacador", buildArmaInteligente(ARMA_ESPIRITO_CACADOR, 1));
}

#endif // ARMA_INTELIGENTE_H
