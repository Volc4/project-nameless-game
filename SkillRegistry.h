#ifndef SKILL_REGISTRY_H
#define SKILL_REGISTRY_H

// ===========================================================================
//  SkillRegistry.h — Único lugar onde skills ganham nome concreto.
//
//  Cada build*() monta uma SkillData por composição de componentes.
//  registrarSkillsPadrao() popula a SkillFactory uma vez na inicialização.
//  SkillManager nunca inclui este arquivo.
// ===========================================================================

#include "SkillTypes.h"
#include "SkillCatalog.h"
#include "SkillFactory.h"

inline SkillData _novaSkill() { return buildBase(); }

// --- Disparo básico ---
inline SkillData buildDisparo() {
    return buildBase();
}

// --- Pilar Celestial de Fogo: Area + Fall + RandomMap + Burn ---
inline SkillData buildPilarFogo() {
    SkillData s = _novaSkill();
    s.forma.tipo          = FORMA_AREA;
    s.forma.raioColisao   = 3.0f;
    s.forma.duracao       = 1.5f;
    s.forma.tickIntervalo = 0.25f;
    s.movimento.tipo            = MOV_FALL;
    s.movimento.velocidadeQueda = 30.0f;
    s.movimento.tempoAviso      = 0.5f;
    s.origem.tipo = ORIG_RANDOMMAP;
    s.efeitos[0].tipo = EFE_DAMAGE; s.efeitos[0].valor = 5;
    s.efeitos[1].tipo = EFE_BURN;   s.efeitos[1].magnitude = 3.0f;
    s.efeitos[1].duracao = 2.0f;    s.efeitos[1].empilhamento = STACK_LIMITADO;
    s.efeitos[1].maxStacks = 3;
    s.numEfeitos = 2;
    return s;
}

// --- Tempestade Orbital: Ring + Orbit + Player + Shock ---
inline SkillData buildTempestadeOrbital() {
    SkillData s = _novaSkill();
    s.forma.tipo        = FORMA_RING;
    s.forma.quantidade  = 6;
    s.forma.raioColisao = 0.25f;
    s.movimento.tipo             = MOV_ORBIT;
    s.movimento.raioOrbita       = 3.0f;
    s.movimento.velocidadeAngular= 4.0f;
    s.origem.tipo              = ORIG_PLAYER;
    s.origem.reavaliarPorFrame = true;
    s.efeitos[0].tipo = EFE_SHOCK; s.efeitos[0].valor = 4; s.efeitos[0].duracao = 0.3f;
    s.numEfeitos = 1;
    return s;
}

// --- Lança de Luz: Beam + Stationary + Stand + Damage ---
inline SkillData buildLancaDeLuz() {
    SkillData s = _novaSkill();
    s.forma.tipo          = FORMA_BEAM;
    s.forma.comprimento   = 12.0f;
    s.forma.largura       = 0.4f;
    s.forma.duracao       = 0.4f;
    s.forma.tickIntervalo = 0.1f;
    s.movimento.tipo = MOV_STATIONARY;
    s.origem.tipo    = ORIG_STAND;
    s.efeitos[0].tipo = EFE_DAMAGE; s.efeitos[0].valor = 10;
    s.numEfeitos = 1;
    return s;
}

// --- Bumerangue Congelante: Projectile + Boomerang + Stand + Freeze ---
inline SkillData buildBumerangueCongelante() {
    SkillData s = _novaSkill();
    s.forma.tipo        = FORMA_PROJECTILE;
    s.forma.raioColisao = 0.30f;
    s.forma.alcanceMax  = 10.0f;
    s.movimento.tipo       = MOV_BOOMERANG;
    s.movimento.velocidade = 20.0f;
    s.origem.tipo = ORIG_STAND;
    s.efeitos[0].tipo = EFE_DAMAGE; s.efeitos[0].valor = 6;
    s.efeitos[1].tipo = EFE_FREEZE; s.efeitos[1].magnitude = 0.5f;
    s.efeitos[1].duracao = 1.5f;    s.efeitos[1].empilhamento = STACK_REFRESCA;
    s.numEfeitos = 2;
    return s;
}

// --- Cadeia Elétrica: Chain + Stationary + NearestEnemy + Shock ---
inline SkillData buildCadeiaEletrica() {
    SkillData s = _novaSkill();
    s.forma.tipo     = FORMA_CHAIN;
    s.forma.saltosMax= 5;
    s.forma.raioColisao = 0.30f;
    s.movimento.tipo = MOV_STATIONARY;
    s.origem.tipo    = ORIG_NEARESTENEMY;
    s.efeitos[0].tipo = EFE_SHOCK; s.efeitos[0].valor = 8; s.efeitos[0].duracao = 0.2f;
    s.numEfeitos = 1;
    return s;
}

// Popula a SkillFactory — chamado uma vez em main().
// Armas Inteligentes são registradas separadamente por registrarArmasInteligentes().
inline void registrarSkillsPadrao() {
    RegistrarSkill("Disparo",            buildDisparo());
    RegistrarSkill("PilarFogo",          buildPilarFogo());
    RegistrarSkill("TempestadeOrbital",  buildTempestadeOrbital());
    RegistrarSkill("LancaDeLuz",         buildLancaDeLuz());
    RegistrarSkill("BumerangueCongelante", buildBumerangueCongelante());
    RegistrarSkill("CadeiaEletrica",     buildCadeiaEletrica());
}

#endif // SKILL_REGISTRY_H