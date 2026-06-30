#ifndef SKILL_INVENTORY_H
#define SKILL_INVENTORY_H

// ===========================================================================
//  SkillInventory.h — Inventário de skills: desbloqueio, equipamento, evolução.
// ===========================================================================

#include "Entities.h"
#include <cstring>   // memset

// Aliases typed para os #define em Entities.h.
static const int MAX_SKILLS_EQUIPADAS = _INV_MAX_EQUIPADAS;   //  6
static const int MAX_CATALOGO_INV     = _INV_MAX_CATALOGO;    // 128
static const int NIVEL_MAX_SKILL      = _INV_NIVEL_MAX;       //  5
static const int MAX_ESCOLHAS_MENU    = _MENU_MAX_ESCOLHAS;   //  3
static const int ESCOLHA_DESC_MAX     = _ESCOLHA_DESC_MAX;    // 96
static const int ESCOLHA_SUB_MAX      = _ESCOLHA_SUB_MAX;     // 64

// Estado limpo: todas bloqueadas, multiplicadores neutros, Disparo (id 0) equipado no slot 0.
inline void inicializarInventario(InventarioSkills& inv) {
    for (int i = 0; i < MAX_CATALOGO_INV; ++i) {
        inv.estado[i]     = SKILL_BLOQUEADA;
        inv.nivelSkill[i] = 0;
    }
    for (int i = 0; i < MAX_SKILLS_EQUIPADAS; ++i)
        inv.equipadas[i] = -1;
    inv.numEquipadas = 0;

    inv.atributosGlobais.bonusDano        = 1.0f;
    inv.atributosGlobais.bonusVida        = 1.0f;
    inv.atributosGlobais.bonusVelocidade  = 1.0f;
    inv.atributosGlobais.bonusTensao      = 1.0f;
    inv.atributosGlobais.bonusPerfuracao  = 0.0f;
    inv.atributosGlobais.bonusHPFlat      = 0;

    inv.estado[0]     = SKILL_EQUIPADA;
    inv.nivelSkill[0] = 0;
    inv.equipadas[0]  = 0;
    inv.numEquipadas  = 1;
}

// Reseta o MenuLevelUp entre sorteios.
inline void limparMenu(MenuLevelUp& menu) {
    menu.quantidade = 0;
    for (int i = 0; i < MAX_ESCOLHAS_MENU; ++i) {
        menu.escolhas[i].tipo       = ESCOLHA_NOVA_SKILL;
        menu.escolhas[i].raridade   = RARIDADE_COMUM;
        menu.escolhas[i].referencia = -1;
        menu.escolhas[i].valorExtra = 0;
        menu.escolhas[i].descricao[0] = '\0';
        menu.escolhas[i].subtitulo[0] = '\0';
    }
}

inline bool estaEquipada(const InventarioSkills& inv, int idSkill) {
    if (idSkill < 0 || idSkill >= MAX_CATALOGO_INV) return false;
    return inv.estado[idSkill] == SKILL_EQUIPADA ||
           inv.estado[idSkill] == SKILL_EVOLUIDA  ||
           inv.estado[idSkill] == SKILL_NIVEL_MAX;
}

inline bool podeEvolir(const InventarioSkills& inv, int idSkill) {
    if (idSkill < 0 || idSkill >= MAX_CATALOGO_INV) return false;
    if (!estaEquipada(inv, idSkill)) return false;
    return inv.nivelSkill[idSkill] < NIVEL_MAX_SKILL;
}

// Passa de BLOQUEADA para DISPONIVEL.
inline void desbloquearSkill(InventarioSkills& inv, int idSkill) {
    if (idSkill < 0 || idSkill >= MAX_CATALOGO_INV) return;
    if (inv.estado[idSkill] == SKILL_BLOQUEADA)
        inv.estado[idSkill] = SKILL_DISPONIVEL;
}

// Equipa no próximo slot livre; false se sem espaço ou já equipada.
inline bool equiparSkill(InventarioSkills& inv, int idSkill) {
    if (idSkill < 0 || idSkill >= MAX_CATALOGO_INV) return false;
    if (inv.numEquipadas >= MAX_SKILLS_EQUIPADAS)   return false;
    if (estaEquipada(inv, idSkill))                 return false;

    if (inv.estado[idSkill] == SKILL_BLOQUEADA)
        inv.estado[idSkill] = SKILL_DISPONIVEL;

    inv.equipadas[inv.numEquipadas] = idSkill;
    inv.numEquipadas++;
    inv.estado[idSkill]     = SKILL_EQUIPADA;
    inv.nivelSkill[idSkill] = 0;
    return true;
}

inline bool evoluirSkill(InventarioSkills& inv, int idSkill) {
    if (!podeEvolir(inv, idSkill)) return false;

    inv.nivelSkill[idSkill]++;
    if (inv.nivelSkill[idSkill] >= NIVEL_MAX_SKILL)
        inv.estado[idSkill] = SKILL_NIVEL_MAX;
    else
        inv.estado[idSkill] = SKILL_EVOLUIDA;
    return true;
}

// Remove do slot, volta a DISPONIVEL; swap-and-pop para manter compacto.
inline bool removerSkill(InventarioSkills& inv, int idSkill) {
    if (idSkill < 0 || idSkill >= MAX_CATALOGO_INV) return false;
    if (!estaEquipada(inv, idSkill)) return false;

    for (int i = 0; i < inv.numEquipadas; ++i) {
        if (inv.equipadas[i] == idSkill) {
            inv.equipadas[i] = inv.equipadas[inv.numEquipadas - 1];
            inv.equipadas[inv.numEquipadas - 1] = -1;
            inv.numEquipadas--;
            inv.estado[idSkill] = SKILL_DISPONIVEL;
            return true;
        }
    }
    return false;
}

// Atualiza AtributosGlobais conforme upgrade. Casos de dano/cadência são
// processados em aplicarUpgradesNaSkill(); aqui apenas upgrades de jogador.
inline void aplicarAtributoGlobal(InventarioSkills& inv,
                                   TipoUpgrade tipo, int /*vezes*/) {
    (void)inv; (void)tipo;
}

// Cor RGB para cada raridade — usada no menu de level-up.
inline void corRaridade(EscolhaRaridade r, float& R, float& G, float& B) {
    switch (r) {
        case RARIDADE_COMUM:    R = 0.80f; G = 0.80f; B = 0.80f; break; // cinza
        case RARIDADE_INCOMUM:  R = 0.20f; G = 0.80f; B = 0.20f; break; // verde
        case RARIDADE_RARA:     R = 0.20f; G = 0.40f; B = 1.00f; break; // azul
        case RARIDADE_EPICA:    R = 0.60f; G = 0.10f; B = 0.90f; break; // roxo
        case RARIDADE_LENDARIA: R = 1.00f; G = 0.65f; B = 0.00f; break; // ouro
        default:                R = 1.00f; G = 1.00f; B = 1.00f; break;
    }
}

// Declaração; definida em Main.cpp (acessa estado global do jogo).
inline void aplicarEscolhaMenu(EstadoDoJogo& jogo, int indiceEscolha);

#endif // SKILL_INVENTORY_H
