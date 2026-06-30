#ifndef SKILL_MANAGER_H
#define SKILL_MANAGER_H

// ===========================================================================
//  SkillManager.h — Orquestrador de N skills equipadas simultaneamente.
//
//  Cada SlotSkill é independente: build, pool de instâncias, emitter e cooldown.
//  Skills automáticas (cooldown>0 ou ORBIT/AURA) disparam via emitter em
//  atualizarTodos(); instantâneas (cooldown==0) disparam por clique em executar().
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

#define MAX_SLOTS_SKILL   6  // espelha _INV_MAX_EQUIPADAS

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

    // True se tem cooldown definido ou é Orbit/Stationary (sem clique do jogador).
    bool ehAutomatica() const {
        if (!ativo) return false;
        if (build.cooldown > 0.0f) return true;
        MovimentoType m = build.movimento.tipo;
        return (m == MOV_ORBIT || m == MOV_STATIONARY);
    }

    bool ehInstantanea() const {
        if (!ativo) return false;
        return (build.cooldown == 0.0f &&
                build.movimento.tipo != MOV_ORBIT &&
                build.movimento.tipo != MOV_STATIONARY);
    }
};

struct SkillManager {

    SlotSkill slots[MAX_SLOTS_SKILL];
    int       numSlotsAtivos;

    SkillManager() : numSlotsAtivos(0) {}

    // Popula um slot com build pronta (chamado por montarBuildCompleta()).
    void registrarSlot(int indice, const SkillData& build) {
        if (indice < 0 || indice >= MAX_SLOTS_SKILL) return;

        SlotSkill& sl = slots[indice];
        sl.pool.clear();
        sl.build            = build;
        sl.cooldownRestante = 0.0f;
        sl.ativo            = true;

        sl.emitter.ativo       = sl.ehAutomatica();
        sl.emitter.idSkillData = build.id;
        // cooldownRestante=0: dispara no primeiro frame; orbitais spawnam na 1ª atualização

        _recalcularNumAtivos();
    }

    void limparSlot(int indice) {
        if (indice < 0 || indice >= MAX_SLOTS_SKILL) return;
        slots[indice].pool.clear();
        slots[indice].ativo = false;
        slots[indice].emitter.ativo = false;
        _recalcularNumAtivos();
    }

    // Esvazia todos os slots — usado em reinício de partida.
    void limparTodos() {
        for (int i = 0; i < MAX_SLOTS_SKILL; ++i) {
            slots[i].pool.clear();
            slots[i].ativo = false;
            slots[i].emitter.ativo = false;
        }
        numSlotsAtivos = 0;
    }

    // Esvazia os pools de projéteis sem reconstruir as builds.
    void limparInstancias() {
        for (int i = 0; i < MAX_SLOTS_SKILL; ++i)
            slots[i].pool.clear();
    }

    // Dispara skills instantâneas (clique); automáticas são tratadas em atualizarTodos().
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

    // Frame completo: emitters → movimento+colisão → colisão jogador-zumbi.
    // Definido fora da struct (precisa de GradeEspacial completo).
    void atualizarTodos(EstadoDoJogo& jogo, GradeEspacial& grade, float deltaTime);

    int numSlots() const { return MAX_SLOTS_SKILL; }

    const SlotSkill& slot(int i) const { return slots[i]; }

    // Retrocompatibilidade: slot 0 para render loop legado.
    const std::vector<RuntimeSkill>& instancias() const { return slots[0].pool; }
    const SkillData& buildAtiva() const { return slots[0].build; }

    // Stubs legados — build é montada em montarBuildCompleta() → registrarSlot().
    void reconstruirHabilidade(const SistemaUpgrades& /*upgrades*/) {}
    void selecionarSkillBase(const SkillData& /*base*/,
                             const SistemaUpgrades& /*upgrades*/) {}

private:
    void _recalcularNumAtivos() {
        numSlotsAtivos = 0;
        for (int i = 0; i < MAX_SLOTS_SKILL; ++i)
            if (slots[i].ativo) numSlotsAtivos++;
    }

    void _empurrarInstancia(SlotSkill& sl, EstadoDoJogo& jogo, const SkillData& s,
                            Vetor3D origem, Vetor3D dir);

    void _spawnarInstancias(SlotSkill& sl, EstadoDoJogo& jogo, Vetor3D alvo) {
        const SkillData& s = sl.build;
        Vetor3D origem     = jogo.posicaoPistola;
        Vetor3D dirBase    = obterDirecaoNormalizada(origem, alvo);

        int n = (s.forma.quantidade > 0 ? s.forma.quantidade : 1);

        if (n == 1) {
            _empurrarInstancia(sl, jogo, s, origem, dirBase);
            return;
        }

        float inicio = -((n - 1) * 0.5f) * s.forma.spreadAngulo;
        for (int i = 0; i < n; ++i) {
            float dx = dirBase.x, dz = dirBase.z;
            _rotacionarXZ(dx, dz, inicio + i * s.forma.spreadAngulo);
            Vetor3D dir = {dx, 0.0f, dz};
            _empurrarInstancia(sl, jogo, s, origem, dir);
        }
    }

    // Spawn inicial do anel orbital (N instâncias distribuídas em 360°).
    void _spawnarInstanciasOrbital(SlotSkill& sl, EstadoDoJogo& jogo) {
        const SkillData& s = sl.build;
        Vetor3D origem = jogo.protagonista.posicao;

        int n = (s.forma.quantidade > 0 ? s.forma.quantidade : 1);
        float passo = (2.0f * 3.14159265f) / (float)n;

        for (int i = 0; i < n; ++i) {
            float angulo = passo * (float)i;
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

    // cópia desabilitada
    SkillManager(const SkillManager&);
    SkillManager& operator=(const SkillManager&);
};

// Definições que precisam de GradeEspacial completo.
#include "SpatialGrid.h"

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
    r.anguloAtual        = std::atan2(dir.z, dir.x);  // FORMA_ARC usa para orientar o setor
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

inline void obterVizinhosHost(GradeEspacial& grade, float x, float z,
                              std::vector<int>& saida) {
    grade.obterInimigosVizinhos(x, z, saida);
}

// Tica emitter: ORBIT=spawn único persistente; cooldown>0=periódico; STATIONARY=spawn único.
inline void _atualizarEmitter(SlotSkill& sl, EstadoDoJogo& jogo,
                               GradeEspacial& /*grade*/, float deltaTime) {
    const SkillData& s = sl.build;

    if (s.movimento.tipo == MOV_ORBIT) {
        int ativas = contarAtivos(sl.pool);
        if (ativas == 0) {
            // Primeira vez: spawna o anel orbital completo (sem cooldown — vive até desejado).
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
        for (size_t k = 0; k < sl.pool.size(); ++k) {
            if (sl.pool[k].ativo)
                sl.pool[k].centro = jogo.protagonista.posicao;
        }
        return;
    }

    if (s.movimento.tipo == MOV_STATIONARY && s.cooldown == 0.0f) {  // Aura/Beam: spawn único
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

    // BEAM com cooldown>0: laser manual de alta frequência; requer mouse pressionado.
    if (s.forma.tipo == FORMA_BEAM && s.cooldown > 0.0f) {
        if (!jogo.atirandoAgora || jogo.stand.emSobrecarga) {
            sl.pool.clear();
            sl.cooldownRestante = 0.0f;
            return;
        }
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
            r.tempoVida          = 0.0f;   // não expira por tempo; controlado por jogo.atirandoAgora
            r.dano               = (s.numEfeitos > 0) ? s.efeitos[0].valor : 1;
            r.raioColisao        = s.forma.raioColisao;
            r.perfuracaoRestante = s.forma.perfuracao;
            adicionarAoPool(sl.pool, r);
        }
        return;
    }

    // Periódico: spawna a cada cooldown segundos.
    sl.cooldownRestante -= deltaTime;
    if (sl.cooldownRestante <= 0.0f) {
        sl.cooldownRestante = (s.cooldown > 0.0f) ? s.cooldown : 1.0f;

        if (jogo.stand.emSobrecarga &&
            (s.forma.tipo == FORMA_BEAM || s.movimento.tipo == MOV_HOMING)) return;

        Vetor3D origem = jogo.stand.posicao;
        int idAlvoProx = -1;
        Vetor3D alvo;
        if (s.forma.tipo == FORMA_BEAM) {  // Beam: segue a mira
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

        // Homing multi-míssil: distribui alvos distintos; reutiliza o mais próximo se faltar.
        const int MAX_MISSEIS_LOTE = 9;  // 3 armas × nível 3
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