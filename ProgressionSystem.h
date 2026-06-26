#ifndef PROGRESSION_SYSTEM_H
#define PROGRESSION_SYSTEM_H

// ===========================================================================
//  ProgressionSystem.h — Progressão Data-Driven (Fase 6 / 6.5)
//
//  RESPONSABILIDADES
//  -----------------
//  1. Modo de Teste (ETAPA 1)  — iniciar com qualquer skill pelo nome
//  2. Sorteio de Recompensas   — 3 tipos de escolha equilibrados
//  3. Aplicação de Escolhas    — executa a LevelUpChoice selecionada
//  4. Montagem de Build Completa (MULTI-SLOT, Fase 6.5)
//       — compila TODOS os slots do inventário → SkillManager.slots[]
//       — CADA skill equipada recebe seu próprio SlotSkill independente
//  5. Atributos Globais        — propaga bônus numéricos às skills e ao jogador
//  6. Sinergias (hook reservado, Fase 7+) — verificarSinergias()
//
//  MUDANÇA PRINCIPAL (Fase 6.5):
//    montarBuildCompleta() agora itera inv.equipadas[0..numEquipadas-1] e
//    chama skillManager.registrarSlot(i, build) para CADA skill. Isso elimina
//    o antigo gargalo "só o slot 0 dispara".
//
//  MODO DE TESTE (ETAPA 1) — removível em 1 linha
//  -----------------------------------------------
//  Defina SKILL_TESTE antes de incluir este header para forçar uma skill:
//    #define SKILL_TESTE "CadeiaEletrica"
//  Remova a define para voltar ao comportamento normal.
//
//  C++98: sem lambda, sem std::function, arrays fixos.
// ===========================================================================

#include "SkillTypes.h"
#include "SkillFactory.h"
#include "SkillRegistry.h"
#include "SkillInventory.h"
#include "LevelUpChoice.h"
#include "SkillManager.h"
#include "SkillCatalog.h"
#include "ArquetipoSystem.h"
#include "ArmaInteligente.h"
#include "Entities.h"
#include <cstdlib>   // rand
#include <cstdio>    // snprintf
#include <cstring>   // strncpy

// ===========================================================================
//  PARTE A — MODO DE TESTE (ETAPA 1)
// ===========================================================================

inline void activarSkillTeste(InventarioSkills& inv) {
#ifdef SKILL_TESTE
    const char* nomeSkillTeste = SKILL_TESTE;
    int idTeste = SkillFactory::id(nomeSkillTeste);
    if (idTeste >= 0) {
        inv.equipadas[0]           = idTeste;
        inv.estado[idTeste]        = SKILL_EQUIPADA;
        inv.nivelSkill[idTeste]    = 0;
        int idDisparo = SkillFactory::id("Disparo");
        if (idDisparo >= 0 && idDisparo != idTeste)
            inv.estado[idDisparo] = SKILL_DISPONIVEL;
    }
#else
    (void)inv;
#endif
}

// ===========================================================================
//  PARTE B — ATRIBUTOS GLOBAIS APLICADOS À SKILL
// ===========================================================================

inline SkillData aplicarAtributosGlobaisNaSkill(SkillData s,
                                                const AtributosGlobais& a) {
    for (int i = 0; i < s.numEfeitos; ++i) {
        if (s.efeitos[i].tipo == EFE_DAMAGE) {
            s.efeitos[i].valor = (int)(s.efeitos[i].valor * a.bonusDano + 0.5f);
            break;
        }
    }
    s.custoTensao *= a.bonusTensao;
    if (s.custoTensao < 1.0f) s.custoTensao = 1.0f;

    if (a.bonusPerfuracao > 0.0f)
        s.forma.perfuracao += (int)a.bonusPerfuracao;

    return s;
}

// ===========================================================================
//  PARTE C — NÍVEL DE EVOLUÇÃO DA SKILL
// ===========================================================================

inline SkillData aplicarNivelSkill(SkillData s, int nivel) {
    if (nivel <= 0) return s;

    float fatorDano = 1.0f + 0.30f * nivel;
    float fatorRaio = 1.0f + 0.10f * nivel;
    float fatorVel  = 1.0f + 0.05f * nivel;

    for (int i = 0; i < s.numEfeitos; ++i) {
        if (s.efeitos[i].tipo == EFE_DAMAGE ||
            s.efeitos[i].tipo == EFE_SHOCK) {
            s.efeitos[i].valor = (int)(s.efeitos[i].valor * fatorDano + 0.5f);
        }
    }
    s.forma.raioColisao    *= fatorRaio;
    s.movimento.velocidade *= fatorVel;
    s.forma.perfuracao     += nivel;

    if (nivel >= 3) {
        s.custoTensao *= 0.80f;
        if (s.custoTensao < 1.0f) s.custoTensao = 1.0f;
    }
    if (nivel >= 5) {
        s.custoTensao = 1.0f;
    }
    return s;
}

// ===========================================================================
//  PARTE D — SINERGIAS (HOOK RESERVADO — Fase 7+)
//
//  Ponto de extensão central para sinergias entre skills.
//  NUNCA espalhe lógica de sinergia fora desta função.
//
//  Chamado automaticamente por:
//    - montarBuildCompleta()   (ao remontar toda a build)
//    - aplicarEscolhaLevelUp() (ao equipar, evoluir ou remover uma skill)
//
//  Para implementar uma sinergia na Fase 7:
//    1. Adicione detecção aqui (ex: "se Laser + Meteoro equipados → ...").
//    2. Modifique os slots afetados via skillManager.registrarSlot().
//    3. NÃO adicione lógica de sinergia em GameLogic.h, SkillManager.h
//       ou qualquer outro arquivo.
//
//  Assinatura:
//    inv        — inventário atual (para checar quais skills estão equipadas)
//    upgrades   — upgrades clássicos do jogador
//    skills     — SkillManager (para modificar slots se necessário)
// ===========================================================================
inline void verificarSinergias(const InventarioSkills& inv,
                                const SistemaUpgrades& /*upgrades*/,
                                SkillManager& /*skills*/) {
    // PLACEHOLDER — nenhuma sinergia implementada até a Fase 7.
    // Exemplo de como será implementado:
    //
    //   int idLaser   = SkillFactory::id("LancaDeLuz");
    //   int idMeteoro = SkillFactory::id("PilarFogo");
    //   bool temLaser   = estaEquipada(inv, idLaser);
    //   bool temMeteoro = estaEquipada(inv, idMeteoro);
    //   if (temLaser && temMeteoro) {
    //       // aplica bônus: busca o slot do Laser e aumenta dano
    //       // skills.registrarSlot(slotDoLaser, buildLaserAmpliado);
    //   }
    (void)inv;
}

// ===========================================================================
//  PARTE E — MONTAGEM DA BUILD COMPLETA (MULTI-SLOT, Fase 6.5)
//
//  Itera TODOS os slots do inventário (0..numEquipadas-1).
//  Cada skill equipada recebe:
//    1. Modelo base da SkillFactory
//    2. Upgrades clássicos de atributo aplicados
//    3. Nível de evolução da skill aplicado
//    4. Atributos globais aplicados
//    5. Registrado em skillManager.registrarSlot(i, build)
//
//  Após montar todos os slots, chama verificarSinergias().
//
//  Esta é a ÚNICA função que deve chamar skillManager.registrarSlot().
//  Nunca chame reconstruirHabilidade() ou selecionarSkillBase() em código novo.
// ===========================================================================
inline void montarBuildCompleta(const InventarioSkills& inv,
                                const SistemaUpgrades& upgrades,
                                SkillManager& skills) {
    // Limpa todos os slots antes de reconfigurar
    skills.limparTodos();

    // Constrói os desbloqueios a partir dos upgrades clássicos
    EstadoDesbloqueio d;
    inicializarDesbloqueio(d);
    for (int t = 0; t < TOTAL_UPGRADES; ++t)
        liberarPorUpgrade(d, (TipoUpgrade)t, upgrades.niveis[t]);

    // Itera cada skill equipada e registra seu slot
    for (int slot = 0; slot < inv.numEquipadas; ++slot) {
        int idSkill = inv.equipadas[slot];
        if (idSkill < 0) continue;

        // 1. Obtém o modelo base da skill.
        //    Armas Inteligentes têm BUILD EXPLÍCITA por nível (não escala
        //    genérica): cada nível muda comportamento conforme o spec.
        SkillData build;
        int armaInt = armaInteligenteDeId(idSkill);
        bool ehArmaInt = (armaInt >= 0);

        if (ehArmaInt) {
            int nv = inv.nivelSkill[idSkill];
            if (nv < 1) nv = 1;                       // equipada = nível 1
            if (nv > NIVEL_MAX_ARMA_INTELIGENTE) nv = NIVEL_MAX_ARMA_INTELIGENTE;
            build = buildArmaInteligente(armaInt, nv);
        } else {
            build = SkillFactory::criar(idSkill);
            if (!build.ativa) build = buildBase();    // fallback seguro
        }

        // 2. Aplica upgrades clássicos de atributo (DANO, CADENCIA, etc.)
        //    sobre a base desta skill específica
        build = aplicarUpgradesNaSkill(build, upgrades, d);

        // 3. Nível de evolução:
        //    - Armas Inteligentes JÁ trazem o comportamento do nível na build
        //      explícita (passo 1), então NÃO recebem a escala genérica.
        //    - Demais skills usam a escala genérica por nível.
        if (!ehArmaInt)
            build = aplicarNivelSkill(build, inv.nivelSkill[idSkill]);

        // 4. Aplica atributos globais do inventário
        build = aplicarAtributosGlobaisNaSkill(build, inv.atributosGlobais);

        // 5. Garante que o id da build reflita o slot (para o executor)
        build.id = idSkill;

        // 6. Registra no slot correspondente do SkillManager
        skills.registrarSlot(slot, build);
    }

    // 7. Verifica sinergias após todos os slots estarem configurados
    verificarSinergias(inv, upgrades, skills);
}

// ===========================================================================
//  PARTE F — SORTEIO DE RECOMPENSAS (ETAPA 4)
// ===========================================================================

static const char* NOME_ATRIB[TOTAL_UPGRADES] = {
    "+Dano",
    "+Cadencia",
    "+Perfuracao",
    "+Eficiencia",
    "+Velocidade",
    "+Vida",
    "+Balas"
};

static const char* DESC_ATRIB[TOTAL_UPGRADES] = {
    "+10% de dano em todos os ataques",
    "+20% Vel. de disparo e -10% custo de tensao, mas -10% de dano base",
    "+1 perfuracao em todos os ataques",
    "-10% custo de tensao por disparo",
    "+20% velocidade de movimento",
    "+15% HP maximo e +1 HP",
    "+1 projetil por disparo"
};

static const EscolhaRaridade RARIDADE_ATRIB[TOTAL_UPGRADES] = {
    RARIDADE_INCOMUM,
    RARIDADE_COMUM,
    RARIDADE_RARA,
    RARIDADE_INCOMUM,
    RARIDADE_COMUM,
    RARIDADE_RARA,
    RARIDADE_INCOMUM
};

static bool _jaSorteado(const MenuLevelUp& menu, EscolhaTipo tipo, int ref) {
    for (int i = 0; i < menu.quantidade; ++i) {
        if (menu.escolhas[i].tipo == tipo &&
            menu.escolhas[i].referencia == ref) return true;
    }
    return false;
}

static bool _tentarNovaSkill(MenuLevelUp& menu, const InventarioSkills& inv) {
    int total = SkillFactory::total();
    if (total <= 0) return false;

    int candidatos[MAX_SKILLS_CATALOGO];
    int nCand = 0;
    int armaIntAtual = armaInteligenteEquipada(inv);   // -1 se nenhuma
    for (int i = 0; i < total && i < MAX_SKILLS_CATALOGO; ++i) {
        if (estaEquipada(inv, i)) continue;
        if (_jaSorteado(menu, ESCOLHA_NOVA_SKILL, i)) continue;

        // Exclusividade: se já há uma arma inteligente equipada, não oferece
        // outra arma inteligente (apenas uma por partida).
        if (armaIntAtual >= 0 && ehArmaInteligente(i)) continue;

        candidatos[nCand++] = i;
    }
    if (nCand == 0) return false;

    int escolhida = candidatos[rand() % nCand];
    LevelUpChoice& c = menu.escolhas[menu.quantidade];
    c.tipo       = ESCOLHA_NOVA_SKILL;
    c.raridade   = RARIDADE_INCOMUM;
    c.referencia = escolhida;
    c.valorExtra = 0;

    const char* nome = SkillFactory::nomeDe(escolhida);
    std::snprintf(c.descricao, ESCOLHA_DESC_MAX, "Nova Skill: %s", nome);
    std::snprintf(c.subtitulo, ESCOLHA_SUB_MAX,  "Equipa %s na build", nome);
    menu.quantidade++;
    return true;
}

static bool _tentarUpgradeSkill(MenuLevelUp& menu, const InventarioSkills& inv) {
    int candidatos[MAX_SKILLS_EQUIPADAS];
    int nCand = 0;
    for (int i = 0; i < inv.numEquipadas; ++i) {
        int id = inv.equipadas[i];
        if (id < 0) continue;
        if (_jaSorteado(menu, ESCOLHA_UPGRADE_SKILL, id)) continue;

        // Arma Inteligente: teto de nível 3 (spec). Ao atingi-lo, não aparece
        // mais como opção de evolução.
        if (ehArmaInteligente(id)) {
            if (armaInteligenteNoMaximo(inv, id)) continue;
        } else {
            if (!podeEvolir(inv, id)) continue;
        }
        candidatos[nCand++] = id;
    }
    if (nCand == 0) return false;

    int escolhido  = candidatos[rand() % nCand];
    int nivelAtual = inv.nivelSkill[escolhido];

    LevelUpChoice& c = menu.escolhas[menu.quantidade];
    c.tipo       = ESCOLHA_UPGRADE_SKILL;
    c.raridade   = (nivelAtual >= 3) ? RARIDADE_EPICA : RARIDADE_RARA;
    c.referencia = escolhido;
    c.valorExtra = nivelAtual + 1;

    const char* nome = SkillFactory::nomeDe(escolhido);
    std::snprintf(c.descricao, ESCOLHA_DESC_MAX,
                  "Evoluir: %s  (Nivel %d -> %d)", nome, nivelAtual, nivelAtual+1);
    std::snprintf(c.subtitulo, ESCOLHA_SUB_MAX,
                  "+30%% dano, +10%% raio, +1 perfuracao");
    menu.quantidade++;
    return true;
}

static bool _tentarAtributoGlobal(MenuLevelUp& menu,
                                  const EstadoArquetipo& arq,
                                  const SistemaUpgrades& up) {
    int candidatos[TOTAL_UPGRADES];
    int nCand = 0;
    for (int t = 0; t < TOTAL_UPGRADES; ++t) {
        if (_jaSorteado(menu, ESCOLHA_ATRIBUTO_GLOBAL, t)) continue;

        // Filtro de Arquétipo: se é atributo de arma, respeita bloqueio e teto.
        int atr = upgradeParaAtributo((TipoUpgrade)t);
        if (atr >= 0) {
            if (atributoBloqueado(arq, atr)) continue;       // travado pela especialização
            if (up.niveis[t] >= 3)           continue;       // já no nível máximo
        }
        candidatos[nCand++] = t;
    }
    if (nCand == 0) return false;

    int t = candidatos[rand() % nCand];
    LevelUpChoice& c = menu.escolhas[menu.quantidade];
    c.tipo       = ESCOLHA_ATRIBUTO_GLOBAL;
    c.raridade   = RARIDADE_ATRIB[t];
    c.referencia = t;
    c.valorExtra = 1;

    std::snprintf(c.descricao, ESCOLHA_DESC_MAX, "Atributo: %s", NOME_ATRIB[t]);
    std::snprintf(c.subtitulo, ESCOLHA_SUB_MAX,  "%s", DESC_ATRIB[t]);
    menu.quantidade++;
    return true;
}

inline void sortearRecompensas(MenuLevelUp& menu, const InventarioSkills& inv,
                               const EstadoArquetipo& arq,
                               const SistemaUpgrades& up) {
    limparMenu(menu);

    if (!_tentarNovaSkill(menu, inv))
        if (!_tentarUpgradeSkill(menu, inv))
            _tentarAtributoGlobal(menu, arq, up);

    if (menu.quantidade < MAX_ESCOLHAS_MENU) {
        if (!_tentarUpgradeSkill(menu, inv))
            if (!_tentarNovaSkill(menu, inv))
                _tentarAtributoGlobal(menu, arq, up);
    }

    if (menu.quantidade < MAX_ESCOLHAS_MENU) {
        if (!_tentarAtributoGlobal(menu, arq, up))
            if (!_tentarNovaSkill(menu, inv))
                _tentarUpgradeSkill(menu, inv);
    }
}

// Overload de compatibilidade: sem estado de arquétipo (nada bloqueado).
// Mantém call sites antigos funcionando. Cria estado neutro local.
inline void sortearRecompensas(MenuLevelUp& menu, const InventarioSkills& inv) {
    EstadoArquetipo neutro; inicializarArquetipo(neutro);
    SistemaUpgrades zero;
    for (int i = 0; i < TOTAL_UPGRADES; ++i) zero.niveis[i] = 0;
    sortearRecompensas(menu, inv, neutro, zero);
}

// ===========================================================================
//  PARTE G — APLICAÇÃO DA ESCOLHA FEITA PELO JOGADOR
//
//  Executa a LevelUpChoice selecionada, modifica o inventário e remonta
//  a build completa via montarBuildCompleta(). verificarSinergias() é
//  chamado automaticamente dentro de montarBuildCompleta().
// ===========================================================================
inline void aplicarEscolhaLevelUp(const LevelUpChoice& escolha,
                                  InventarioSkills& inv,
                                  SistemaUpgrades& upgrades,
                                  SkillManager& skills,
                                  Jogador& jogador,
                                  EstadoArquetipo& arq) {
    switch (escolha.tipo) {
        case ESCOLHA_NOVA_SKILL: {
            int id = escolha.referencia;
            // Exclusividade de Arma Inteligente: se o jogador tenta equipar uma
            // arma inteligente e já há outra equipada, recusa (defesa extra
            // além do filtro do menu).
            if (ehArmaInteligente(id) && !podeEquiparArmaInteligente(inv, id))
                break;
            desbloquearSkill(inv, id);
            equiparSkill(inv, id);
            break;
        }
        case ESCOLHA_UPGRADE_SKILL: {
            int id = escolha.referencia;
            evoluirSkill(inv, id);
            break;
        }
        case ESCOLHA_ATRIBUTO_GLOBAL: {
            TipoUpgrade tipo = (TipoUpgrade)escolha.referencia;

            // Arquétipo: nunca aplica um atributo de arma bloqueado pela
            // especialização (defesa extra além do filtro do menu).
            if (upgradeBloqueado(arq, tipo))
                break;

            aplicarAtributoGlobal(inv, tipo, 1);

            if (upgrades.niveis[tipo] < 3)
                upgrades.niveis[tipo]++;

            if (tipo == VIDA) {
                int novoHP = 3 + upgrades.niveis[VIDA] * 2;
                int diff   = novoHP - jogador.hpMaximo;
                jogador.hpMaximo = novoHP;
                jogador.hp += diff;
                if (jogador.hp > jogador.hpMaximo) jogador.hp = jogador.hpMaximo;
            }
            if (tipo == VELOCIDADE) {
                const float vels[4] = {10.f, 13.f, 18.f, 28.f};
                int nv = upgrades.niveis[VELOCIDADE];
                if (nv > 3) nv = 3;
                jogador.velocidade = vels[nv];
                jogador.velocidade *= inv.atributosGlobais.bonusVelocidade;
            }

            // Após evoluir um atributo de arma, verifica especialização/evolução.
            verificarEspecializacao(arq, upgrades);
            break;
        }
    }

    // Remonta a build completa com TODOS os slots (inclui verificarSinergias)
    montarBuildCompleta(inv, upgrades, skills);
}

// Overload de compatibilidade (sem estado de arquétipo).
inline void aplicarEscolhaLevelUp(const LevelUpChoice& escolha,
                                  InventarioSkills& inv,
                                  SistemaUpgrades& upgrades,
                                  SkillManager& skills,
                                  Jogador& jogador) {
    EstadoArquetipo descartavel; inicializarArquetipo(descartavel);
    aplicarEscolhaLevelUp(escolha, inv, upgrades, skills, jogador, descartavel);
}

#endif // PROGRESSION_SYSTEM_H