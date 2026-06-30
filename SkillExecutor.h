#ifndef SKILL_EXECUTOR_H
#define SKILL_EXECUTOR_H

// ===========================================================================
//  SkillExecutor.h — Executor universal por switch sobre enum (O(1) por camada).
//
//  resolverOrigem → mov*() → atualizarForma → colideForma → resolverEfeitos
//  Não inclui SpatialGrid.h; depende de funções "host" declaradas abaixo.
// ===========================================================================

#include "SkillTypes.h"
#include "SkillPool.h"
#include "MathUtils.h"
#include "Entities.h"
#include <vector>
#include <cstdlib>   // rand
#include <cmath>

// Funções host definidas em Main.cpp/SpatialGrid.h — o executor não as inclui.
struct GradeEspacial;
extern const float FATOR_DANO_SOBRECARGA;

void  processarMorteZumbi(EstadoDoJogo& jogo, Zumbi& z);                 // GameLogic.h
void  criarFloatingDamage(EstadoDoJogo& jogo, Vetor3D posicaoImpacto, int dano); // GameLogic.h
void  criarParticulasMorte(EstadoDoJogo& jogo, Vetor3D posicaoOrigem,
                           float corR, float corG, float corB);          // GameLogic.h
void  obterCorBaseZumbi(TipoZumbi tipo, float& corR, float& corG, float& corB);  // GameLogic.h

// Adaptador livre — executor não inclui GradeEspacial diretamente.
void  obterVizinhosHost(GradeEspacial& grade, float x, float z,
                        std::vector<int>& saida);

// Índice do inimigo vivo mais próximo de `de` via grade, ou -1.
inline int acharInimigoMaisProximo(EstadoDoJogo& jogo, GradeEspacial& grade,
                                   Vetor3D de) {
    std::vector<int> cand;
    obterVizinhosHost(grade, de.x, de.z, cand);
    int melhor = -1;
    float melhorDist = 1e30f;
    for (size_t k = 0; k < cand.size(); ++k) {
        int j = cand[k];
        if (!jogo.horda[j].vivo) continue;
        float d = calcularDistanciaQuadrada(de, jogo.horda[j].posicao);
        if (d < melhorDist) { melhorDist = d; melhor = j; }
    }
    return melhor;
}

// Posição de spawn para cada OrigemType; alvoClique é o ponto do cursor.
inline Vetor3D resolverOrigem(EstadoDoJogo& jogo, GradeEspacial& grade,
                              const OrigemData& o, Vetor3D alvoClique) {
    Vetor3D p = {0.0f, 0.0f, 0.0f};
    switch (o.tipo) {
        case ORIG_PLAYER:      p = jogo.protagonista.posicao; break;
        case ORIG_STAND:       p = jogo.stand.posicao;        break;
        case ORIG_CURSOR:      p = alvoClique;                break;
        case ORIG_RANDOMMAP: {
            const float LIM = 150.0f;
            p.x = ((rand() % 2000) / 1000.0f - 1.0f) * LIM;
            p.z = ((rand() % 2000) / 1000.0f - 1.0f) * LIM;
            p.y = 0.0f;
            break;
        }
        case ORIG_KILLEDENEMY:
            p = jogo.houveMorteRecente ? jogo.ultimaPosicaoMorte
                                       : jogo.stand.posicao;
            break;
        case ORIG_NEARESTENEMY: {
            int j = acharInimigoMaisProximo(jogo, grade, jogo.stand.posicao);
            p = (j >= 0) ? jogo.horda[j].posicao : jogo.stand.posicao;
            break;
        }
        case ORIG_ALLENEMIES:   p = jogo.stand.posicao; break;  // spawn por inimigo no disparo, não aqui
        case ORIG_ORBITPOINT:   p = jogo.protagonista.posicao; break;
        default:                p = jogo.stand.posicao; break;
    }
    p.x += o.offset.x; p.y += o.offset.y; p.z += o.offset.z;
    return p;
}

// --- Integradores de movimento (um por MovimentoType) ---

inline void movLinear(const SkillData& s, RuntimeSkill& r, float dt) {
    r.posicao.x += r.direcao.x * s.movimento.velocidade * dt;
    r.posicao.z += r.direcao.z * s.movimento.velocidade * dt;
    r.distPercorrida += s.movimento.velocidade * dt;
}

inline void movOrbit(const SkillData& s, RuntimeSkill& r, float dt) {
    r.anguloAtual += s.movimento.velocidadeAngular * dt;
    r.posicao.x = r.centro.x + std::cos(r.anguloAtual) * s.movimento.raioOrbita;
    r.posicao.z = r.centro.z + std::sin(r.anguloAtual) * s.movimento.raioOrbita;
}

inline void movBoomerang(const SkillData& s, RuntimeSkill& r, float dt) {
    float passo = s.movimento.velocidade * dt;
    if (r.fase == 0) {                          // ida
        r.posicao.x += r.direcao.x * passo;
        r.posicao.z += r.direcao.z * passo;
        r.distPercorrida += passo;
        if (r.distPercorrida >= s.forma.alcanceMax) {
            r.fase = 1;                          // inverte
            r.direcao.x = -r.direcao.x;
            r.direcao.z = -r.direcao.z;
        }
    } else {                                     // volta
        r.posicao.x += r.direcao.x * passo;
        r.posicao.z += r.direcao.z * passo;
        r.distPercorrida -= passo;
        if (r.distPercorrida <= 0.0f) r.ativo = false; // reencontrou origem
    }
}

inline void movHoming(const SkillData& s, RuntimeSkill& r,
                      EstadoDoJogo& jogo, float dt) {
    // Re-adquire o zumbi mais próximo se o alvo atual morreu — o míssil nunca voa reto.
    bool alvoValido = (r.idAlvo >= 0 && r.idAlvo < (int)jogo.horda.size() &&
                       jogo.horda[r.idAlvo].vivo);
    if (!alvoValido) {
        int melhor = -1;
        float melhorDist = 1e30f;
        for (int i = 0; i < (int)jogo.horda.size(); ++i) {
            if (!jogo.horda[i].vivo) continue;
            float dx = jogo.horda[i].posicao.x - r.posicao.x;
            float dz = jogo.horda[i].posicao.z - r.posicao.z;
            float d2 = dx * dx + dz * dz;
            if (d2 < melhorDist) { melhorDist = d2; melhor = i; }
        }
        r.idAlvo = melhor;
        alvoValido = (melhor >= 0);
    }

    if (alvoValido) {
        Vetor3D desejada = obterDirecaoNormalizada(r.posicao,
                                                   jogo.horda[r.idAlvo].posicao);
        float t = s.movimento.taxaCorrecao * dt;
        if (t > 1.0f) t = 1.0f;
        r.direcao.x += (desejada.x - r.direcao.x) * t;
        r.direcao.z += (desejada.z - r.direcao.z) * t;
        float len = std::sqrt(r.direcao.x*r.direcao.x + r.direcao.z*r.direcao.z);
        if (len > 0.0001f) { r.direcao.x /= len; r.direcao.z /= len; }
    }
    r.posicao.x += r.direcao.x * s.movimento.velocidade * dt;
    r.posicao.z += r.direcao.z * s.movimento.velocidade * dt;
}

inline void movSpiral(const SkillData& s, RuntimeSkill& r, float dt) {
    r.anguloAtual += s.movimento.velocidadeAngular * dt;
    r.raioAtual   += s.movimento.velocidadeRadial * dt;
    r.posicao.x = r.centro.x + std::cos(r.anguloAtual) * r.raioAtual;
    r.posicao.z = r.centro.z + std::sin(r.anguloAtual) * r.raioAtual;
}

inline void movBounce(const SkillData& s, RuntimeSkill& r, float dt) {
    const float LIM = 150.0f;
    r.posicao.x += r.direcao.x * s.movimento.velocidade * dt;
    r.posicao.z += r.direcao.z * s.movimento.velocidade * dt;
    bool ricocheteou = false;
    if (r.posicao.x >  LIM) { r.posicao.x =  LIM; r.direcao.x = -r.direcao.x; ricocheteou = true; }
    if (r.posicao.x < -LIM) { r.posicao.x = -LIM; r.direcao.x = -r.direcao.x; ricocheteou = true; }
    if (r.posicao.z >  LIM) { r.posicao.z =  LIM; r.direcao.z = -r.direcao.z; ricocheteou = true; }
    if (r.posicao.z < -LIM) { r.posicao.z = -LIM; r.direcao.z = -r.direcao.z; ricocheteou = true; }
    if (ricocheteou) {
        r.ricochetesFeitos++;
        if (r.ricochetesFeitos > s.movimento.ricochetesMax) r.ativo = false;
    }
}

inline void movFall(const SkillData& s, RuntimeSkill& r, float dt) {
    if (r.fase == 0) {                          // ainda caindo / telegrafando
        r.posicao.y -= s.movimento.velocidadeQueda * dt;
        if (r.posicao.y <= 0.0f) {
            r.posicao.y = 0.0f;
            r.fase = 1;                          // pousou: forma fica ativa
        }
    }
    // fase 1: estacionária no chão; a Forma (Area/Explosion) é quem age.
}

inline void movTeleport(const SkillData& s, RuntimeSkill& r,
                        EstadoDoJogo& jogo, GradeEspacial& grade, float dt) {
    r.tickAcumulado += dt;
    if (r.tickAcumulado >= s.movimento.intervalo) {
        r.tickAcumulado = 0.0f;
        r.posicao = resolverOrigem(jogo, grade, s.origem, r.alvo); // salta p/ nova origem
    }
}

inline void movRandomWalk(const SkillData& s, RuntimeSkill& r, float dt) {
    r.tickAcumulado += dt;
    if (r.tickAcumulado >= s.movimento.intervalo) {
        r.tickAcumulado = 0.0f;
        float ang = (float)(rand() % 360) * (3.14159265f / 180.0f);
        r.direcao.x = std::cos(ang);
        r.direcao.z = std::sin(ang);
    }
    r.posicao.x += r.direcao.x * s.movimento.velocidade * dt;
    r.posicao.z += r.direcao.z * s.movimento.velocidade * dt;
}

inline void movStationary(const SkillData& s, RuntimeSkill& r) {
    if (s.movimento.seguirOrigem)
        r.posicao = r.centro;  // Aura segue origem; Wall/Beam fixos ficam onde nasceram
}

// True se a forma deve aplicar efeitos neste frame (tick disparado ou contato).
inline bool atualizarForma(const SkillData& s, RuntimeSkill& r, float dt) {
    switch (s.forma.tipo) {
        case FORMA_BEAM:
        case FORMA_AREA:
        case FORMA_AURA:
        case FORMA_WALL: {
            r.tickAcumulado += dt;
            if (r.tickAcumulado >= s.forma.tickIntervalo) {
                r.tickAcumulado -= s.forma.tickIntervalo;
                return true;
            }
            return false;
        }
        case FORMA_ARC: {
            r.centro = r.posicao;  // arco se move como projétil; centro segue posição
            r.tickAcumulado += dt;
            if (r.tickAcumulado >= s.forma.tickIntervalo) {
                r.tickAcumulado -= s.forma.tickIntervalo;
                return true;
            }
            return false;
        }
        case FORMA_EXPLOSION:
            return true;   // aplica uma vez; expira no frame seguinte
        default:
            return true;   // Projectile/Cone/Ring/etc: colisão por contato
    }
}

// Predicado de colisão por FormaType — usa MathUtils.h.
inline bool colideForma(const SkillData& s, const RuntimeSkill& r,
                        const Zumbi& z) {
    switch (s.forma.tipo) {
        case FORMA_PROJECTILE:
        case FORMA_CONE:
        case FORMA_RING:
        case FORMA_PRISM:
            return verificarColisao(r.posicao, r.raioColisao,
                                    z.posicao, z.raioColisao);
        case FORMA_AREA:
        case FORMA_AURA:
        case FORMA_EXPLOSION: {
            float rr = s.forma.raioColisao + z.raioColisao;
            return calcularDistanciaQuadrada(r.posicao, z.posicao) <= rr * rr;
        }
        case FORMA_BEAM:
        case FORMA_WALL:
            return distanciaPontoSegmento(z.posicao, r.posicao, r.direcao,
                       s.forma.comprimento) <= (s.forma.largura + z.raioColisao);
        case FORMA_WAVE:
            return distanciaPontoSegmento(z.posicao, r.centro, r.direcao,
                       r.distPercorrida) <= (s.forma.largura + z.raioColisao);
        case FORMA_ARC:
            return dentroDoArco(z.posicao, r.centro,
                                s.forma.raioInterno, s.forma.raioExterno,
                                r.anguloAtual, s.forma.anguloAbertura * 0.5f);
        case FORMA_CHAIN:
            return verificarColisao(r.posicao, r.raioColisao,
                                    z.posicao, z.raioColisao);
        default:
            return false;
    }
}

inline void aplicarDanoZumbi(EstadoDoJogo& jogo, Zumbi& z, int dano) {
    int danoEfetivo = dano;
    if (jogo.stand.emSobrecarga)
        danoEfetivo = (int)(dano * FATOR_DANO_SOBRECARGA);
    z.vida -= danoEfetivo;
    criarFloatingDamage(jogo, z.posicao, danoEfetivo);
    if (z.vida <= 0) {
        float cR, cG, cB;
        obterCorBaseZumbi(z.tipo, cR, cG, cB);
        criarParticulasMorte(jogo, z.posicao, cR, cG, cB);
        processarMorteZumbi(jogo, z);
    }
}

// Aplica o array EfeitoData ao zumbi atingido.
inline void resolverEfeitos(const SkillData& s, const RuntimeSkill& r,
                            Zumbi& z, EstadoDoJogo& jogo) {
    bool retornoBumerangue = (s.movimento.tipo == MOV_BOOMERANG && r.fase == 1);  // metade do dano na volta

    for (int i = 0; i < s.numEfeitos; ++i) {
        const EfeitoData& e = s.efeitos[i];
        switch (e.tipo) {
            case EFE_DAMAGE: {
                int dano = e.valor;
                if (retornoBumerangue) {
                    dano = dano / 2;
                    if (dano < 1) dano = 1;
                }
                aplicarDanoZumbi(jogo, z, dano);
                break;
            }
            case EFE_DRAIN:
                if (!jogo.stand.emSobrecarga) {
                    jogo.stand.tensaoAtual -= (e.valor > 0 ? (float)e.valor : 3.0f);
                    if (jogo.stand.tensaoAtual < 0.0f) jogo.stand.tensaoAtual = 0.0f;
                }
                break;
            case EFE_HEAL: {
                int cura = (e.valor > 0) ? e.valor : 1;
                jogo.protagonista.hp += cura;
                if (jogo.protagonista.hp > jogo.protagonista.hpMaximo)
                    jogo.protagonista.hp = jogo.protagonista.hpMaximo;
                break;
            }
            case EFE_SHOCK:
                aplicarDanoZumbi(jogo, z, e.valor);  // stun completo na Camada 3
                break;
            case EFE_KNOCKBACK: {
                Vetor3D dir = obterDirecaoNormalizada(jogo.stand.posicao, z.posicao);
                z.posicao.x += dir.x * e.magnitude;
                z.posicao.z += dir.z * e.magnitude;
                break;
            }
            case EFE_EXPLOSION: {
                float raio = (e.magnitude > 0.0f) ? e.magnitude : 1.5f;
                float raio2 = raio * raio;
                int danoExp = (e.valor > 0) ? e.valor : 1;
                for (int k = 0; k < (int)jogo.horda.size(); ++k) {
                    Zumbi& alvo = jogo.horda[k];
                    if (!alvo.vivo) continue;
                    float dx = alvo.posicao.x - z.posicao.x;
                    float dz = alvo.posicao.z - z.posicao.z;
                    if (dx*dx + dz*dz <= raio2)
                        aplicarDanoZumbi(jogo, alvo, danoExp);
                }
                break;
            }
            case EFE_BURN:
            case EFE_FREEZE:
            case EFE_CHAINEXPLOSION:
            case EFE_SPAWNSKILL:
                if (e.valor > 0) aplicarDanoZumbi(jogo, z, e.valor);  // Camada 3: placeholder
                break;
            default:
                break;
        }
    }
}

// Orquestra Origem→Movimento→vida por instância; colisão/efeitos ficam no host.
inline void executarSkill(const SkillData& s, RuntimeSkill& r,
                          EstadoDoJogo& jogo, GradeEspacial& grade, float dt) {
    if (!r.ativo) return;

    if (s.origem.reavaliarPorFrame)
        r.centro = resolverOrigem(jogo, grade, s.origem, r.alvo);

    switch (s.movimento.tipo) {
        case MOV_LINEAR:     movLinear(s, r, dt);             break;
        case MOV_ORBIT:      movOrbit(s, r, dt);              break;
        case MOV_BOOMERANG:  movBoomerang(s, r, dt);          break;
        case MOV_HOMING:     movHoming(s, r, jogo, dt);       break;
        case MOV_SPIRAL:     movSpiral(s, r, dt);             break;
        case MOV_BOUNCE:     movBounce(s, r, dt);             break;
        case MOV_FALL:       movFall(s, r, dt);               break;
        case MOV_TELEPORT:   movTeleport(s, r, jogo, grade, dt); break;
        case MOV_RANDOMWALK: movRandomWalk(s, r, dt);         break;
        case MOV_STATIONARY: movStationary(s, r);             break;
        default: break;
    }

    // Decai tempo de vida; projéteis fora da arena morrem.
    if (r.tempoVida > 0.0f) {
        r.tempoVida -= dt;
        if (r.tempoVida <= 0.0f) r.ativo = false;
    }
    const float LIM = 150.0f;
    if (s.forma.tipo == FORMA_PROJECTILE || s.forma.tipo == FORMA_CONE ||
        s.forma.tipo == FORMA_PRISM) {
        if (r.posicao.x >  LIM || r.posicao.x < -LIM ||
            r.posicao.z >  LIM || r.posicao.z < -LIM) {
            r.ativo = false;
        }
    }
}

#endif // SKILL_EXECUTOR_H
