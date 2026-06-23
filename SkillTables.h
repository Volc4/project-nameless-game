// ===========================================================================
//  SkillTables.cpp — Definição ÚNICA de tabelas globais do sistema de skills
//
//  Resolve ODR: a matriz COMPAT_FORMA_MOV é DECLARADA extern em
//  SkillValidator.h e DEFINIDA aqui uma só vez. Nenhum outro .cpp/.h define
//  este símbolo. Adicione este arquivo à lista de compilação do projeto:
//      g++ -std=c++98 ... SkillTables.cpp Main.cpp ...
//
//  A ordem das linhas segue FormaType e a das colunas segue MovimentoType,
//  EXATAMENTE como declarados em SkillTypes.h:
//
//    Linhas (FormaType):
//      0 PROJECTILE 1 CONE 2 RING 3 AREA 4 AURA 5 BEAM
//      6 WALL 7 WAVE 8 ARC 9 EXPLOSION 10 CHAIN 11 PRISM
//    Colunas (MovimentoType):
//      0 LIN 1 HOM 2 ORB 3 BOO 4 BOU 5 SPI 6 FAL 7 TEL 8 RND 9 STA
// ===========================================================================

#include "SkillTypes.h"

// 1 = combinação permitida, 0 = bloqueada.
// 'extern' explícito: garante linkage EXTERNA (em C++ um const de escopo
// namespace teria linkage interna por padrão; aqui precisamos do símbolo único
// global declarado em SkillValidator.h).
extern const bool COMPAT_FORMA_MOV[FORMA_TOTAL][MOV_TOTAL];
const bool COMPAT_FORMA_MOV[FORMA_TOTAL][MOV_TOTAL] = {
    //              LIN HOM ORB BOO BOU SPI FAL TEL RND STA
/* PROJECTILE */ {  1,  1,  0,  1,  1,  1,  0,  1,  1,  0 },
/* CONE       */ {  1,  0,  0,  0,  0,  1,  0,  0,  0,  0 },
/* RING       */ {  0,  0,  1,  0,  0,  0,  0,  0,  0,  0 },
/* AREA       */ {  0,  0,  0,  0,  0,  0,  1,  1,  0,  1 },
/* AURA       */ {  0,  0,  0,  0,  0,  0,  0,  0,  0,  1 },
/* BEAM       */ {  1,  0,  0,  0,  0,  0,  0,  1,  0,  1 },
/* WALL       */ {  0,  0,  0,  0,  0,  0,  0,  0,  0,  1 },
/* WAVE       */ {  1,  0,  0,  0,  0,  0,  1,  0,  0,  0 },
/* ARC        */ {  0,  0,  1,  0,  0,  0,  0,  0,  0,  1 },
/* EXPLOSION  */ {  0,  0,  0,  0,  0,  0,  0,  0,  0,  1 },
/* CHAIN      */ {  0,  0,  0,  0,  0,  0,  0,  0,  0,  1 },
/* PRISM      */ {  1,  1,  0,  1,  1,  0,  0,  0,  0,  0 }
};
