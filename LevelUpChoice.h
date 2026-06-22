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

#endif // LEVEL_UP_CHOICE_H
