#ifndef RUNTIME_SKILL_H
#define RUNTIME_SKILL_H

// RuntimeSkill.h — estado de instancia das skills em execucao
//
// PRINCIPIO DE SEPARACAO:
//   SkillData  = configuracao IMUTAVEL (o que a skill e)
//   RuntimeSkill = estado MUTAVEL de uma instancia viva (onde ela esta)
//
// Mil instancias podem compartilhar uma unica SkillData por indice.
// Toda a struct e POD puro: sem construtores, sem ponteiros donos,
// memcpy-safe e serializavel trivialmente.
//
// Dependencias: Entities.h (Vetor3D) e os enums de SkillTypes.h.
// NÃO incluímos SkillTypes.h aqui para evitar ciclo:
//   SkillTypes.h → RuntimeSkill.h → SkillTypes.h
// Quando RuntimeSkill.h é incluído por SkillTypes.h, todos os enums
// necessários (EfeitoType, FormaType…) já foram definidos acima naquele
// mesmo arquivo, então os guards evitam a recursão e o compilador os vê.
// Quando RuntimeSkill.h é incluído diretamente (ex: por RuntimeSkill.cpp
// de teste), inclua SkillTypes.h antes ou use o include abaixo que é
// seguro pelo guard:
#include "Entities.h"   // Vetor3D — sempre seguro (sem ciclo)
#include <vector>
#include <cstdlib> // rand()
#include <cmath>   // cosf, sinf

// ===========================================================================
// RuntimeSkill — uma instancia viva de uma SkillData
//
// Campos de estado por camada:
//   Origem    : centro, alvo, idAlvo
//   Movimento : posicao, direcao, anguloAtual, raioAtual,
//               distPercorrida, fase, ricochetesFeitos, tickAcumulado
//   Forma     : tickAcumulado (reusado para dano continuo), geracao
//   Recursao  : geracao, saltosFeitos
// ===========================================================================
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

    // --- Cache de atributos efetivos desta instância ---
    // Cacheados no spawn a partir da SkillData para acesso O(1) no executor
    // sem precisar consultar o catálogo a cada colisão.
    int     dano;              // dano efetivo (efeitos[0].valor ao nascer)
    float   raioColisao;       // raio de colisão efetivo (forma.raioColisao ao nascer)
    int     perfuracaoRestante;// atravessamentos restantes (forma.perfuracao ao nascer)

    // --- Array local de zumbis ja acertados neste frame ---
    // Evita que o mesmo projétil acerte o mesmo zumbi duas vezes.
    // Tamanho fixo (MAX_HITS_POR_FRAME) para manter POD.
    int     jaAcertados[32];
    int     qtdAcertados;
};

// ===========================================================================
// RuntimeEmitter — fonte que dispara skills periodicamente
//
// Usado por skills auto-fire (Aura por tick, Ring que re-emite, etc.).
// Separado de RuntimeSkill para nao poluir o estado de instancias voando.
// ===========================================================================
struct RuntimeEmitter {
    int     idSkillData;      // qual skill emite
    Vetor3D posicao;          // de onde emite (resolvido pela Origem)
    float   cooldownRestante; // timer ate proximo disparo
    bool    ativo;
};

// ===========================================================================
// RuntimeEffect — status aplicado a um zumbi (DoT, slow, stun)
//
// Vive separado para ticar independentemente da skill que o causou.
// Quando duracaoRestante <= 0, marcar como !ativo e compactar.
// ===========================================================================
struct RuntimeEffect {
    int        idZumbi;       // indice na horda que sofre o efeito
    EfeitoType tipo;          // BURN / FREEZE / SHOCK
    float      duracaoRest;   // segundos restantes
    float      magnitude;     // dps (Burn) / slow% (Freeze) / stun (Shock)
    int        stacks;        // contador para STACK_LIMITADO
    bool       ativo;
};

// ===========================================================================
// GESTAO DE POOL
//
// Estrategia swap-and-pop adaptada: reusa slots inativos antes de crescer
// o vector. Isso evita alocacao de heap no hot path apos o warm-up inicial.
//
// Complexidade:
//   adicionarAoPool   : O(n) no pior caso (busca slot livre), O(1) amortizado
//   compactarPool     : O(n) — chamada periodicamente, nao por frame
// ===========================================================================

// Adiciona uma nova instancia ao pool, reusando o primeiro slot inativo.
// Se nao houver slot livre, faz push_back (crescimento amortizado O(1)).
// Retorna o indice do slot onde a instancia foi colocada.
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

// Compacta o pool removendo slots inativos (swap-and-pop sequencial).
// Preserva a ordem relativa dos ativos. Chamada periodicamente (nao por frame).
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

// ===========================================================================
// inicializarRuntime
//
// Configura um RuntimeSkill recem-criado a partir da SkillData e dos
// parametros de spawn (posicao de origem, alvo, indice dentro do lote).
//
// O campo 'indiceNoLote' e 'totalNoLote' sao usados para distribuir
// angulos em Cone, Ring e Radial sem precisar de um loop externo aqui.
//
// Entrada: r             — RuntimeSkill a preencher
//          s             — SkillData de referencia
//          origemPos     — posicao de spawn resolvida pela Origem
//          alvo          — posicao do alvo (mouse/inimigo/ponto)
//          indiceNoLote  — 0-based: qual projetil do lote e este
//          totalNoLote   — quantos projeteis estao sendo spawnados juntos
//          geracaoInicial— profundidade de recursao (0 para disparos normais)
// ===========================================================================
inline void inicializarRuntime(RuntimeSkill& r,
                                const SkillData& s,
                                int idSkillData,
                                Vetor3D origemPos,
                                Vetor3D alvo,
                                int indiceNoLote,
                                int totalNoLote,
                                int geracaoInicial) {
    // Zera tudo
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

    // Tempo de vida: usa duracao da forma para formas continuas,
    // senao um valor alto (projeteis expiram ao sair da arena)
    if (s.forma.duracao > 0.0f)
        r.tempoVida = s.forma.duracao;
    else
        r.tempoVida = 30.0f; // 30s: limite de seguranca para projeteis

    // --- Direcao base (origem -> alvo, plano XZ) ---
    float dx = alvo.x - origemPos.x;
    float dz = alvo.z - origemPos.z;
    float len = std::sqrt(dx * dx + dz * dz);
    if (len > 0.0001f) {
        r.direcao.x = dx / len;
        r.direcao.z = dz / len;
    } else {
        r.direcao.x = 1.0f; // direcao padrao se origem == alvo
        r.direcao.z = 0.0f;
    }

    // --- Distribuicao angular para Cone / Ring / Projectile multi ---
    if (totalNoLote > 1) {
        float spread = s.forma.spreadAngulo;
        float angulo = 0.0f;

        if (totalNoLote == 2) {
            // Tiro duplo em V
            angulo = (indiceNoLote == 0) ? -spread : spread;
        } else {
            // Cone simetrico: distribui de -spread/2 a +spread/2
            // Ring: distribui 360 graus uniformemente
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

        // Rotacionar a direcao base pelo angulo calculado
        float cosA = std::cos(angulo);
        float sinA = std::sin(angulo);
        float nx = r.direcao.x * cosA - r.direcao.z * sinA;
        float nz = r.direcao.x * sinA + r.direcao.z * cosA;
        r.direcao.x = nx;
        r.direcao.z = nz;
    }

    // --- Estado inicial por tipo de movimento ---
    switch (s.movimento.tipo) {
        case MOV_ORBIT:
            // Inicia em posicao orbital: centro + offset angular
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
            // Começa no alto e cai
            r.posicao.y = s.movimento.alturaInicial;
            r.fase      = 0; // 0 = caindo, 1 = forma ativa no chao
            break;

        case MOV_BOOMERANG:
            r.fase           = 0; // 0 = ida, 1 = volta
            r.distPercorrida = 0.0f;
            break;

        case MOV_BOUNCE:
            r.ricochetesFeitos = 0;
            break;

        default:
            break;
    }

    // --- Estado inicial por tipo de forma ---
    if (s.forma.tipo == FORMA_RING) {
        r.anguloAtual = (2.0f * 3.14159265f / (float)totalNoLote) * (float)indiceNoLote;
    }
}

#endif // RUNTIME_SKILL_H