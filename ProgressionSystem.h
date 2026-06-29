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

    // NOVO: duração do projétil escala com perfuração total acumulada.
    // perfuracao 0 → 1.0s, cada ponto extra soma 0.5s (cap em 4.0s).
    if (s.forma.tipo == FORMA_PROJECTILE ||
        s.forma.tipo == FORMA_CONE       ||
        s.forma.tipo == FORMA_PRISM) {
        float duracao = 1.0f + s.forma.perfuracao * 0.5f;
        if (duracao > 4.0f) duracao = 4.0f;
        s.forma.duracao = duracao;
    }

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

    // NOVO: mesma lógica de duração — aplicada após o += nivel acima.
    if (s.forma.tipo == FORMA_PROJECTILE ||
        s.forma.tipo == FORMA_CONE       ||
        s.forma.tipo == FORMA_PRISM) {
        float duracao = 1.0f + s.forma.perfuracao * 0.5f;
        if (duracao > 4.0f) duracao = 4.0f;
        s.forma.duracao = duracao;
    }
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
//  PARTE E.0 — TRANSFORMAÇÃO DO ARQUÉTIPO NA SKILL DE DISPARO
//
//  Chamado como ÚLTIMO passo em montarBuildCompleta(), exclusivamente para o
//  slot do Disparo (idSkill == idDisparo). Os upgrades clássicos e os atributos
//  globais já foram aplicados; o arquétipo sobrepõe a camada mecânica.
//
//  ATENÇÃO AOS NOMES DOS ENUMS (legado — nomes ≠ mecânica):
//   ARQ_CANHAO_ROTATIVO     = Cadência + Perfuração → Laser Contínuo
//   ARQ_RIFLE_LASER         = Dano + Perfuração     → Sniper
//   ARQ_CANHAO_FRAGMENTACAO = Balas + Perfuração    → Bumerangue
//
//  Os 3 outros arquétipos ficam como TODO explícito.
// ===========================================================================
inline SkillData aplicarArquetipoNaSkill(SkillData build,
                                          const EstadoArquetipo& arq,
                                          int idSkill,
                                          int idDisparo,
                                          const SistemaUpgrades& upgrades) {
    if (!arq.especializado) return build;
    if (idSkill != idDisparo) return build;

    switch (arq.arquetipo) {

        // -----------------------------------------------------------------
        // Cadência + Perfuração → ARQ_CANHAO_ROTATIVO → Laser Contínuo
        //
        //  - FORMA_BEAM estacionário seguindo a mira, auto-fire a 10Hz
        //  - ~10 tensão/s (1.0 por spawn × 10Hz, cobrado no emitter)
        //  - Atravessa todos os inimigos do segmento (PERFURACAO_INFINITA)
        //  - Metade do dano (balanceamento)
        // -----------------------------------------------------------------
        case ARQ_CANHAO_ROTATIVO: {
            int nivelCad = upgrades.niveis[CADENCIA];

            // Desfaz a penalidade de dano que Cadência aplica em aplicarUpgradesNaSkill.
            // No laser, Cadência aumenta a taxa de ticks (DPS), não reduz o dano.
            if (nivelCad > 0) {
                float penalidade = 1.0f - 0.10f * (float)nivelCad;
                if (penalidade < 0.1f) penalidade = 0.1f;
                float desfazer = 1.0f / penalidade;
                for (int i = 0; i < build.numEfeitos; ++i) {
                    if (build.efeitos[i].tipo == EFE_DAMAGE ||
                        build.efeitos[i].tipo == EFE_SHOCK) {
                        build.efeitos[i].valor =
                            (int)((float)build.efeitos[i].valor * desfazer + 0.5f);
                    }
                }
            }

            // Grossura escala com o atributo Balas (forma.quantidade já aplicado pelos upgrades).
            // qtd=1(nv0):0.8  qtd=3(nv1):1.56  qtd=6(nv2):2.7  qtd=8(nv3):3.46
            int qtd = (build.forma.quantidade > 0) ? build.forma.quantidade : 1;
            float largura = 0.8f + (qtd - 1) * 0.38f;

            // Cadência escala a taxa de ticks (mais cadência = mais DPS).
            // nv0:0.25s(4/s)  nv1:0.19s(5.3/s)  nv2:0.125s(8/s)  nv3:0.063s(16/s)
            float tickFinal = 0.25f * fatorCadenciaDoNivel(nivelCad);

            build.forma.tipo          = FORMA_BEAM;
            build.forma.comprimento   = 45.0f;
            build.forma.largura       = largura;
            build.forma.tickIntervalo = tickFinal;
            build.forma.duracao       = 0.0f;
            build.forma.perfuracao    = PERFURACAO_INFINITA;
            build.forma.quantidade    = 1;
            build.forma.spreadAngulo  = 0.0f;

            build.movimento.tipo         = MOV_STATIONARY;
            build.movimento.seguirOrigem = true;
            build.movimento.velocidade   = 0.0f;

            build.origem.tipo              = ORIG_STAND;
            build.origem.reavaliarPorFrame = true;

            build.cooldown    = 0.033f;
            build.custoTensao = 1.0f;
            break;
        }

        // -----------------------------------------------------------------
        // Dano + Perfuração → ARQ_RIFLE_LASER → Sniper
        //
        //  - Cadência reduzida à metade (auto-fire a 1s vs base 0.5s)
        //  - Projétil maior (raio de colisão × 2 → visual + hitbox)
        //  - Cor vermelha (campo FormaData.corR/G/B lido por SkillRender)
        //  - Dano e perfuração já escalados pelos upgrades acima
        //  - Sem custo de tensão por tiro (compensação pela cadência baixa)
        // -----------------------------------------------------------------
        case ARQ_RIFLE_LASER: {
            // Disparo manual (cooldown=0) pelo clique padrão do jogador.
            // Projétil maior e vermelho compensa a perda da perfuração extra.
            build.cooldown    = 0.0f;
            build.custoTensao = 20.0f;  // dobro do custo normal (10 base → 20 sniper)

            // Projétil maior: colisão E visual crescem juntos (raioColisao × 2)
            build.forma.raioColisao *= 2.0f;

            // Cor vermelha — lida por SkillRender no case FORMA_PROJECTILE
            build.forma.corR = 1.0f;
            build.forma.corG = 0.0f;
            build.forma.corB = 0.0f;
            break;
        }

        // -----------------------------------------------------------------
        // Balas + Perfuração → ARQ_CANHAO_FRAGMENTACAO → Bumerangue
        //
        //  - Projéteis mudam para MOV_BOOMERANG (vai e volta)
        //  - Metade do dano na VOLTA é aplicado genericamente em
        //    resolverEfeitos() (SkillExecutor.h) via r.fase == 1,
        //    sem qualquer referência ao arquétipo.
        //  - Continua sendo disparo manual (cooldown = 0, mouse click)
        // -----------------------------------------------------------------
        case ARQ_CANHAO_FRAGMENTACAO: {
            build.movimento.tipo   = MOV_BOOMERANG;
            build.forma.alcanceMax = 15.0f;  // range curto para retorno rápido
            // cooldown = 0 mantido: disparo manual via mouse click
            break;
        }

        // -----------------------------------------------------------------
        // Cadência + Dano → ARQ_METRALHADORA_PESADA → Metralhadora
        //
        //  - Cadência é atributo primário: desfaz sua penalidade de -10%/nível no
        //    dano para que o dano alto do atributo Dano seja preservado.
        //  - Perfuração zerada: cada bala para no primeiro inimigo.
        //  - 15 tensão por tiro: cadência alta queima a barra rapidamente.
        //  - Projétil vermelho.
        //  - Manual: a taxa de disparo vem do gate de cadência em Main.cpp.
        // -----------------------------------------------------------------
        case ARQ_METRALHADORA_PESADA: {
            int nivelCad = upgrades.niveis[CADENCIA];
            // Desfaz penalidade de dano do upgrade de Cadência
            if (nivelCad > 0) {
                float pen = 1.0f - 0.10f * (float)nivelCad;
                if (pen < 0.1f) pen = 0.1f;
                float desfazer = 1.0f / pen;
                for (int i = 0; i < build.numEfeitos; ++i) {
                    if (build.efeitos[i].tipo == EFE_DAMAGE ||
                        build.efeitos[i].tipo == EFE_SHOCK) {
                        build.efeitos[i].valor =
                            (int)((float)build.efeitos[i].valor * desfazer + 0.5f);
                    }
                }
            }
            build.forma.raioColisao *= 2.0f;
            build.forma.perfuracao = 0;    // bala para no 1º inimigo
            build.custoTensao      = 15.0f;
            build.forma.corR = 1.0f; build.forma.corG = 0.0f; build.forma.corB = 0.0f;
            // cooldown = 0 mantido: disparo manual
            break;
        }

        // -----------------------------------------------------------------
        // Dano + Balas → ARQ_ESPINGARDA_TATICA → Escopeta
        //
        //  - Dano ×2 sobre o valor final (cada perdigão já é forte).
        //  - Projéteis ×1.8 maiores: impacto visual e hitbox.
        //  - Spread aumentado para leque de escopeta (0.35 rad entre perdigões).
        //  - Cadência reduzida a 1/4: cooldownManual = 2.0s (lido em Main.cpp).
        //  - Projétil vermelho.
        //  - Manual.
        // -----------------------------------------------------------------
        case ARQ_ESPINGARDA_TATICA: {
            for (int i = 0; i < build.numEfeitos; ++i) {
                if (build.efeitos[i].tipo == EFE_DAMAGE ||
                    build.efeitos[i].tipo == EFE_SHOCK) {
                    build.efeitos[i].valor *= 2;
                }
            }
            build.forma.raioColisao *= 4.0f;
            build.forma.spreadAngulo = 0.35f; // ~20° entre perdigões, leque de escopeta
            build.forma.corR = 1.0f; build.forma.corG = 0.0f; build.forma.corB = 0.0f;
            build.cooldownManual = 2.0f; // 4× mais lento que o base (0.5s × 4 = 2.0s)
            // cooldown = 0: disparo manual; quantidade vem do atributo Balas
            break;
        }

        // -----------------------------------------------------------------
        // Cadência + Balas → ARQ_METRALHADORA_LEVE → Arco Viajante
        //
        //  UM projétil FORMA_ARC que voa em MOV_LINEAR como bala normal.
        //  Não é leque de N projéteis; Balas não gera múltiplos — o atributo
        //  apenas escala o dano via os upgrades já aplicados.
        //
        //  Cadência: desfaz penalidade de -10%/nível no dano (Cadência é o
        //  atributo primário do arquétipo).
        //  Dano ×0.5. 5 tensão/tiro. Azul claro. Manual.
        //
        //  Geometria do arco:
        //    raioInterno = 0         → setor cheio (sem buraco central)
        //    raioExterno = 4.0       → raio do semicírculo (ajustável)
        //    anguloAbertura = PI     → 180° (semicírculo à frente)
        //
        //  Movimento:
        //    velocidade = 52 u/s    → igual ao projétil base
        //    duracao = alcanceMax/52 → mesma distância que o projétil base
        //    tickIntervalo = 0.05s  → frequência de dano durante a passagem
        //      (arco passa por inimigo em ~raioExterno/vel ≈ 0.077s → ~1 hit)
        //
        //  Pipeline de colisão:
        //    atualizarForma(FORMA_ARC) sinc r.centro = r.posicao a cada frame.
        //    dentroDoArco(z.posicao, r.centro, ...) checa a posição ATUAL. ✓
        // -----------------------------------------------------------------
        case ARQ_METRALHADORA_LEVE: {
            const float PI  = 3.14159265f;
            const float VEL = 52.0f;

            // Desfaz penalidade de dano de Cadência
            int nivelCadL = upgrades.niveis[CADENCIA];
            if (nivelCadL > 0) {
                float pen = 1.0f - 0.10f * (float)nivelCadL;
                if (pen < 0.1f) pen = 0.1f;
                float desfazer = 1.0f / pen;
                for (int i = 0; i < build.numEfeitos; ++i) {
                    if (build.efeitos[i].tipo == EFE_DAMAGE ||
                        build.efeitos[i].tipo == EFE_SHOCK) {
                        build.efeitos[i].valor =
                            (int)((float)build.efeitos[i].valor * desfazer + 0.5f);
                    }
                }
            }
            // Dano ×0.5
            for (int i = 0; i < build.numEfeitos; ++i) {
                if (build.efeitos[i].tipo == EFE_DAMAGE ||
                    build.efeitos[i].tipo == EFE_SHOCK) {
                    build.efeitos[i].valor = (build.efeitos[i].valor + 1) / 2;
                    if (build.efeitos[i].valor < 1) build.efeitos[i].valor = 1;
                }
            }

            // alcanceMax já reflete upgrades de Perfuração aplicados antes
            float alcance = (build.forma.alcanceMax > 1.0f) ? build.forma.alcanceMax : 50.0f;

            build.forma.tipo           = FORMA_ARC;
            build.forma.raioInterno    = 0.0f;        // setor cheio: sem buraco
            build.forma.raioExterno    = 4.0f;        // raio do semicírculo
            build.forma.anguloAbertura = PI;           // 180° fixo
            build.forma.duracao        = alcance / VEL; // expira quando chega no alcance
            build.forma.tickIntervalo  = 0.05f;        // dano frequente na passagem
            build.forma.perfuracao     = PERFURACAO_INFINITA;
            build.forma.quantidade     = 1;            // SEMPRE um arco; Balas ≠ múltiplos
            build.forma.spreadAngulo   = 0.0f;
            build.movimento.tipo       = MOV_LINEAR;
            build.movimento.velocidade = VEL;
            build.origem.reavaliarPorFrame = false;
            build.custoTensao          = 5.0f;
            build.forma.corR = 0.4f; build.forma.corG = 0.8f; build.forma.corB = 1.0f;
            break;
        }

        default:
            break;
    }

    return build;
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
// Overload principal: recebe EstadoArquetipo e aplica transformação mecânica.
inline void montarBuildCompleta(const InventarioSkills& inv,
                                const SistemaUpgrades& upgrades,
                                SkillManager& skills,
                                const EstadoArquetipo& arq) {
    skills.limparTodos();

    EstadoDesbloqueio d;
    inicializarDesbloqueio(d);
    for (int t = 0; t < TOTAL_UPGRADES; ++t)
        liberarPorUpgrade(d, (TipoUpgrade)t, upgrades.niveis[t]);

    // Cache do id do Disparo para aplicarArquetipoNaSkill()
    int idDisparo = SkillFactory::id("Disparo");

    for (int slot = 0; slot < inv.numEquipadas; ++slot) {
        int idSkill = inv.equipadas[slot];
        if (idSkill < 0) continue;

        SkillData build;
        int armaInt = armaInteligenteDeId(idSkill);
        bool ehArmaInt = (armaInt >= 0);

        if (ehArmaInt) {
            int nv = inv.nivelSkill[idSkill];
            if (nv < 1) nv = 1;
            if (nv > NIVEL_MAX_ARMA_INTELIGENTE) nv = NIVEL_MAX_ARMA_INTELIGENTE;
            build = buildArmaInteligente(armaInt, nv);
        } else {
            build = SkillFactory::criar(idSkill);
            if (!build.ativa) build = buildBase();
        }

        // 1. Upgrades clássicos (DANO, CADENCIA, PERFURACAO, QUANTIDADE)
        //    Armas Inteligentes são isentas: têm progressão própria por nível e
        //    os upgrades de atributo quebrariam a build (ex: QUANTIDADE sobrescreve
        //    forma.quantidade; PERFURACAO quebraria o limite do Míssil Guiado).
        if (!ehArmaInt)
            build = aplicarUpgradesNaSkill(build, upgrades, d);

        // 2. Nível de evolução da skill (não aplicado em Armas Inteligentes)
        if (!ehArmaInt)
            build = aplicarNivelSkill(build, inv.nivelSkill[idSkill]);

        // 3. Atributos globais do inventário (isentos para Armas Inteligentes
        //    pela mesma razão: bonusPerfuracao quebraria o Míssil Guiado)
        if (!ehArmaInt)
            build = aplicarAtributosGlobaisNaSkill(build, inv.atributosGlobais);

        // 4. Transformação de Arquétipo — ÚLTIMO passo, sobre tudo anterior.
        //    Só afeta o Disparo base (idSkill == idDisparo).
        build = aplicarArquetipoNaSkill(build, arq, idSkill, idDisparo, upgrades);

        build.id = idSkill;
        skills.registrarSlot(slot, build);
    }

    verificarSinergias(inv, upgrades, skills);
}

// Overload de compatibilidade: sem arquétipo (nenhuma transformação aplicada).
//   Mantém chamadas legadas compilando (GameLogic.h, código de teste).
inline void montarBuildCompleta(const InventarioSkills& inv,
                                const SistemaUpgrades& upgrades,
                                SkillManager& skills) {
    EstadoArquetipo neutro; inicializarArquetipo(neutro);
    montarBuildCompleta(inv, upgrades, skills, neutro);
}

// ===========================================================================
//  PARTE F — SORTEIO DE RECOMPENSAS (ETAPA 4)
// ===========================================================================

static const char* NOME_ATRIB[TOTAL_UPGRADES] = {
    "+Dano",
    "+Cadencia",
    "+Perfuracao",
    "+Balas",
    "+Tensao",
    "+Vida",
    "+Velocidade"
};

static const char* DESC_ATRIB[TOTAL_UPGRADES] = {
    "dano base*2 por nivel",
    "cadencia de disparo mais rapida",
    "projeteis atravessam mais inimigos",
    "mais projeteis por disparo em cone",
    "barra de tensao com mais limite",
    "+2 HP maximo por nivel",
    "1.25x / 1.5x / 2.0x de velocidade"
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
    for (int i = 0; i < total && i < MAX_SKILLS_CATALOGO; ++i) {
        if (estaEquipada(inv, i)) continue;
        if (_jaSorteado(menu, ESCOLHA_NOVA_SKILL, i)) continue;

        // Apenas Armas Inteligentes aparecem como nova skill no menu.
        if (!ehArmaInteligente(i)) continue;

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

        // Apenas Armas Inteligentes podem ser evoluídas pelo menu.
        if (!ehArmaInteligente(id)) continue;
        if (armaInteligenteNoMaximo(inv, id)) continue;

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
                  "+1 projetil guiado por disparo");
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

    // Slot 1: nova skill (inclui Armas Inteligentes se nenhuma equipada ainda)
    _tentarNovaSkill(menu, inv);
    // Slot 2: upgrade de skill existente (inclui evolução de Arma Inteligente)
    _tentarUpgradeSkill(menu, inv);
    // Slots restantes: atributos globais
    while (menu.quantidade < MAX_ESCOLHAS_MENU)
        if (!_tentarAtributoGlobal(menu, arq, up)) break;
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
                const int HP_POR_NIVEL = 2;
                jogador.hpMaximo += HP_POR_NIVEL;
                jogador.hp       += HP_POR_NIVEL;
                if (jogador.hp > jogador.hpMaximo) jogador.hp = jogador.hpMaximo;
            }

            // Após evoluir um atributo de arma, verifica especialização/evolução.
            verificarEspecializacao(arq, upgrades);
            break;
        }
    }

    // Remonta a build completa com arquétipo (inclui verificarSinergias)
    montarBuildCompleta(inv, upgrades, skills, arq);
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