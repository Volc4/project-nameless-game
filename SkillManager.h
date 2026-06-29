#ifndef SKILL_MANAGER_H
#define SKILL_MANAGER_H

// ===========================================================================
//  SkillManager.h — Orquestrador Multi-Skill Data-Driven (Fase 6.5)
//
//  REESCRITO (Fase 6.5): suporte a N skills equipadas simultaneamente.
//
//  ARQUITETURA ANTERIOR (Fase 5/6):
//    - catalogo : vector<SkillData>  com apenas [0] usado
//    - pool     : vector<RuntimeSkill> único compartilhado
//    → Apenas a skill do slot 0 disparava; as demais eram ignoradas.
//
//  ARQUITETURA ATUAL (Fase 6.5):
//    - slots[MAX_SLOTS_SKILL] : array de SlotSkill POD
//      Cada SlotSkill possui:
//        SkillData     build        — configuração imutável desta skill
//        vector<RuntimeSkill> pool  — instâncias vivas desta skill
//        RuntimeEmitter emitter     — emissor periódico (Orbit/Aura/Periodic)
//        float cooldownRestante     — timer até próximo disparo automático
//        bool  ativo                — slot em uso
//    - numSlotsAtivos              — quantos slots estão populados
//
//  CICLO DE VIDA:
//    inicializarJogo()
//      └─ montarBuildCompleta()         [ProgressionSystem.h]
//           └─ registrarSlot()           [SkillManager]
//                 → popula slots[] com builds do inventário
//
//    executar()       — dispara skills instantâneas (clique)
//    atualizarTodos() — tick de emitters + movimento + colisão de todos os slots
//
//  REGRAS:
//    - Nunca switch por índice de slot.
//    - Cada slot é completamente independente (pool, emitter, cooldown).
//    - SpatialGrid continua sendo a única fonte de colisão.
//    - Skills automáticas (cooldown > 0 ou AURA/ORBIT) disparam via emitter.
//    - Skills instantâneas (cooldown == 0) disparam via executar() por clique.
//
//  C++98: sem std::function, sem lambda, sem unordered_map.
// ===========================================================================

#include "SkillTypes.h"
#include "SkillCatalog.h"
#include "SkillValidator.h"
#include "SkillPool.h"
#include "SkillExecutor.h"
#include "Entities.h"
#include <vector>
#include <cstring>   // memset

struct GradeEspacial;    // definido em SpatialGrid.h (incluído ao final)

// ---------------------------------------------------------------------------
// Limite de slots simultâneos. Espelha _INV_MAX_EQUIPADAS de Entities.h.
// Definido aqui como constante para arrays estáticos (C++98).
// ---------------------------------------------------------------------------
#define MAX_SLOTS_SKILL   6

// ===========================================================================
//  SlotSkill — Runtime completo de UMA skill equipada
//
//  POD na parte de dados escalares (build, emitter, cooldown, ativo).
//  Os vectors são membros STL normais: construtores default são chamados.
//  Não é POD total, mas é seguro em std::vector<SlotSkill>.
// ===========================================================================
struct SlotSkill {
    SkillData              build;            // build desta skill (imutável em runtime)
    std::vector<RuntimeSkill> pool;          // instâncias vivas desta skill
    RuntimeEmitter         emitter;          // emissor periódico associado
    float                  cooldownRestante; // timer até próxima emissão automática
    bool                   ativo;            // false = slot vazio

    SlotSkill() : cooldownRestante(0.0f), ativo(false) {
        std::memset(&build,   0, sizeof(SkillData));
        std::memset(&emitter, 0, sizeof(RuntimeEmitter));
        pool.reserve(64);
    }

    // Verifica se esta skill dispara automaticamente por emitter
    // (tem cooldown definido OU é do tipo Orbit/Aura/Stationary).
    bool ehAutomatica() const {
        if (!ativo) return false;
        if (build.cooldown > 0.0f) return true;
        MovimentoType m = build.movimento.tipo;
        return (m == MOV_ORBIT || m == MOV_STATIONARY);
    }

    // Verifica se esta skill dispara por clique do jogador.
    bool ehInstantanea() const {
        if (!ativo) return false;
        return (build.cooldown == 0.0f &&
                build.movimento.tipo != MOV_ORBIT &&
                build.movimento.tipo != MOV_STATIONARY);
    }
};

// ===========================================================================
//  SkillManager — Gerencia todos os slots de skills em paralelo
// ===========================================================================
struct SkillManager {

    SlotSkill slots[MAX_SLOTS_SKILL];   // slots[0..numSlotsAtivos-1] usados
    int       numSlotsAtivos;            // quantos slots estão com ativo=true

    SkillManager() : numSlotsAtivos(0) {
        // SlotSkill tem construtor default; nenhuma inicialização adicional necessária.
    }

    // -----------------------------------------------------------------------
    // registrarSlot — popula um slot com uma build pronta.
    //   Chamado por montarBuildCompleta() (ProgressionSystem.h) para cada
    //   skill equipada no inventário. Substitui reconstruirHabilidade() e
    //   selecionarSkillBase() como ponto de entrada único de configuração.
    //
    //   indice : 0..MAX_SLOTS_SKILL-1
    //   build  : SkillData já com upgrades e nível de evolução aplicados
    // -----------------------------------------------------------------------
    void registrarSlot(int indice, const SkillData& build) {
        if (indice < 0 || indice >= MAX_SLOTS_SKILL) return;

        SlotSkill& sl = slots[indice];
        sl.pool.clear();
        sl.build            = build;
        sl.cooldownRestante = 0.0f;
        sl.ativo            = true;

        // Configura o emitter para skills automáticas
        sl.emitter.ativo       = sl.ehAutomatica();
        sl.emitter.idSkillData = build.id;
        // cooldownRestante = 0 para disparar imediatamente no primeiro frame
        // (emitters orbitais spawnam suas instâncias na primeira atualização)

        // Conta ativos (recalcula para garantir consistência)
        _recalcularNumAtivos();
    }

    // -----------------------------------------------------------------------
    // limparSlot — desativa um slot e esvazia seu pool.
    // -----------------------------------------------------------------------
    void limparSlot(int indice) {
        if (indice < 0 || indice >= MAX_SLOTS_SKILL) return;
        slots[indice].pool.clear();
        slots[indice].ativo = false;
        slots[indice].emitter.ativo = false;
        _recalcularNumAtivos();
    }

    // -----------------------------------------------------------------------
    // limparTodos — esvazia todos os slots (usado em reinício de partida).
    // -----------------------------------------------------------------------
    void limparTodos() {
        for (int i = 0; i < MAX_SLOTS_SKILL; ++i) {
            slots[i].pool.clear();
            slots[i].ativo = false;
            slots[i].emitter.ativo = false;
        }
        numSlotsAtivos = 0;
    }

    // -----------------------------------------------------------------------
    // limparInstancias — esvazia apenas os pools (projéteis), mantém builds.
    //   Útil para reinício sem ter que reconfigurar todas as skills.
    // -----------------------------------------------------------------------
    void limparInstancias() {
        for (int i = 0; i < MAX_SLOTS_SKILL; ++i)
            slots[i].pool.clear();
    }

    // -----------------------------------------------------------------------
    // executar — dispara TODAS as skills instantâneas (ativadas por clique).
    //   Skills automáticas (emitter) NÃO são disparadas aqui; elas ticarão
    //   em atualizarTodos(). Cobra tensão pela soma dos custos.
    //   Preserva: bloqueio em sobrecarga, cálculo de tensão.
    // -----------------------------------------------------------------------
    void executar(EstadoDoJogo& jogo, Vetor3D posicaoAlvo) {
        if (jogo.stand.emSobrecarga) return;

        for (int i = 0; i < MAX_SLOTS_SKILL; ++i) {
            SlotSkill& sl = slots[i];
            if (!sl.ativo) continue;
            if (!sl.ehInstantanea()) continue;
            if (sl.cooldownRestante > 0.0f) continue;

            _spawnarInstancias(sl, jogo, posicaoAlvo);

            jogo.stand.tensaoAtual += sl.build.custoTensao;
            if (jogo.stand.tensaoAtual > maxTensaoDoNivel(jogo.protagonista.upgrades.niveis[TENSAO_UP]))
                jogo.stand.tensaoAtual = maxTensaoDoNivel(jogo.protagonista.upgrades.niveis[TENSAO_UP]);
        }
    }

    // -----------------------------------------------------------------------
    // atualizarTodos — frame completo de combate para TODOS os slots.
    //   1. Tica emitters de skills automáticas (spawna novas instâncias).
    //   2. Move e colide todas as instâncias de todos os slots.
    //   3. Processa colisão jogador-zumbi.
    //   Definido fora da struct (precisa de GradeEspacial completo).
    // -----------------------------------------------------------------------
    void atualizarTodos(EstadoDoJogo& jogo, GradeEspacial& grade, float deltaTime);

    // -----------------------------------------------------------------------
    // Acesso somente-leitura para renderização.
    //   O render loop itera os slots e pede o pool de cada um.
    // -----------------------------------------------------------------------
    int numSlots() const { return MAX_SLOTS_SKILL; }

    const SlotSkill& slot(int i) const { return slots[i]; }

    // -----------------------------------------------------------------------
    // COMPATIBILIDADE LEGADA — mantidos para Main.cpp não quebrar.
    //   instancias() devolve o pool do slot 0 (para o render loop legado).
    //   buildAtiva() devolve a build do slot 0.
    //   Ambos serão removidos na Fase 7 quando o render loop for migrado.
    // -----------------------------------------------------------------------
    const std::vector<RuntimeSkill>& instancias() const {
        return slots[0].pool;
    }
    const SkillData& buildAtiva() const {
        return slots[0].build;
    }

    // -----------------------------------------------------------------------
    // reconstruirHabilidade — STUB de compatibilidade legada.
    //   O antigo sistema chamava este método para remontar a build do slot 0.
    //   Agora NÃO FAZ NADA: a build é montada exclusivamente por
    //   montarBuildCompleta() → registrarSlot(). Mantido apenas para que
    //   código que ainda compile com esta assinatura não quebre.
    //   Não chame este método em código novo.
    // -----------------------------------------------------------------------
    void reconstruirHabilidade(const SistemaUpgrades& /*upgrades*/) {
        // INTENCIONALMENTE VAZIO — ver comentário acima.
        // A build é montada em montarBuildCompleta() via registrarSlot().
    }

    // -----------------------------------------------------------------------
    // selecionarSkillBase — STUB de compatibilidade legada.
    //   Mesmo motivo de reconstruirHabilidade. NÃO FAZ NADA.
    // -----------------------------------------------------------------------
    void selecionarSkillBase(const SkillData& /*base*/,
                             const SistemaUpgrades& /*upgrades*/) {
        // INTENCIONALMENTE VAZIO — ver reconstruirHabilidade acima.
    }

private:
    // -----------------------------------------------------------------------
    // _recalcularNumAtivos — mantém numSlotsAtivos sincronizado.
    // -----------------------------------------------------------------------
    void _recalcularNumAtivos() {
        numSlotsAtivos = 0;
        for (int i = 0; i < MAX_SLOTS_SKILL; ++i)
            if (slots[i].ativo) numSlotsAtivos++;
    }

    void _empurrarInstancia(SlotSkill& sl, EstadoDoJogo& jogo, const SkillData& s,
                            Vetor3D origem, Vetor3D dir);

    // -----------------------------------------------------------------------
    // _spawnarInstancias — cria 1..N RuntimeSkill no pool do slot.
    //   Idêntico ao spawnarInstancias() anterior, mas opera sobre o SlotSkill
    //   em vez do pool global. A distribuição angular (Cone/Ring) e a
    //   configuração por tipo de movimento são preservadas.
    // -----------------------------------------------------------------------
    void _spawnarInstancias(SlotSkill& sl, EstadoDoJogo& jogo, Vetor3D alvo) {
        const SkillData& s = sl.build;
        Vetor3D origem     = jogo.posicaoPistola;   // projéteis manuais saem da pistola
        Vetor3D dirBase    = obterDirecaoNormalizada(origem, alvo);

        int n = (s.forma.quantidade > 0 ? s.forma.quantidade : 1);

        if (n == 1) {
            _empurrarInstancia(sl, jogo, s, origem, dirBase);
            return;
        }

        // Distribuição em leque simétrico para PROJECTILE/CONE ou radial para RING
        float inicio = -((n - 1) * 0.5f) * s.forma.spreadAngulo;
        for (int i = 0; i < n; ++i) {
            float dx = dirBase.x, dz = dirBase.z;
            _rotacionarXZ(dx, dz, inicio + i * s.forma.spreadAngulo);
            Vetor3D dir = {dx, 0.0f, dz};
            _empurrarInstancia(sl, jogo, s, origem, dir);
        }
    }

    // -----------------------------------------------------------------------
    // _spawnarInstanciasOrbital — spawn de N instâncias orbitais distribuídas
    //   em 360°. Chamado pelo emitter de skills MOV_ORBIT.
    // -----------------------------------------------------------------------
    void _spawnarInstanciasOrbital(SlotSkill& sl, EstadoDoJogo& jogo) {
        const SkillData& s = sl.build;
        Vetor3D origem = jogo.protagonista.posicao;  // ORIG_PLAYER para orbitais

        int n = (s.forma.quantidade > 0 ? s.forma.quantidade : 1);
        float passo = (2.0f * 3.14159265f) / (float)n;

        for (int i = 0; i < n; ++i) {
            float angulo = passo * (float)i;
            // Direção apontando para fora do centro no ângulo inicial
            Vetor3D dir = {std::cos(angulo), 0.0f, std::sin(angulo)};
            RuntimeSkill r;
            zerarRuntime(r);
            r.idSkillData        = s.id;
            r.ativo              = true;
            r.centro             = origem;
            r.posicao.x          = origem.x + std::cos(angulo) * s.movimento.raioOrbita;
            r.posicao.z          = origem.z + std::sin(angulo) * s.movimento.raioOrbita;
            r.posicao.y          = 0.0f;
            r.direcao            = dir;
            r.anguloAtual        = angulo;
            r.tempoVida          = (s.forma.duracao > 0.0f) ? s.forma.duracao : 99999.0f;
            r.dano               = (s.numEfeitos > 0) ? s.efeitos[0].valor : 1;
            r.raioColisao        = s.forma.raioColisao;
            r.perfuracaoRestante = s.forma.perfuracao;
            adicionarAoPool(sl.pool, r);
        }
    }

    // -----------------------------------------------------------------------
    // _empurrarInstancia — cria um RuntimeSkill e insere no pool do slot.
    //   Centraliza a inicialização de campos comuns a qualquer forma.
    // -----------------------------------------------------------------------

    // cópia desabilitada
    SkillManager(const SkillManager&);
    SkillManager& operator=(const SkillManager&);
};

// ===========================================================================
//  SEÇÃO FINAL — definições que precisam de GradeEspacial completo.
// ===========================================================================
#include "SpatialGrid.h"

// _acharZumbiMaisProximo — função livre, visível para _empurrarInstancia
//   e _atualizarEmitter.
inline int _acharZumbiMaisProximo(EstadoDoJogo& jogo, Vetor3D de) {
    int melhor = -1;
    float melhorDist = 1e30f;
    for (int i = 0; i < (int)jogo.horda.size(); ++i) {
        if (!jogo.horda[i].vivo) continue;
        float dx = jogo.horda[i].posicao.x - de.x;
        float dz = jogo.horda[i].posicao.z - de.z;
        float d2 = dx * dx + dz * dz;
        if (d2 < melhorDist) { melhorDist = d2; melhor = i; }
    }
    return melhor;
}

inline void SkillManager::_empurrarInstancia(SlotSkill& sl, EstadoDoJogo& jogo,
                                             const SkillData& s,
                                             Vetor3D origem, Vetor3D dir) {
    RuntimeSkill r;
    zerarRuntime(r);
    r.idSkillData        = s.id;
    r.ativo              = true;
    r.posicao            = origem;
    r.direcao            = dir;
    r.anguloAtual        = std::atan2(dir.z, dir.x); // FORMA_ARC usa para orientar o setor
    r.centro             = origem;
    r.alvo               = origem;
    r.idAlvo             = -1;
    r.tempoVida          = (s.forma.duracao > 0.0f) ? s.forma.duracao : 0.0f;
    r.dano               = (s.numEfeitos > 0) ? s.efeitos[0].valor : 1;
    r.raioColisao        = s.forma.raioColisao;
    r.perfuracaoRestante = s.forma.perfuracao;

    if (s.movimento.tipo == MOV_HOMING)
        r.idAlvo = _acharZumbiMaisProximo(jogo, origem);

    adicionarAoPool(sl.pool, r);
}

// Adaptador do contrato do executor para a API real da grade.
inline void obterVizinhosHost(GradeEspacial& grade, float x, float z,
                              std::vector<int>& saida) {
    grade.obterInimigosVizinhos(x, z, saida);
}

// ---------------------------------------------------------------------------
// _atualizarEmitter — tica o emitter de um slot e spawna instâncias quando
//   o cooldown chega a zero. Chamado por atualizarTodos() para cada slot ativo
//   com ehAutomatica() == true.
//
//   Skills MOV_ORBIT: spawnam UMA vez (pool fica vivo continuamente).
//   Skills com cooldown > 0: re-spawnam a cada cooldown segundos.
//   Skills MOV_STATIONARY (Aura/Beam): spawnam uma vez e persistem por duracao.
// ---------------------------------------------------------------------------
inline void _atualizarEmitter(SlotSkill& sl, EstadoDoJogo& jogo,
                               GradeEspacial& /*grade*/, float deltaTime) {
    const SkillData& s = sl.build;

    // Orbit: spawn inicial único; as instâncias vivem para sempre (seguem origem).
    if (s.movimento.tipo == MOV_ORBIT) {
        // Conta quantas instâncias orbitais ativas existem no pool deste slot.
        int ativas = contarAtivos(sl.pool);
        if (ativas == 0) {
            // Primeira vez ou após limpeza: spawna o anel orbital completo.
            // Não há cooldown — o anel vive enquanto a skill estiver equipada.
            // O campo tempoVida fica em 0 (sem expiração automática por tempo).

            // Usa _spawnarInstanciasOrbital via ponteiro ao SkillManager não é
            // possível diretamente (função livre). Replicamos inline aqui:
            Vetor3D origem = jogo.protagonista.posicao;
            int n = (s.forma.quantidade > 0 ? s.forma.quantidade : 1);
            float passo = (2.0f * 3.14159265f) / (float)n;
            for (int i = 0; i < n; ++i) {
                float ang = passo * (float)i;
                RuntimeSkill r;
                zerarRuntime(r);
                r.idSkillData        = s.id;
                r.ativo              = true;
                r.centro             = origem;
                r.posicao.x          = origem.x + std::cos(ang) * s.movimento.raioOrbita;
                r.posicao.z          = origem.z + std::sin(ang) * s.movimento.raioOrbita;
                r.posicao.y          = 0.0f;
                r.direcao.x          = std::cos(ang);
                r.direcao.z          = std::sin(ang);
                r.anguloAtual        = ang;
                r.tempoVida          = 0.0f;   // não expira por tempo
                r.dano               = (s.numEfeitos > 0) ? s.efeitos[0].valor : 1;
                r.raioColisao        = s.forma.raioColisao;
                r.perfuracaoRestante = s.forma.perfuracao;
                adicionarAoPool(sl.pool, r);
            }
        }
        // Atualiza o centro de órbita para seguir o jogador
        for (size_t k = 0; k < sl.pool.size(); ++k) {
            if (sl.pool[k].ativo)
                sl.pool[k].centro = jogo.protagonista.posicao;
        }
        return;
    }

    // Stationary sem cooldown (Aura/Beam de longa duração): spawn único.
    if (s.movimento.tipo == MOV_STATIONARY && s.cooldown == 0.0f) {
        if (contarAtivos(sl.pool) == 0) {
            RuntimeSkill r;
            zerarRuntime(r);
            r.idSkillData        = s.id;
            r.ativo              = true;
            r.posicao            = jogo.stand.posicao;
            r.centro             = jogo.stand.posicao;
            r.direcao.x          = 1.0f;
            r.tempoVida          = (s.forma.duracao > 0.0f) ? s.forma.duracao : 99999.0f;
            r.dano               = (s.numEfeitos > 0) ? s.efeitos[0].valor : 1;
            r.raioColisao        = s.forma.raioColisao;
            r.perfuracaoRestante = s.forma.perfuracao;
            adicionarAoPool(sl.pool, r);
        }
        // Atualiza posição para seguir o jogador se seguirOrigem
        if (s.movimento.seguirOrigem) {
            for (size_t k = 0; k < sl.pool.size(); ++k) {
                if (sl.pool[k].ativo) {
                    sl.pool[k].posicao = jogo.stand.posicao;
                    sl.pool[k].centro  = jogo.stand.posicao;
                }
            }
        }
        return;
    }

    // BEAM com cooldown > 0 — laser "manual em alta frequência".
    // Requer mouse pressionado (atirandoAgora). Mantém UMA instância persistente
    // e atualiza posição+direção continuamente; cobra tensão na taxa do cooldown.
    if (s.forma.tipo == FORMA_BEAM && s.cooldown > 0.0f) {
        if (!jogo.atirandoAgora || jogo.stand.emSobrecarga) {
            sl.pool.clear();           // remove o feixe ao soltar o mouse ou sobrecarga
            sl.cooldownRestante = 0.0f;
            return;
        }
        // Cobrança de tensão na frequência do cooldown (~30Hz)
        sl.cooldownRestante -= deltaTime;
        if (sl.cooldownRestante <= 0.0f) {
            sl.cooldownRestante = s.cooldown;
            if (s.custoTensao > 0.0f) {
                float maxT = maxTensaoDoNivel(jogo.protagonista.upgrades.niveis[TENSAO_UP]);
                jogo.stand.tensaoAtual += s.custoTensao;
                if (jogo.stand.tensaoAtual >= maxT) {
                    jogo.stand.tensaoAtual = maxT;
                    jogo.stand.emSobrecarga = true;
                }
            }
        }
        // Atualiza posição e direção do feixe a cada frame (acompanha a mira suavemente)
        bool temAtivo = false;
        for (size_t k = 0; k < sl.pool.size(); ++k) {
            if (sl.pool[k].ativo) {
                sl.pool[k].posicao   = jogo.stand.posicao;
                sl.pool[k].centro    = jogo.stand.posicao;
                sl.pool[k].direcao.x = std::cos(jogo.stand.anguloMira);
                sl.pool[k].direcao.y = 0.0f;
                sl.pool[k].direcao.z = std::sin(jogo.stand.anguloMira);
                temAtivo = true;
            }
        }
        if (!temAtivo) {
            // Cria a instância inicial do feixe
            Vetor3D orig = jogo.stand.posicao;
            RuntimeSkill r;
            zerarRuntime(r);
            r.idSkillData        = s.id;
            r.ativo              = true;
            r.posicao            = orig;
            r.centro             = orig;
            r.direcao.x          = std::cos(jogo.stand.anguloMira);
            r.direcao.y          = 0.0f;
            r.direcao.z          = std::sin(jogo.stand.anguloMira);
            r.tempoVida          = 0.0f;   // não expira por tempo; controlado por atirandoAgora
            r.dano               = (s.numEfeitos > 0) ? s.efeitos[0].valor : 1;
            r.raioColisao        = s.forma.raioColisao;
            r.perfuracaoRestante = s.forma.perfuracao;
            adicionarAoPool(sl.pool, r);
        }
        return;
    }

    // Periódico (cooldown > 0): spawna periodicamente.
    sl.cooldownRestante -= deltaTime;
    if (sl.cooldownRestante <= 0.0f) {
        sl.cooldownRestante = (s.cooldown > 0.0f) ? s.cooldown : 1.0f;

        // Beam e armas automáticas homing: sobrecarga bloqueia a emissão.
        if (jogo.stand.emSobrecarga &&
            (s.forma.tipo == FORMA_BEAM || s.movimento.tipo == MOV_HOMING)) return;

        Vetor3D origem = jogo.stand.posicao;

        // Beam: usa direção de mira; outros: mira no zumbi mais próximo.
        // idAlvoProx declarado aqui para ter escopo no loop de homing abaixo.
        int idAlvoProx = -1;
        Vetor3D alvo;
        if (s.forma.tipo == FORMA_BEAM) {
            alvo.x = origem.x + std::cos(jogo.stand.anguloMira) * 10.0f;
            alvo.y = 0.0f;
            alvo.z = origem.z + std::sin(jogo.stand.anguloMira) * 10.0f;
        } else {
            idAlvoProx = _acharZumbiMaisProximo(jogo, origem);
            if (idAlvoProx >= 0)
                alvo = jogo.horda[idAlvoProx].posicao;
            else
                alvo = jogo.protagonista.posicao;
        }

        int n = (s.forma.quantidade > 0 ? s.forma.quantidade : 1);

        // Para skills homing com N projéteis: cada míssil recebe um zumbi distinto.
        // Busca os N zumbis mais próximos sem repetição; se faltar zumbi, reutiliza o mais próximo.
        const int MAX_MISSEIS_LOTE = 9; // 3 armas × nível 3
        int alvosHoming[MAX_MISSEIS_LOTE];
        for (int ai = 0; ai < MAX_MISSEIS_LOTE; ++ai) alvosHoming[ai] = idAlvoProx;

        if (s.movimento.tipo == MOV_HOMING && n > 1) {
            int jaAtrib[MAX_MISSEIS_LOTE];
            int qtdAtrib = 0;
            int nBusca = (n < MAX_MISSEIS_LOTE) ? n : MAX_MISSEIS_LOTE;
            for (int mi = 0; mi < nBusca; ++mi) {
                int melhor = -1;
                float melhorD2 = 1e30f;
                for (int k = 0; k < (int)jogo.horda.size(); ++k) {
                    if (!jogo.horda[k].vivo) continue;
                    bool jaUsado = false;
                    for (int t = 0; t < qtdAtrib; ++t)
                        if (jaAtrib[t] == k) { jaUsado = true; break; }
                    if (jaUsado) continue;
                    float dxk = jogo.horda[k].posicao.x - origem.x;
                    float dzk = jogo.horda[k].posicao.z - origem.z;
                    float d2  = dxk*dxk + dzk*dzk;
                    if (d2 < melhorD2) { melhorD2 = d2; melhor = k; }
                }
                alvosHoming[mi] = (melhor >= 0) ? melhor : idAlvoProx;
                if (melhor >= 0) jaAtrib[qtdAtrib++] = melhor;
            }
        }

        Vetor3D dirBase = obterDirecaoNormalizada(origem, alvo);

        for (int i = 0; i < n; ++i) {
            // Para homing: direção e alvo individuais por míssil.
            // Para outros: leque/spread padrão.
            Vetor3D dir = dirBase;
            int idAlvoMissil = idAlvoProx;

            if (s.movimento.tipo == MOV_HOMING) {
                idAlvoMissil = (i < MAX_MISSEIS_LOTE) ? alvosHoming[i] : idAlvoProx;
                if (idAlvoMissil >= 0)
                    dir = obterDirecaoNormalizada(origem, jogo.horda[idAlvoMissil].posicao);
            } else {
                float dx2 = dirBase.x, dz2 = dirBase.z;
                float inicio = -((n - 1) * 0.5f) * s.forma.spreadAngulo;
                if (n > 1) _rotacionarXZ(dx2, dz2, inicio + i * s.forma.spreadAngulo);
                dir.x = dx2; dir.y = 0.0f; dir.z = dz2;
            }

            RuntimeSkill r;
            zerarRuntime(r);
            r.idSkillData        = s.id;
            r.ativo              = true;
            r.posicao            = origem;
            r.direcao            = dir;
            r.anguloAtual        = std::atan2(dir.z, dir.x);
            r.centro             = origem;
            r.alvo               = (idAlvoMissil >= 0) ? jogo.horda[idAlvoMissil].posicao : alvo;
            r.idAlvo             = idAlvoMissil;
            r.tempoVida          = (s.forma.duracao > 0.0f) ? s.forma.duracao : 0.0f;
            r.dano               = (s.numEfeitos > 0) ? s.efeitos[0].valor : 1;
            r.raioColisao        = s.forma.raioColisao;
            r.perfuracaoRestante = s.forma.perfuracao;

            if (s.movimento.tipo == MOV_FALL) {
                r.posicao.y = s.movimento.alturaInicial > 0.0f
                              ? s.movimento.alturaInicial : 10.0f;
                r.fase = 0;
            }

            adicionarAoPool(sl.pool, r);
        }

        // Beam: cobra tensão por emissão (~10/s com cooldown=0.1).
        // Outros skills automáticos (cooldown>0) têm custoTensao=0 e ficam isento.
        if (s.forma.tipo == FORMA_BEAM && s.custoTensao > 0.0f) {
            float maxT = maxTensaoDoNivel(jogo.protagonista.upgrades.niveis[TENSAO_UP]);
            jogo.stand.tensaoAtual += s.custoTensao;
            if (jogo.stand.tensaoAtual >= maxT) {
                jogo.stand.tensaoAtual = maxT;
                jogo.stand.emSobrecarga = true;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// _processarColisoesSlot — move e colide todas as instâncias de UM slot.
//   Espelha processarColisoesSkills_Grade mas opera sobre um SlotSkill isolado.
//   Preserva: perfuração, jaAcertados, compactação.
// ---------------------------------------------------------------------------
inline void _processarColisoesSlot(SlotSkill& sl, EstadoDoJogo& jogo,
                                    GradeEspacial& grade, float deltaTime) {
    const SkillData& s = sl.build;
    const int MAX_ALVOS = 64;

    std::vector<int> candidatos;
    candidatos.reserve(64);
    std::vector<int> beamTmp;   // reutilizado nas amostras do beam

    for (size_t i = 0; i < sl.pool.size(); ++i) {
        RuntimeSkill& r = sl.pool[i];
        if (!r.ativo) continue;

        // 1. Move / atualiza a instância (Origem → Movimento → vida).
        executarSkill(s, r, jogo, grade, deltaTime);
        if (!r.ativo) continue;

        // 2. Esta forma deve aplicar efeitos neste frame?
        if (!atualizarForma(s, r, deltaTime)) continue;

        // 3. Colisão contra vizinhos da grade.
        // BEAM: percorre várias células — amostra ao longo de todo o comprimento.
        // Sem isso, apenas zumbis perto da origem (stand) seriam detectados.
        if (s.forma.tipo == FORMA_BEAM) {
            candidatos.clear();
            int nAmostras = (int)(s.forma.comprimento / TAMANHO_CELULA) + 2;
            for (int si = 0; si < nAmostras; ++si) {
                float t = s.forma.comprimento * si /
                          (float)(nAmostras > 1 ? nAmostras - 1 : 1);
                grade.obterInimigosVizinhos(
                    r.posicao.x + r.direcao.x * t,
                    r.posicao.z + r.direcao.z * t,
                    beamTmp);
                for (int ti = 0; ti < (int)beamTmp.size(); ++ti) {
                    bool dup = false;
                    for (int ci = 0; ci < (int)candidatos.size(); ++ci)
                        if (candidatos[ci] == beamTmp[ti]) { dup = true; break; }
                    if (!dup) candidatos.push_back(beamTmp[ti]);
                }
            }
        } else {
            grade.obterInimigosVizinhos(r.posicao.x, r.posicao.z, candidatos);
        }

        int jaAcertados[MAX_ALVOS];
        int qtd = 0;

        for (size_t k = 0; k < candidatos.size(); ++k) {
            int j = candidatos[k];
            if (j < 0 || j >= (int)jogo.horda.size()) continue;
            Zumbi& z = jogo.horda[j];
            if (!z.vivo) continue;
            if (jaAcertouEsteZumbi_sg(jaAcertados, qtd, j)) continue;

            if (colideForma(s, r, z)) {
                if (qtd < MAX_ALVOS) jaAcertados[qtd++] = j;
                resolverEfeitos(s, r, z, jogo);

                // Perfuração (apenas projéteis de contato)
                if (s.forma.tipo == FORMA_PROJECTILE ||
                    s.forma.tipo == FORMA_CONE ||
                    s.forma.tipo == FORMA_PRISM) {
                    
                    if (r.perfuracaoRestante <= 0) { 
                        r.ativo = false; 
                        break; // <--- ESTE BREAK É OBRIGATÓRIO! Ele impede que a bala acerte zumbis sobrepostos no mesmo frame.
                    } else { 
                        r.perfuracaoRestante--;   
                    }
                }
            }
        }
    }

    compactarPool(sl.pool);
}

// ---------------------------------------------------------------------------
// Definição de atualizarTodos (agora que GradeEspacial e os helpers existem).
// ---------------------------------------------------------------------------
inline void SkillManager::atualizarTodos(EstadoDoJogo& jogo,
                                         GradeEspacial& grade,
                                         float deltaTime) {
    // 1. Constrói a grade de inimigos para este frame.
    grade.construirGrade(jogo);

    // 2. Itera TODOS os slots ativos — cada um é completamente independente.
    for (int i = 0; i < MAX_SLOTS_SKILL; ++i) {
        SlotSkill& sl = slots[i];
        if (!sl.ativo) continue;

        // 2a. Tica o emitter de skills automáticas (spawna quando necessário).
        if (sl.ehAutomatica()) {
            _atualizarEmitter(sl, jogo, grade, deltaTime);
        }

        // 2b. Move e colide todas as instâncias vivas deste slot.
        _processarColisoesSlot(sl, jogo, grade, deltaTime);
    }

    // 3. Colisão jogador-zumbi (única, compartilhada entre todos os slots).
    processarColisaoZumbiJogador_Grade(jogo, grade, deltaTime);
}

#endif // SKILL_MANAGER_H