#ifndef LEVEL_UP_CHOICE_H
#define LEVEL_UP_CHOICE_H

// ===========================================================================
//  LevelUpChoice.h — Funções utilitárias para o menu de Level Up (Fase 6)
//
//  As structs LevelUpChoice, MenuLevelUp, EscolhaTipo, EscolhaRaridade
//  são definidas em Entities.h para evitar dependência circular.
//  Este arquivo contém APENAS as funções inline que as operam.
// ===========================================================================

#include "Entities.h"   // LevelUpChoice, MenuLevelUp, EscolhaTipo, EscolhaRaridade

// Limites reexportados
#define MAX_ESCOLHAS_MENU   _MENU_MAX_ESCOLHAS
#define ESCOLHA_DESC_MAX    _ESCOLHA_DESC_MAX
#define ESCOLHA_SUB_MAX     _ESCOLHA_SUB_MAX

// ---------------------------------------------------------------------------
// limparMenu
// ---------------------------------------------------------------------------
inline void limparMenu(MenuLevelUp& m) {
    m.quantidade = 0;
    for (int i = 0; i < _MENU_MAX_ESCOLHAS; ++i) {
        m.escolhas[i].tipo       = ESCOLHA_ATRIBUTO_GLOBAL;
        m.escolhas[i].raridade   = RARIDADE_COMUM;
        m.escolhas[i].referencia = -1;
        m.escolhas[i].valorExtra = 0;
        m.escolhas[i].descricao[0] = '\0';
        m.escolhas[i].subtitulo[0] = '\0';
    }
}

// ---------------------------------------------------------------------------
// nomeRaridade — string descritivo para display
// ---------------------------------------------------------------------------
inline const char* nomeRaridade(EscolhaRaridade r) {
    switch (r) {
        case RARIDADE_COMUM:    return "Comum";
        case RARIDADE_INCOMUM:  return "Incomum";
        case RARIDADE_RARA:     return "Rara";
        case RARIDADE_EPICA:    return "Epica";
        case RARIDADE_LENDARIA: return "Lendaria";
        default:                return "";
    }
}

// ---------------------------------------------------------------------------
// corRaridade — RGB (0..1) para glColor3f em Main.cpp
// ---------------------------------------------------------------------------
inline void corRaridade(EscolhaRaridade r, float& red, float& grn, float& blu) {
    switch (r) {
        case RARIDADE_COMUM:    red=0.85f; grn=0.85f; blu=0.85f; return;
        case RARIDADE_INCOMUM:  red=0.20f; grn=1.00f; blu=0.30f; return;
        case RARIDADE_RARA:     red=0.30f; grn=0.60f; blu=1.00f; return;
        case RARIDADE_EPICA:    red=0.80f; grn=0.20f; blu=1.00f; return;
        case RARIDADE_LENDARIA: red=1.00f; grn=0.60f; blu=0.10f; return;
        default:                red=1.00f; grn=1.00f; blu=1.00f; return;
    }
}

#endif // LEVEL_UP_CHOICE_H
