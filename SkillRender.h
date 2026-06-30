#ifndef SKILL_RENDER_H
#define SKILL_RENDER_H

// ===========================================================================
//  SkillRender.h — Despacho de renderização por FormaType.
//
//  Não inclui GL diretamente; depende das primitivas declaradas abaixo,
//  definidas em DrawPrimitives3D.h. Adicionar uma forma nova = novo case
//  em desenharSkill(), sem tocar no loop de desenho.
// ===========================================================================

#include "SkillTypes.h"
#include <cmath>   // std::atan2

// Declarações das primitivas GL definidas em DrawPrimitives3D.h.
void desenharBloco3D(float cx, float cy_base, float cz,
                     float lx, float lz, float altura,
                     float rT, float gT, float bT,
                     float rF, float gF, float bF,
                     float rB, float gB, float bB,
                     float rR, float gR, float bR,
                     float rL, float gL, float bL);

void desenharLinha3D(float x0, float y, float z0,
                     float x1, float z1,
                     float largura,
                     float r, float g, float b);
void desenharCirculo3D(float cx, float y, float cz,
                       float raio,
                       float r, float g, float b);
void desenharArco3D(float cx, float y, float cz,
                    float raioInterno, float raioExterno,
                    float anguloCentral, float meiaAbertura,
                    float r, float g, float b);

// yaw = atan2(direcao.z, direcao.x); orienta o eixo longo do projétil.
void desenharMissilOrientado3D(float cx, float cy, float cz, float yaw,
                                float cr, float cg, float cb);

void desenharTrailBala3D(float cx, float cy, float cz,
                          float dx, float dz,
                          float cr, float cg, float cb);

void ativarMaterialGlow(float cr, float cg, float cb);
void desativarMaterialGlow();

// Despacha a geometria certa pelo FormaType; cor default amarela quando FormaData.cor* = 0.
inline void desenharSkill(const SkillData& s, const RuntimeSkill& r) {
    if (!r.ativo) return;

    switch (s.forma.tipo) {
        case FORMA_PROJECTILE:
        case FORMA_CONE:
        case FORMA_RING:
        case FORMA_PRISM: {
                float tam = r.raioColisao * 0.8f;
                float py  = r.posicao.y;
                bool temCor = (s.forma.corR > 0.001f ||
                               s.forma.corG > 0.001f ||
                               s.forma.corB > 0.001f);
                float cr = temCor ? s.forma.corR : 1.0f;
                float cg = temCor ? s.forma.corG : 1.0f;
                float cb = temCor ? s.forma.corB : 0.2f;

                desenharTrailBala3D(r.posicao.x, py + tam * 0.5f, r.posicao.z,
                                    r.direcao.x, r.direcao.z, cr, cg, cb);
                ativarMaterialGlow(cr, cg, cb);

                bool ehMissil = (temCor && s.movimento.tipo == MOV_HOMING);
                if (ehMissil) {
                    float yaw = std::atan2(r.direcao.z, r.direcao.x);
                    desenharMissilOrientado3D(r.posicao.x, py, r.posicao.z, yaw,
                                              cr, cg, cb);
                } else if (temCor) {
                    desenharBloco3D(r.posicao.x, py, r.posicao.z,
                                    tam, tam, tam,
                                    cr,        cg,        cb,
                                    cr*0.80f,  cg*0.80f,  cb*0.80f,
                                    cr*0.55f,  cg*0.55f,  cb*0.55f,
                                    cr*0.90f,  cg*0.90f,  cb*0.90f,
                                    cr*0.65f,  cg*0.65f,  cb*0.65f);
                } else {
                    desenharBloco3D(r.posicao.x, py, r.posicao.z,
                                    tam, tam, tam,
                                    1.0f, 1.0f, 0.20f,
                                    0.80f, 0.80f, 0.0f,
                                    0.55f, 0.55f, 0.0f,
                                    0.90f, 0.90f, 0.05f,
                                    0.65f, 0.65f, 0.0f);
                }

                desativarMaterialGlow();
                break;
            }
        case FORMA_BEAM:
        case FORMA_WALL: {
            float x1 = r.posicao.x + r.direcao.x * s.forma.comprimento;
            float z1 = r.posicao.z + r.direcao.z * s.forma.comprimento;
            desenharLinha3D(r.posicao.x, r.posicao.y, r.posicao.z,
                            x1, z1,
                            s.forma.largura,
                            0.40f, 0.90f, 1.0f);   // ciano para feixe
            break;
        }
        case FORMA_AREA:
        case FORMA_AURA:
        case FORMA_EXPLOSION: {
            desenharCirculo3D(r.posicao.x, 0.06f, r.posicao.z,
                              s.forma.raioColisao,
                              1.0f, 0.55f, 0.10f);  // laranja para área
            break;
        }
        case FORMA_ARC: {
            desenharArco3D(r.centro.x, 0.06f, r.centro.z,
                           s.forma.raioInterno, s.forma.raioExterno,
                           r.anguloAtual, s.forma.anguloAbertura * 0.5f,
                           0.90f, 0.90f, 1.0f);
            break;
        }
        case FORMA_WAVE: {
            float px = -r.direcao.z, pz = r.direcao.x;  // perpendicular à direção
            float meia = s.forma.largura * 0.5f;
            desenharLinha3D(r.posicao.x - px * meia, 0.06f, r.posicao.z - pz * meia,
                            r.posicao.x + px * meia,        r.posicao.z + pz * meia,
                            0.20f,
                            0.20f, 0.60f, 1.0f);
            break;
        }
        case FORMA_CHAIN: {
            const float L = 0.18f, ALT = 0.18f;
            desenharBloco3D(r.posicao.x, r.posicao.y, r.posicao.z,
                            L, L, ALT,
                            0.40f, 0.70f, 1.0f,
                            0.20f, 0.45f, 0.80f,
                            0.15f, 0.30f, 0.55f,
                            0.30f, 0.55f, 0.90f,
                            0.20f, 0.40f, 0.65f);
            break;
        }
        default:
            break;
    }
}

// Itera o pool e delega cada instância a desenharSkill().
inline void desenharTodasSkills(const SkillData& buildAtiva,
                                const RuntimeSkill* pool, int n) {
    for (int i = 0; i < n; ++i)
        desenharSkill(buildAtiva, pool[i]);
}

#endif // SKILL_RENDER_H