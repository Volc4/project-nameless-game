#ifndef SKILL_CATALOG_H
#define SKILL_CATALOG_H

// ===========================================================================
//  SkillCatalog.h — Montagem de builds (Build Assembler) — Camada 3
//
//  Converte os upgrades do jogador (SistemaUpgrades) + capacidades liberadas
//  (EstadoDesbloqueio) em uma SkillData válida. Substitui as seis funções
//  aplicarModificadorXxx() do antigo SkillManager.h por composição de dados.
//
//  PARIDADE: os números aqui reproduzem EXATAMENTE o comportamento do sistema
//  atual (calcularAtributosBase + aplicarModificadorDano/Cadencia/Perfuracao/
//  Tensao/Velocidade/Vida), de modo que a build base e suas evoluções por
//  upgrade fiquem idênticas ao jogo antes da migração. A diferença é que agora
//  a Forma resultante (Cone/Radial) é dado, não classe.
//
//  C++98: funções inline, sem STL associativa.
// ===========================================================================

#include "SkillTypes.h"
#include "SkillValidator.h"
#include "Entities.h"
#include <cstring>   // memset

// ---------------------------------------------------------------------------
// buildBase — a SkillData mínima sempre válida: o tiro simples do jogo atual.
//   Espelha calcularAtributosBase(): dano 1, vel 20, raio 0.20, 1 tiro.
// ---------------------------------------------------------------------------
inline SkillData buildBase() {
    SkillData s;
    std::memset(&s, 0, sizeof(s));
    s.id = 0;

    s.forma.tipo         = FORMA_PROJECTILE;
    s.forma.raioColisao  = 0.20f;
    s.forma.quantidade   = 1;
    s.forma.spreadAngulo = 0.15f;
    s.forma.perfuracao   = 0;

    s.movimento.tipo        = MOV_LINEAR;
    s.movimento.velocidade  = 20.0f;

    s.origem.tipo           = ORIG_STAND;
    s.origem.reavaliarPorFrame = false;

    s.efeitos[0].tipo  = EFE_DAMAGE;
    s.efeitos[0].valor = 1;
    s.numEfeitos       = 1;

    s.cooldown    = 0.0f;     // disparo manual (clique), como hoje
    s.custoTensao = 5.0f;     // +5 por clique, preservado
    s.ativa       = true;
    return s;
}

// ---------------------------------------------------------------------------
// Injeções numéricas por upgrade — PARIDADE com aplicarModificadorXxx().
//   Cada função recebe a SkillData em construção e o nível (0..3) do upgrade.
// ---------------------------------------------------------------------------

// DANO: nível 1→3, 2→7, 3→20 de dano; raio ×1.5 / ×2.2 / ×3.5.
inline void injDano(SkillData& s, int nivel) {
    if (nivel <= 0) return;
    int dano = 1;
    if (nivel == 1) dano = 3; else if (nivel == 2) dano = 7; else dano = 20;
    s.efeitos[0].valor = dano;
    float mult = 1.0f;
    if (nivel == 1) mult = 1.5f; else if (nivel == 2) mult = 2.2f; else mult = 3.5f;
    s.forma.raioColisao *= mult;
}

// CADENCIA: 1→2 tiros (Cone), 2→5 tiros (Cone), 3→8 tiros (Ring radial 360°).
inline void injCadencia(SkillData& s, int nivel) {
    if (nivel <= 0) return;
    if (nivel == 1) {
        s.forma.tipo = FORMA_CONE; s.forma.quantidade = 2;
    } else if (nivel == 2) {
        s.forma.tipo = FORMA_CONE; s.forma.quantidade = 5;
    } else {
        // 8 tiros em 360° = Ring com spread total. Mantém Linear (cada raio voa).
        s.forma.tipo = FORMA_CONE; s.forma.quantidade = 8;
        s.forma.spreadAngulo = (6.28318531f / 8.0f); // distribuição radial completa
    }
}

// PERFURACAO: 1→1, 2→5, 3→9999.
inline void injPerfuracao(SkillData& s, int nivel) {
    if (nivel <= 0) return;
    if (nivel == 1) s.forma.perfuracao = 1;
    else if (nivel == 2) s.forma.perfuracao = 5;
    else s.forma.perfuracao = 9999;
}

// TENSAO: adiciona efeito Drain; afina o raio (×0.85/0.75/0.65).
inline void injTensao(SkillData& s, int nivel) {
    if (nivel <= 0) return;
    // adiciona Drain se ainda houver espaço e não existir
    bool jaTem = false;
    for (int i = 0; i < s.numEfeitos; ++i)
        if (s.efeitos[i].tipo == EFE_DRAIN) jaTem = true;
    if (!jaTem && s.numEfeitos < MAX_EFEITOS_POR_SKILL) {
        s.efeitos[s.numEfeitos].tipo  = EFE_DRAIN;
        s.efeitos[s.numEfeitos].valor = 3;
        s.numEfeitos++;
    }
    s.forma.raioColisao *= (nivel == 1 ? 0.85f : nivel == 2 ? 0.75f : 0.65f);
}

// VELOCIDADE: +6 / +16 / +20 na velocidade do projétil.
inline void injVelocidade(SkillData& s, int nivel) {
    if (nivel <= 0) return;
    float b = 0.0f;
    if (nivel == 1) b = 6.0f; else if (nivel == 2) b = 16.0f; else b = 20.0f;
    s.movimento.velocidade += b;
}

// VIDA: adiciona efeito Heal; projétil levemente mais lento/maior.
inline void injVida(SkillData& s, int nivel) {
    if (nivel <= 0) return;
    bool jaTem = false;
    for (int i = 0; i < s.numEfeitos; ++i)
        if (s.efeitos[i].tipo == EFE_HEAL) jaTem = true;
    if (!jaTem && s.numEfeitos < MAX_EFEITOS_POR_SKILL) {
        s.efeitos[s.numEfeitos].tipo   = EFE_HEAL;
        s.efeitos[s.numEfeitos].chance = 0.30f;
        s.numEfeitos++;
    }
    s.movimento.velocidade = (s.movimento.velocidade > 4.0f)
                             ? s.movimento.velocidade - 4.0f
                             : s.movimento.velocidade;
    s.forma.raioColisao *= 1.15f;
}

// ---------------------------------------------------------------------------
// montarBuild — Build Assembler completo (Parte 7.1).
//   Aplica injeções na ordem dos upgrades, valida e rebaixa se necessário.
//   Recebe os desbloqueios para garantir que a forma resultante é permitida;
//   na ausência de desbloqueios de Cadência (Cone/Ring), o fallback mantém
//   Projectile — preservando o comportamento base.
// ---------------------------------------------------------------------------
inline SkillData montarBuild(const SistemaUpgrades& up,
                             const EstadoDesbloqueio& d) {
    SkillData s = buildBase();

    injDano       (s, up.niveis[DANO]);
    injCadencia   (s, up.niveis[CADENCIA]);
    injPerfuracao (s, up.niveis[PERFURACAO]);
    injTensao     (s, up.niveis[TENSAO_UP]);
    injVelocidade (s, up.niveis[VELOCIDADE]);
    injVida       (s, up.niveis[VIDA]);

    // Garante coerência: se a forma escolhida não está liberada ou a
    // combinação é inválida, rebaixa para algo válido (nunca quebra o jogo).
    if (validarSkill(s, d) != VALID_OK)
        rebaixarParaCompativel(s, d);

    return s;
}

// ---------------------------------------------------------------------------
// liberarPorCadencia — exemplo de hook de desbloqueio chamado no level-up:
//   quando o jogador sobe Cadência, libera as formas Cone/Ring correspondentes.
//   (As demais tabelas de desbloqueio da Parte 5.4 entram aqui conforme o
//    conteúdo for ativado; mantido mínimo para não alterar balanceamento.)
// ---------------------------------------------------------------------------
inline void liberarPorUpgrade(EstadoDesbloqueio& d, TipoUpgrade tipo, int novoNivel) {
    switch (tipo) {
        case CADENCIA:
            if (novoNivel >= 1) liberar(d.formasLiberadas, FORMA_CONE);
            if (novoNivel >= 3) liberar(d.formasLiberadas, FORMA_RING);
            break;
        case TENSAO_UP:
            if (novoNivel >= 1) liberar(d.efeitosLiberados, EFE_DRAIN);
            break;
        case VIDA:
            if (novoNivel >= 1) liberar(d.efeitosLiberados, EFE_HEAL);
            break;
        default:
            break;
    }
}

// ---------------------------------------------------------------------------
// aplicarUpgradesNaSkill — aplica os upgrades do jogador sobre uma SkillData
//   base (vinda da SkillFactory), retornando a build resultante.
//   Usado por SkillManager::selecionarSkillBase() e ProgressionSystem.h.
// ---------------------------------------------------------------------------
inline SkillData aplicarUpgradesNaSkill(SkillData base,
                                        const SistemaUpgrades& up,
                                        const EstadoDesbloqueio& d) {
    injDano       (base, up.niveis[DANO]);
    injCadencia   (base, up.niveis[CADENCIA]);
    injPerfuracao (base, up.niveis[PERFURACAO]);
    injTensao     (base, up.niveis[TENSAO_UP]);
    injVelocidade (base, up.niveis[VELOCIDADE]);
    injVida       (base, up.niveis[VIDA]);

    if (validarSkill(base, d) != VALID_OK)
        rebaixarParaCompativel(base, d);
    return base;
}

#endif // SKILL_CATALOG_H