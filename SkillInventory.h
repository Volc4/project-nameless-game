#ifndef SKILL_INVENTORY_H
#define SKILL_INVENTORY_H

// ===========================================================================
//  SkillInventory.h — Gerenciamento do inventário de skills do jogador
//
//  Responsabilidades:
//    - Constantes de limite do inventário (espelhadas em Entities.h via
//      #define para autossuficiência de EstadoDoJogo).
//    - inicializarInventario(): estado inicial limpo.
//    - Funções de consulta: estaEquipada(), podeEvolir().
//    - Funções de modificação: desbloquearSkill(), equiparSkill(),
//      evoluirSkill(), removerSkill().
//    - aplicarAtributoGlobal(): atualiza AtributosGlobais do inventário.
//
//  Utiliza as structs InventarioSkills, SkillEstado e AtributosGlobais
//  definidas diretamente em Entities.h (para quebrar dependência circular
//  com SkillTypes.h).
// ===========================================================================

#include "Entities.h"
#include <cstring>   // memset

// Constantes de inventário (mesmo valor dos #define em Entities.h)
static const int MAX_SKILLS_EQUIPADAS = _INV_MAX_EQUIPADAS;   //  6
static const int MAX_CATALOGO_INV     = _INV_MAX_CATALOGO;    // 128
static const int NIVEL_MAX_SKILL      = _INV_NIVEL_MAX;       //  5
static const int MAX_ESCOLHAS_MENU    = _MENU_MAX_ESCOLHAS;   //  3
static const int ESCOLHA_DESC_MAX     = _ESCOLHA_DESC_MAX;    // 96
static const int ESCOLHA_SUB_MAX      = _ESCOLHA_SUB_MAX;     // 64

// ---------------------------------------------------------------------------
// inicializarInventario — configura o estado inicial limpo.
//   - Todas as skills bloqueadas, exceto o Disparo (id 0 — desbloqueado
//     e equipado por padrão no slot 0).
//   - Atributos globais em 1.0 (multiplicadores neutros).
// ---------------------------------------------------------------------------
inline void inicializarInventario(InventarioSkills& inv) {
    // Zera o array de estados
    for (int i = 0; i < MAX_CATALOGO_INV; ++i) {
        inv.estado[i]      = SKILL_BLOQUEADA;
        inv.nivelSkill[i]  = 0;
    }

    // Nenhum slot ocupado inicialmente
    for (int i = 0; i < MAX_SKILLS_EQUIPADAS; ++i)
        inv.equipadas[i] = -1;
    inv.numEquipadas = 0;

    // Atributos globais neutros
    inv.atributosGlobais.bonusDano        = 1.0f;
    inv.atributosGlobais.bonusVida        = 1.0f;
    inv.atributosGlobais.bonusVelocidade  = 1.0f;
    inv.atributosGlobais.bonusTensao      = 1.0f;
    inv.atributosGlobais.bonusPerfuracao  = 0.0f;
    inv.atributosGlobais.bonusHPFlat      = 0;

    // Disparo base (id 0): desbloqueado e equipado por padrão
    // (a factory pode não estar populada ainda; o ProgressionSystem
    //  chama montarBuildCompleta() depois, o que resolve o slot 0.)
    inv.estado[0]     = SKILL_EQUIPADA;
    inv.nivelSkill[0] = 0;
    inv.equipadas[0]  = 0;
    inv.numEquipadas  = 1;
}

// ---------------------------------------------------------------------------
// limparMenu — reseta o MenuLevelUp entre sorteios.
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Consultas
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Modificações
// ---------------------------------------------------------------------------

// Desbloqueia uma skill (passa de BLOQUEADA para DISPONIVEL).
inline void desbloquearSkill(InventarioSkills& inv, int idSkill) {
    if (idSkill < 0 || idSkill >= MAX_CATALOGO_INV) return;
    if (inv.estado[idSkill] == SKILL_BLOQUEADA)
        inv.estado[idSkill] = SKILL_DISPONIVEL;
}

// Equipa uma skill no próximo slot livre, se houver espaço.
inline bool equiparSkill(InventarioSkills& inv, int idSkill) {
    if (idSkill < 0 || idSkill >= MAX_CATALOGO_INV) return false;
    if (inv.numEquipadas >= MAX_SKILLS_EQUIPADAS)   return false;
    if (estaEquipada(inv, idSkill))                 return false; // já equipada

    // Garante que está pelo menos DISPONIVEL antes de equipar
    if (inv.estado[idSkill] == SKILL_BLOQUEADA)
        inv.estado[idSkill] = SKILL_DISPONIVEL;

    inv.equipadas[inv.numEquipadas] = idSkill;
    inv.numEquipadas++;
    inv.estado[idSkill]     = SKILL_EQUIPADA;
    inv.nivelSkill[idSkill] = 0;
    return true;
}

// Evolui uma skill equipada (incrementa nível, atualiza estado).
inline bool evoluirSkill(InventarioSkills& inv, int idSkill) {
    if (!podeEvolir(inv, idSkill)) return false;

    inv.nivelSkill[idSkill]++;
    if (inv.nivelSkill[idSkill] >= NIVEL_MAX_SKILL)
        inv.estado[idSkill] = SKILL_NIVEL_MAX;
    else
        inv.estado[idSkill] = SKILL_EVOLUIDA;
    return true;
}

// Remove uma skill do slot de equipadas (volta a DISPONIVEL).
inline bool removerSkill(InventarioSkills& inv, int idSkill) {
    if (idSkill < 0 || idSkill >= MAX_CATALOGO_INV) return false;
    if (!estaEquipada(inv, idSkill)) return false;

    // Procura o slot
    for (int i = 0; i < inv.numEquipadas; ++i) {
        if (inv.equipadas[i] == idSkill) {
            // Remove com swap-and-pop
            inv.equipadas[i] = inv.equipadas[inv.numEquipadas - 1];
            inv.equipadas[inv.numEquipadas - 1] = -1;
            inv.numEquipadas--;
            inv.estado[idSkill] = SKILL_DISPONIVEL;
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// aplicarAtributoGlobal — atualiza os AtributosGlobais do inventário
//   conforme o TipoUpgrade escolhido pelo jogador.
// ---------------------------------------------------------------------------
inline void aplicarAtributoGlobal(InventarioSkills& inv,
                                   TipoUpgrade tipo, int /*vezes*/) {
    AtributosGlobais& a = inv.atributosGlobais;
    switch (tipo) {
        case DANO:
            a.bonusDano += 0.10f;          // +10% dano
            break;
        case CADENCIA:
            // Processado diretamente via escala de cooldown em aplicarUpgradesNaSkill e Main.cpp
            break;
        case PERFURACAO:
            a.bonusPerfuracao += 1.0f;     // +1 perfuração flat
            break;
        case TENSAO_UP:
            a.bonusTensao -= 0.10f;        // −10% custo tensão
            if (a.bonusTensao < 0.05f) a.bonusTensao = 0.05f;
            break;
        case VELOCIDADE:
            a.bonusVelocidade += 0.20f;    // +20% velocidade de movimento
            break;
        case VIDA:
            a.bonusVida  += 0.15f;         // +15% HP
            a.bonusHPFlat += 1;
            break;
        default:
            break;
    }
}

// ---------------------------------------------------------------------------
// corRaridade — devolve cor RGB de uma raridade de escolha.
//   Usado no render do menu de level-up (Main.cpp).
// ---------------------------------------------------------------------------
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

// Alias compatível com a chamada em ProgressionSystem.h
inline void aplicarEscolhaMenu(EstadoDoJogo& jogo, int indiceEscolha);
// (Definida em ProgressionSystem.h via aplicarEscolhaLevelUp — aqui apenas
//  declarada para que código legacy que use este nome compile. A implementação
//  real está em ProgressionSystem.h e chama aplicarEscolhaLevelUp.)

#endif // SKILL_INVENTORY_H
