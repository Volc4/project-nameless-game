#ifndef SKILL_INVENTORY_H
#define SKILL_INVENTORY_H

// ===========================================================================
//  SkillInventory.h — Funções de inventário de skills do jogador (Fase 6)
//
//  ATENÇÃO: As structs InventarioSkills, SkillEstado, AtributosGlobais
//  são definidas em Entities.h para evitar dependência circular.
//  Este arquivo contém APENAS as funções inline que as operam.
//
//  Ordem de include obrigatória (já garantida pelo grafo existente):
//    Entities.h  →  SkillTypes.h  →  SkillFactory.h  →  SkillInventory.h
// ===========================================================================

#include "SkillTypes.h"    // FormaType, EfeitoType, etc.
#include "SkillFactory.h"  // SkillFactory::id(), nomeDe()
#include "Entities.h"      // InventarioSkills, SkillEstado, AtributosGlobais
#include <cstring>

// ---------------------------------------------------------------------------
// Limites reexportados (mesmos valores que _INV_* em Entities.h)
// ---------------------------------------------------------------------------
#define MAX_SKILLS_EQUIPADAS   _INV_MAX_EQUIPADAS
#define MAX_SKILLS_CATALOGO    _INV_MAX_CATALOGO
#define NIVEL_MAX_SKILL        _INV_NIVEL_MAX

// ---------------------------------------------------------------------------
// inicializarAtributosGlobais
// ---------------------------------------------------------------------------
inline void inicializarAtributosGlobais(AtributosGlobais& a) {
    a.bonusDano        = 1.0f;
    a.bonusVida        = 1.0f;
    a.bonusVelocidade  = 1.0f;
    a.bonusTensao      = 1.0f;
    a.bonusPerfuracao  = 0.0f;
    a.bonusHPFlat      = 0;
}

// ---------------------------------------------------------------------------
// inicializarInventario — zera tudo; coloca a skill "Disparo" (id 0)
//   como já equipada no slot 0.
// ---------------------------------------------------------------------------
inline void inicializarInventario(InventarioSkills& inv) {
    int i;
    for (i = 0; i < _INV_MAX_CATALOGO; ++i) {
        inv.estado[i]      = SKILL_BLOQUEADA;
        inv.nivelSkill[i]  = 0;
    }
    for (i = 0; i < _INV_MAX_EQUIPADAS; ++i)
        inv.equipadas[i] = -1;
    inv.numEquipadas = 0;
    inicializarAtributosGlobais(inv.atributosGlobais);

    // Skill "Disparo" sempre disponível e equipada desde o início
    int idDisparo = SkillFactory::id("Disparo");
    if (idDisparo < 0) idDisparo = 0;
    inv.estado[idDisparo]     = SKILL_EQUIPADA;
    inv.nivelSkill[idDisparo] = 0;
    inv.equipadas[0]          = idDisparo;
    inv.numEquipadas          = 1;
}

// ---------------------------------------------------------------------------
// estaEquipada
// ---------------------------------------------------------------------------
inline bool estaEquipada(const InventarioSkills& inv, int idSkill) {
    for (int i = 0; i < inv.numEquipadas; ++i)
        if (inv.equipadas[i] == idSkill) return true;
    return false;
}

// ---------------------------------------------------------------------------
// podeEvolir
// ---------------------------------------------------------------------------
inline bool podeEvolir(const InventarioSkills& inv, int idSkill) {
    if (idSkill < 0 || idSkill >= _INV_MAX_CATALOGO) return false;
    return estaEquipada(inv, idSkill) &&
           inv.nivelSkill[idSkill] < _INV_NIVEL_MAX;
}

// ---------------------------------------------------------------------------
// desbloquearSkill
// ---------------------------------------------------------------------------
inline void desbloquearSkill(InventarioSkills& inv, int idSkill) {
    if (idSkill < 0 || idSkill >= _INV_MAX_CATALOGO) return;
    if (inv.estado[idSkill] == SKILL_BLOQUEADA)
        inv.estado[idSkill] = SKILL_DISPONIVEL;
}

// ---------------------------------------------------------------------------
// equiparSkill
// ---------------------------------------------------------------------------
inline bool equiparSkill(InventarioSkills& inv, int idSkill) {
    if (idSkill < 0 || idSkill >= _INV_MAX_CATALOGO) return false;
    for (int i = 0; i < inv.numEquipadas; ++i)
        if (inv.equipadas[i] == idSkill) return false;

    if (inv.numEquipadas < _INV_MAX_EQUIPADAS) {
        inv.equipadas[inv.numEquipadas++] = idSkill;
    } else {
        inv.equipadas[_INV_MAX_EQUIPADAS - 1] = idSkill;
    }
    inv.estado[idSkill] = SKILL_EQUIPADA;
    return true;
}

// ---------------------------------------------------------------------------
// evoluirSkill
// ---------------------------------------------------------------------------
inline int evoluirSkill(InventarioSkills& inv, int idSkill) {
    if (idSkill < 0 || idSkill >= _INV_MAX_CATALOGO) return -1;
    if (inv.nivelSkill[idSkill] >= _INV_NIVEL_MAX) return -1;

    inv.nivelSkill[idSkill]++;
    int novo = inv.nivelSkill[idSkill];
    inv.estado[idSkill] = (novo >= _INV_NIVEL_MAX) ? SKILL_NIVEL_MAX : SKILL_EVOLUIDA;
    return novo;
}

// ---------------------------------------------------------------------------
// aplicarAtributoGlobal
// ---------------------------------------------------------------------------
inline void aplicarAtributoGlobal(InventarioSkills& inv, TipoUpgrade tipo, int nivel) {
    AtributosGlobais& a = inv.atributosGlobais;
    switch (tipo) {
        case DANO:
            a.bonusDano *= (1.0f + 0.10f * nivel);
            break;
        case VIDA:
            a.bonusVida *= (1.0f + 0.15f * nivel);
            a.bonusHPFlat += nivel;
            break;
        case VELOCIDADE:
            a.bonusVelocidade *= (1.0f + 0.20f * nivel);
            break;
        case TENSAO_UP:
            a.bonusTensao *= (1.0f - 0.10f * nivel);
            if (a.bonusTensao < 0.1f) a.bonusTensao = 0.1f;
            break;
        case PERFURACAO:
            a.bonusPerfuracao += (float)nivel;
            break;
        case CADENCIA:
            a.bonusTensao *= (1.0f - 0.05f * nivel);
            if (a.bonusTensao < 0.1f) a.bonusTensao = 0.1f;
            break;
        default:
            break;
    }
}

#endif // SKILL_INVENTORY_H
