#ifndef RUNTIME_SKILL_H
#define RUNTIME_SKILL_H

// RuntimeSkill.h — estado mutável de uma instância viva de uma SkillData.
//
// SkillData = configuração imutável; RuntimeSkill = onde e como ela está.
// N instâncias podem compartilhar um único SkillData por índice. POD puro.
//
// Não inclui SkillTypes.h para evitar ciclo (SkillTypes→RuntimeSkill→SkillTypes).
// Quando incluso via SkillTypes.h, os enums já estão definidos acima pelo TU.
#include "Entities.h"
#include <vector>
#include <cstdlib> // rand()
#include <cmath>   // cosf, sinf

struct RuntimeSkill {
    int     idSkillData;      // indice no catalogo SkillManager::catalogo
    bool    ativo;            // false = slot livre no pool

    // --- Posicao e movimento ---
    Vetor3D posicao;          // posicao atual no mundo (plano XZ, y=0)
    Vetor3D direcao;          // direcao atual normalizada (Linear/Homing)
    Vetor3D centro;           // centro de orbita ou origem resolvida (Orbit/Aura)
    Vetor3D alvo;             // alvo de mira (Homing/Cursor)

    // --- Tempo de vida ---
    float   tempoVida;        // segundos restantes antes de expirar (0 = inativo)

    // --- Estado de movimento ---
    float   anguloAtual;      // Orbit/Spiral/Arc: angulo em radianos
    float   raioAtual;        // Spiral: raio crescente
    float   distPercorrida;   // Boomerang/Wave: distancia percorrida
    float   timerMovimento;   // Teleport/RandomWalk: tempo ate proxima acao

    // --- Ticks continuos (Beam/Area/Aura) ---
    float   tickAcumulado;    // tempo acumulado desde o ultimo tick de dano

    // --- Fase de execucao ---
    int     fase;             // Boomerang: 0=ida 1=volta | Fall: 0=caindo 1=ativo

    // --- Controle de colisao ---
    int     ricochetesFeitos; // Bounce: quantos ricochetes ja ocorreram
    int     saltosFeitos;     // Chain: quantos saltos ja ocorreram

    // --- Homing: cache do alvo ---
    int     idAlvo;           // indice na horda; -1 = sem alvo
    float   timerReaquisicao; // tempo ate re-buscar o alvo mais proximo

    // --- Recursao (Prism/Chain) ---
    int     geracao;          // profundidade de recursao atual (0 = raiz)

    // Cacheados no spawn para acesso O(1) no executor sem consultar o catálogo.
    int     dano;              // efeitos[0].valor ao nascer
    float   raioColisao;       // raio de colisão efetivo (forma.raioColisao ao nascer)
    int     perfuracaoRestante;// atravessamentos restantes (forma.perfuracao ao nascer)

    int     jaAcertados[32];   // evita acertar o mesmo zumbi duas vezes por frame
    int     qtdAcertados;
};

// Separado de RuntimeSkill para não poluir instâncias voando.
struct RuntimeEmitter {
    int     idSkillData;      // qual skill emite
    Vetor3D posicao;          // de onde emite (resolvido pela Origem)
    float   cooldownRestante; // timer ate proximo disparo
    bool    ativo;
};

// Toca independentemente da skill que o causou; inativo quando duracaoRest <= 0.
struct RuntimeEffect {
    int        idZumbi;       // indice na horda que sofre o efeito
    EfeitoType tipo;          // BURN / FREEZE / SHOCK
    float      duracaoRest;   // segundos restantes
    float      magnitude;     // dps (Burn) / slow% (Freeze) / stun (Shock)
    int        stacks;        // contador para STACK_LIMITADO
    bool       ativo;
};

// Reusa o primeiro slot inativo; push_back se não houver. Retorna o índice do slot.
inline int adicionarAoPool(std::vector<RuntimeSkill>& pool,
                            const RuntimeSkill& nova) {
    for (int i = 0; i < (int)pool.size(); ++i) {
        if (!pool[i].ativo) {
            pool[i] = nova;
            return i;
        }
    }
    pool.push_back(nova);
    return (int)pool.size() - 1;
}

inline int adicionarEmitter(std::vector<RuntimeEmitter>& pool,
                             const RuntimeEmitter& novo) {
    for (int i = 0; i < (int)pool.size(); ++i) {
        if (!pool[i].ativo) {
            pool[i] = novo;
            return i;
        }
    }
    pool.push_back(novo);
    return (int)pool.size() - 1;
}

inline int adicionarEfeito(std::vector<RuntimeEffect>& pool,
                            const RuntimeEffect& novo) {
    for (int i = 0; i < (int)pool.size(); ++i) {
        if (!pool[i].ativo) {
            pool[i] = novo;
            return i;
        }
    }
    pool.push_back(novo);
    return (int)pool.size() - 1;
}

// Remove inativos; chamada periodicamente, não por frame — O(n).
inline void compactarPool(std::vector<RuntimeSkill>& pool) {
    int w = 0;
    for (int r = 0; r < (int)pool.size(); ++r) {
        if (pool[r].ativo) pool[w++] = pool[r];
    }
    pool.resize(w);
}

inline void compactarEmitters(std::vector<RuntimeEmitter>& pool) {
    int w = 0;
    for (int r = 0; r < (int)pool.size(); ++r) {
        if (pool[r].ativo) pool[w++] = pool[r];
    }
    pool.resize(w);
}

inline void compactarEfeitos(std::vector<RuntimeEffect>& pool) {
    int w = 0;
    for (int r = 0; r < (int)pool.size(); ++r) {
        if (pool[r].ativo) pool[w++] = pool[r];
    }
    pool.resize(w);
}

// Preenche r a partir de s e dos parâmetros de spawn.
// indiceNoLote/totalNoLote distribuem ângulos em Cone/Ring sem loop externo.
inline void inicializarRuntime(RuntimeSkill& r,
                                const SkillData& s,
                                int idSkillData,
                                Vetor3D origemPos,
                                Vetor3D alvo,
                                int indiceNoLote,
                                int totalNoLote,
                                int geracaoInicial) {
    int i;
    char* p = (char*)&r;
    for (i = 0; i < (int)sizeof(RuntimeSkill); ++i) p[i] = 0;

    r.idSkillData = idSkillData;
    r.ativo       = true;
    r.posicao     = origemPos;
    r.centro      = origemPos;
    r.alvo        = alvo;
    r.idAlvo      = -1;
    r.geracao     = geracaoInicial;

    // Formas contínuas usam duracao; projéteis calculam tempo de voo por alcance/velocidade.
    if (s.forma.duracao > 0.0f) {
        r.tempoVida = s.forma.duracao;
    } else if (s.movimento.velocidade > 0.001f && s.forma.alcanceMax > 0.0f) {
        r.tempoVida = s.forma.alcanceMax / s.movimento.velocidade;
    } else {
        r.tempoVida = 30.0f;  // fallback se sem alcance/velocidade definidos
    }

    float dx = alvo.x - origemPos.x;
    float dz = alvo.z - origemPos.z;
    float len = std::sqrt(dx * dx + dz * dz);
    if (len > 0.0001f) {
        r.direcao.x = dx / len;
        r.direcao.z = dz / len;
    } else {
        r.direcao.x = 1.0f;  // fallback quando origem == alvo
        r.direcao.z = 0.0f;
    }

    if (totalNoLote > 1) {
        float spread = s.forma.spreadAngulo;
        float angulo = 0.0f;

        if (totalNoLote == 2) {
            angulo = (indiceNoLote == 0) ? -spread : spread;  // duplo em V
        } else {
            bool ehRing = (s.forma.tipo == FORMA_RING);
            if (ehRing) {
                float passo = (2.0f * 3.14159265f) / (float)totalNoLote;
                angulo = (float)indiceNoLote * passo;
            } else {
                float passoSpread = (totalNoLote > 1)
                    ? spread / (float)(totalNoLote - 1)
                    : 0.0f;
                angulo = -spread * 0.5f + (float)indiceNoLote * passoSpread;
            }
        }

        float cosA = std::cos(angulo);
        float sinA = std::sin(angulo);
        float nx = r.direcao.x * cosA - r.direcao.z * sinA;
        float nz = r.direcao.x * sinA + r.direcao.z * cosA;
        r.direcao.x = nx;
        r.direcao.z = nz;
    }

    switch (s.movimento.tipo) {
        case MOV_ORBIT:
            r.anguloAtual = (totalNoLote > 1)
                ? (2.0f * 3.14159265f / (float)totalNoLote) * (float)indiceNoLote
                : 0.0f;
            r.posicao.x = origemPos.x + s.movimento.raioOrbita * std::cos(r.anguloAtual);
            r.posicao.z = origemPos.z + s.movimento.raioOrbita * std::sin(r.anguloAtual);
            break;

        case MOV_SPIRAL:
            r.anguloAtual = 0.0f;
            r.raioAtual   = 0.0f;
            break;

        case MOV_FALL:
            r.posicao.y = s.movimento.alturaInicial;
            r.fase      = 0;  // 0=caindo, 1=ativo no chão
            break;

        case MOV_BOOMERANG:
            r.fase           = 0;  // 0=ida, 1=volta
            r.distPercorrida = 0.0f;
            break;

        case MOV_BOUNCE:
            r.ricochetesFeitos = 0;
            break;

        default:
            break;
    }

    if (s.forma.tipo == FORMA_RING) {
        r.anguloAtual = (2.0f * 3.14159265f / (float)totalNoLote) * (float)indiceNoLote;
    }
}

#endif // RUNTIME_SKILL_H