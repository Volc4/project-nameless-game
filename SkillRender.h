#ifndef SKILL_RENDER_H
#define SKILL_RENDER_H

// ===========================================================================
//  SkillRender.h — Renderização data-driven, dependente APENAS da Forma
//                  (Fase 5 — "a renderização deve depender apenas deste
//                   componente")
//
//  PAPEL
//  -----
//  O render loop (Main.cpp) não conhece "Laser" nem "Disco". Ele chama
//  desenharSkill(SkillData, RuntimeSkill) e o despacho por FormaType decide
//  a geometria visual. Adicionar uma forma nova = um case aqui, sem tocar no
//  loop de desenho nem no SkillManager.
//
//  DESACOPLAMENTO
//  --------------
//  Este header NÃO faz #include <GL/glut.h>. Ele depende de um conjunto mínimo
//  de PRIMITIVAS DE DESENHO declaradas adiante (definidas em Main.cpp com GL
//  real). Assim a lógica de "qual forma desenha o quê" fica isolada e
//  testável, e o GL fica concentrado em um lugar só.
//
//  C++98: switch sobre enum, sem classes, sem ponteiros donos.
// ===========================================================================

#include "SkillTypes.h"
#include <cmath>   // std::atan2

// ---------------------------------------------------------------------------
// CONTRATO DE PRIMITIVAS (definidas em Main.cpp, onde o GL existe).
//   São blocos de desenho genéricos; nenhuma "sabe" o que é uma skill.
// ---------------------------------------------------------------------------

// Bloco 3D (cubo colorido) — assinatura já existente em Main.cpp.
void desenharBloco3D(float cx, float cy_base, float cz,
                     float lx, float lz, float altura,
                     float rT, float gT, float bT,
                     float rF, float gF, float bF,
                     float rB, float gB, float bB,
                     float rR, float gR, float bR,
                     float rL, float gL, float bL);

// Primitivas novas, pequenas e genéricas (definidas em Main.cpp na Fase 5):
void desenharLinha3D(float x0, float y, float z0,
                     float x1, float z1,
                     float largura,
                     float r, float g, float b);              // Beam/Wall
void desenharCirculo3D(float cx, float y, float cz,
                       float raio,
                       float r, float g, float b);            // Area/Aura/Ring/Disco
void desenharArco3D(float cx, float y, float cz,
                    float raioInterno, float raioExterno,
                    float anguloCentral, float meiaAbertura,
                    float r, float g, float b);               // Arc/Meia-Lua

// Paralelepípedo orientado pela direção — usado pelas 3 Armas Inteligentes.
//   cx, cy, cz : centro XYZ do projétil (Y vem de r.posicao.y para seguir a origem)
//   yaw        : atan2(direcao.z, direcao.x) — orienta o eixo longo
//   cr/cg/cb   : cor base da arma (definida em FormaData.corR/G/B)
void desenharMissilOrientado3D(float cx, float cy, float cz, float yaw,
                                float cr, float cg, float cb);

// ---------------------------------------------------------------------------
// desenharSkill — despacho de renderização por FormaType. Lê cor/parâmetros
//   da SkillData (componente Forma) e posição/ângulo da instância runtime.
//   Cores default amarelas para manter paridade visual com o projétil atual;
//   builds podem sobrescrever via campos futuros de cor em FormaData.
// ---------------------------------------------------------------------------
inline void desenharSkill(const SkillData& s, const RuntimeSkill& r) {
    if (!r.ativo) return;

    switch (s.forma.tipo) {
        case FORMA_PROJECTILE:
        case FORMA_CONE:
        case FORMA_RING:
        case FORMA_PRISM: {
                float tam = r.raioColisao * 0.8f;
                float py  = r.posicao.y;   // altura real da instância (origem da pistola ou do fantasma)
                bool temCor = (s.forma.corR > 0.001f ||
                               s.forma.corG > 0.001f ||
                               s.forma.corB > 0.001f);
                // Armas Inteligentes: homing + cor definida → paralelepípedo orientado.
                bool ehMissil = (temCor && s.movimento.tipo == MOV_HOMING);
                if (ehMissil) {
                    float yaw = std::atan2(r.direcao.z, r.direcao.x);
                    desenharMissilOrientado3D(r.posicao.x, py, r.posicao.z, yaw,
                                              s.forma.corR, s.forma.corG, s.forma.corB);
                } else if (temCor) {
                    float cr = s.forma.corR, cg = s.forma.corG, cb = s.forma.corB;
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
                break;
            }
        case FORMA_BEAM:
        case FORMA_WALL: {
            // Linha da posição na direção, comprimento da forma.
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
            // frente de onda: linha perpendicular à direção, largura da frente
            float px = -r.direcao.z, pz = r.direcao.x;     // perpendicular
            float meia = s.forma.largura * 0.5f;
            desenharLinha3D(r.posicao.x - px * meia, 0.06f, r.posicao.z - pz * meia,
                            r.posicao.x + px * meia,        r.posicao.z + pz * meia,
                            0.20f,
                            0.20f, 0.60f, 1.0f);
            break;
        }
        case FORMA_CHAIN: {
            // ponto de salto: bloco pequeno azulado
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

// ---------------------------------------------------------------------------
// desenharTodasSkills — itera o pool e desenha cada instância pela sua Forma.
//   Chamado pelo render loop em vez do antigo laço sobre tirosNaTela.
//   Recebe a build ativa (todas as instâncias do pool a compartilham) e o pool.
// ---------------------------------------------------------------------------
inline void desenharTodasSkills(const SkillData& buildAtiva,
                                const RuntimeSkill* pool, int n) {
    for (int i = 0; i < n; ++i)
        desenharSkill(buildAtiva, pool[i]);
}

#endif // SKILL_RENDER_H