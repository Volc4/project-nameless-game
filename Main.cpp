// =============================================================================
//  Stand Survivor — Main.cpp
//  Renderização isométrica 3D clássica com OpenGL/GLUT
//
//  Câmera: projeção ortográfica + rotação 45° em Y + 35.264° em X
//  Toda lógica de jogo preservada de GameLogic.h / Entities.h / MathUtils.h
//
//  CORREÇÕES APLICADAS:
//    P1 — Hitbox alinhada ao tamanho visual de cada cubo
//    P2 — Chão calculado dinamicamente para cobrir toda a câmera
//    P3 — Pausa instantânea no frame exato do level up, sem timer artificial
//    P4 — Normalização vetorial correta do movimento (velocidade idêntica em toda direção)
//    P5 — HUD de tensão com clamp, borda de sobrecarga e sincronização em tempo real
// =============================================================================

#include <GL/glut.h>
#include "GameLogic.h"
#include <iostream>
#include <cstdio>
#include <cmath>

// ---------------------------------------------------------------------------
// Estado global
// ---------------------------------------------------------------------------
EstadoDoJogo jogo;
int tempoAnterior = 0;
bool teclasPressionadas[256] = {false};
bool disparouNesteFrame = false;  // setado em cliqueMouse, lido e limpo em timer()

static int JANELA_W = 900;
static int JANELA_H = 900;

// ---------------------------------------------------------------------------
// Parâmetros da câmera isométrica REAL
// ---------------------------------------------------------------------------
static const float CAM_ANGULO_Y   = 45.0f;
static const float CAM_ANGULO_X   = 35.264f;
static const float CAM_ORTHO      = 18.0f;
static const float CAM_Z_NEAR     = -300.0f;
static const float CAM_Z_FAR      =  300.0f;

// ---------------------------------------------------------------------------
// Matrizes gravadas para conversão de clique → mundo
// ---------------------------------------------------------------------------
static GLdouble matModelview[16];
static GLdouble matProjection[16];
static GLint    viewport[4];

void gravarMatrizes() {
    glGetDoublev(GL_MODELVIEW_MATRIX,  matModelview);
    glGetDoublev(GL_PROJECTION_MATRIX, matProjection);
    glGetIntegerv(GL_VIEWPORT,         viewport);
}

// ---------------------------------------------------------------------------
// Converte pixel de tela → ponto no plano Y=0 do mundo
// ---------------------------------------------------------------------------
Vetor3D cliqueParaMundo(int px, int py) {
    float syGL = (float)(viewport[3] - py);

    GLdouble wx0, wy0, wz0;
    GLdouble wx1, wy1, wz1;
    gluUnProject(px, syGL, 0.0, matModelview, matProjection, viewport,
                 &wx0, &wy0, &wz0);
    gluUnProject(px, syGL, 1.0, matModelview, matProjection, viewport,
                 &wx1, &wy1, &wz1);

    float dy = (float)(wy1 - wy0);
    float t  = (dy != 0.0f) ? (float)(-wy0 / dy) : 0.0f;

    Vetor3D resultado;
    resultado.x = (float)(wx0 + t * (wx1 - wx0));
    resultado.y = 0.0f;
    resultado.z = (float)(wz0 + t * (wz1 - wz0));
    return resultado;
}

// ---------------------------------------------------------------------------
// Aplica a transformação de câmera isométrica no ModelView
// ---------------------------------------------------------------------------
void aplicarCameraIsometrica() {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glRotatef(CAM_ANGULO_X, 1.0f, 0.0f, 0.0f);
    glRotatef(CAM_ANGULO_Y, 0.0f, 1.0f, 0.0f);

    glTranslatef(-jogo.protagonista.posicao.x,
                  0.0f,
                 -jogo.protagonista.posicao.z);
}

// ---------------------------------------------------------------------------
// desenharBloco3D — renderiza um bloco com 5 faces visíveis (topo + 4 laterais)
//
//  Parâmetros:
//    cx, cy_base, cz — centro X, base Y, centro Z
//    lx, lz          — meia-largura em X e Z
//    altura          — altura total em Y
//    rT/gT/bT        — topo         (mais iluminado)
//    rF/gF/bF        — frente  Z-   (iluminação média)
//    rB/gB/bB        — trás    Z+   (mais escuro)
//    rR/gR/bR        — direita  X+  (intermediário)
//    rL/gL/bL        — esquerda X-  (escuro)
// ---------------------------------------------------------------------------
void desenharBloco3D(float cx, float cy_base, float cz,
                     float lx, float lz, float altura,
                     float rT, float gT, float bT,
                     float rF, float gF, float bF,
                     float rB, float gB, float bB,
                     float rR, float gR, float bR,
                     float rL, float gL, float bL)
{
    float x0 = cx - lx;
    float x1 = cx + lx;
    float z0 = cz - lz;
    float z1 = cz + lz;
    float y0 = cy_base;
    float y1 = cy_base + altura;

    // Topo
    glColor3f(rT, gT, bT);
    glBegin(GL_QUADS);
        glVertex3f(x0, y1, z0);
        glVertex3f(x1, y1, z0);
        glVertex3f(x1, y1, z1);
        glVertex3f(x0, y1, z1);
    glEnd();

    // Frente (Z negativo)
    glColor3f(rF, gF, bF);
    glBegin(GL_QUADS);
        glVertex3f(x0, y0, z0);
        glVertex3f(x1, y0, z0);
        glVertex3f(x1, y1, z0);
        glVertex3f(x0, y1, z0);
    glEnd();

    // Trás (Z positivo)
    glColor3f(rB, gB, bB);
    glBegin(GL_QUADS);
        glVertex3f(x0, y0, z1);
        glVertex3f(x1, y0, z1);
        glVertex3f(x1, y1, z1);
        glVertex3f(x0, y1, z1);
    glEnd();

    // Direita (X positivo)
    glColor3f(rR, gR, bR);
    glBegin(GL_QUADS);
        glVertex3f(x1, y0, z0);
        glVertex3f(x1, y0, z1);
        glVertex3f(x1, y1, z1);
        glVertex3f(x1, y1, z0);
    glEnd();

    // Esquerda (X negativo)
    glColor3f(rL, gL, bL);
    glBegin(GL_QUADS);
        glVertex3f(x0, y0, z0);
        glVertex3f(x0, y0, z1);
        glVertex3f(x0, y1, z1);
        glVertex3f(x0, y1, z0);
    glEnd();
}

// ---------------------------------------------------------------------------
// Sombra projetada no chão — quad preto semitransparente em y~0
// Requer GL_BLEND habilitado pelo chamador.
// ---------------------------------------------------------------------------
void desenharSombra(float cx, float cz, float rx, float rz) {
    float x0 = cx - rx;
    float x1 = cx + rx;
    float z0 = cz - rz;
    float z1 = cz + rz;
    float y  = 0.005f;

    glColor4f(0.0f, 0.0f, 0.0f, 0.45f);
    glBegin(GL_QUADS);
        glVertex3f(x0, y, z0);
        glVertex3f(x1, y, z0);
        glVertex3f(x1, y, z1);
        glVertex3f(x0, y, z1);
    glEnd();
}

// ---------------------------------------------------------------------------
// CORREÇÃO P2 — Chão com tiles de espessura, cobertura dinâmica
//
//  O raio de tiles visíveis é calculado a partir do volume ortográfico da
//  câmera e do ângulo de rotação, garantindo que o chão cubra toda a janela
//  independente de resolução ou posição do jogador.
//
//  Raciocínio:
//    - O volume ortográfico em Y cobre ±CAM_ORTHO unidades de tela.
//    - Com câmera 45°Y + 35.264°X, o eixo mais "esticado" no plano XZ
//      corresponde a aproximadamente CAM_ORTHO * aspecto * sqrt(2) unidades
//      de mundo. Multiplicamos por um fator de segurança (1.5) para cobrir
//      bordas diagonais e movimento.
//    - Somamos 4 tiles extras de margem para eliminar qualquer pop-in.
// ---------------------------------------------------------------------------
void desenharChao() {
    const float TILE_H = 0.18f;
    const float HALF   = 0.5f;

    float aspecto = (JANELA_H > 0) ? (float)JANELA_W / (float)JANELA_H : 1.0f;

    // Raio dinâmico: volume ortográfico * diagonal isométrica * margem
    // A câmera 45°Y estica tiles na diagonal, multiplicamos por sqrt(2).
    // O fator 1.6 é a margem de segurança para nunca mostrar o fundo.
    int raio = (int)ceilf(CAM_ORTHO * (aspecto > 1.0f ? aspecto : 1.0f) * 1.4142f * 1.6f) + 4;

    float px = jogo.protagonista.posicao.x;
    float pz = jogo.protagonista.posicao.z;
    int ox = (int)floorf(px);
    int oz = (int)floorf(pz);

    for (int gx = ox - raio; gx <= ox + raio; gx++) {
        for (int gz = oz - raio; gz <= oz + raio; gz++) {
            int par = ((gx % 2 + 2) % 2) ^ ((gz % 2 + 2) % 2);

            float cT  = par ? 0.22f : 0.16f;
            float cTg = par ? 0.23f : 0.17f;

            float cx  = (float)gx;
            float cz_ = (float)gz;

            // Face superior
            glColor3f(cT, cTg, cT);
            glBegin(GL_QUADS);
                glVertex3f(cx - HALF, TILE_H, cz_ - HALF);
                glVertex3f(cx + HALF, TILE_H, cz_ - HALF);
                glVertex3f(cx + HALF, TILE_H, cz_ + HALF);
                glVertex3f(cx - HALF, TILE_H, cz_ + HALF);
            glEnd();

            // Frente (Z negativo)
            float cF = cT * 0.60f;
            glColor3f(cF, cF + 0.01f, cF);
            glBegin(GL_QUADS);
                glVertex3f(cx - HALF, 0.0f,   cz_ - HALF);
                glVertex3f(cx + HALF, 0.0f,   cz_ - HALF);
                glVertex3f(cx + HALF, TILE_H,  cz_ - HALF);
                glVertex3f(cx - HALF, TILE_H,  cz_ - HALF);
            glEnd();

            // Lado direito (X positivo)
            float cR = cT * 0.75f;
            glColor3f(cR, cR + 0.01f, cR);
            glBegin(GL_QUADS);
                glVertex3f(cx + HALF, 0.0f,   cz_ - HALF);
                glVertex3f(cx + HALF, 0.0f,   cz_ + HALF);
                glVertex3f(cx + HALF, TILE_H,  cz_ + HALF);
                glVertex3f(cx + HALF, TILE_H,  cz_ - HALF);
            glEnd();
        }
    }
}

// ---------------------------------------------------------------------------
// Anel no chão indicando raio do Parry ativo
// ---------------------------------------------------------------------------
void desenharAroParry(float cx, float cz, float raio) {
    const int SEG = 48;
    const float PI2 = 2.0f * 3.14159265f;
    glColor4f(0.0f, 0.9f, 0.9f, 0.7f);
    glLineWidth(2.5f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < SEG; ++i) {
        float ang = (float)i / (float)SEG * PI2;
        glVertex3f(cx + raio * cosf(ang), 0.22f, cz + raio * sinf(ang));
    }
    glEnd();
    glLineWidth(1.0f);
}

// ---------------------------------------------------------------------------
// Renderização do jogador
//
//  CORREÇÃO P1: raioColisao do jogador = 0.70 (= LARGURA visual).
//  O valor é definido em inicializarJogo() onde jog.raioColisao = 0.70f.
// ---------------------------------------------------------------------------
void desenharJogador() {
    if (!jogo.protagonista.vivo) return;

    const float BASE_Y  = 0.18f;
    const float LARGURA = 0.7f;   // meia-largura — igual ao raioColisao definido em inicializarJogo
    const float ALTURA  = 1.8f;

    bool piscando       = (jogo.protagonista.temporizadorIframe > 0.0f);
    bool frameVisivel   = ((glutGet(GLUT_ELAPSED_TIME) / 90) % 2 == 0);
    bool mostrarInverso = piscando && !frameVisivel;

    float rT, gT, bT;
    if (mostrarInverso) {
        rT = 1.0f; gT = 1.0f; bT = 0.5f;
    } else {
        rT = 0.25f; gT = 0.45f; bT = 1.0f;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    desenharSombra(jogo.protagonista.posicao.x, jogo.protagonista.posicao.z,
                   LARGURA * 1.1f, LARGURA * 1.1f);
    glDisable(GL_BLEND);

    desenharBloco3D(jogo.protagonista.posicao.x, BASE_Y,
                    jogo.protagonista.posicao.z,
                    LARGURA, LARGURA, ALTURA,
                    rT,           gT,           bT,
                    rT * 0.70f,   gT * 0.70f,   bT * 0.70f,
                    rT * 0.45f,   gT * 0.45f,   bT * 0.45f,
                    rT * 0.85f,   gT * 0.85f,   bT * 0.85f,
                    rT * 0.55f,   gT * 0.55f,   bT * 0.55f);
}

// ---------------------------------------------------------------------------
// Renderização do Stand
// ---------------------------------------------------------------------------
void desenharStand() {
    const float BASE_Y  = 0.18f;
    const float LARGURA = 0.5f;
    const float ALTURA  = 1.1f;

    float t  = jogo.stand.tensaoAtual / 100.0f;
    float rT = t;
    float gT = 1.0f - t;
    float bT = 0.5f;

    if (jogo.stand.parryBemSucedido) {
        rT = 1.0f; gT = 1.0f; bT = 1.0f;
    }
    if (jogo.stand.emSobrecarga) {
        bool pulso = ((glutGet(GLUT_ELAPSED_TIME) / 120) % 2 == 0);
        rT = pulso ? 1.0f : 0.7f;
        gT = 0.0f;
        bT = 0.0f;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    desenharSombra(jogo.stand.posicao.x, jogo.stand.posicao.z,
                   LARGURA * 1.1f, LARGURA * 1.1f);
    glDisable(GL_BLEND);

    desenharBloco3D(jogo.stand.posicao.x, BASE_Y,
                    jogo.stand.posicao.z,
                    LARGURA, LARGURA, ALTURA,
                    rT,           gT,           bT,
                    rT * 0.70f,   gT * 0.70f,   bT * 0.70f,
                    rT * 0.45f,   gT * 0.45f,   bT * 0.45f,
                    rT * 0.85f,   gT * 0.85f,   bT * 0.85f,
                    rT * 0.55f,   gT * 0.55f,   bT * 0.55f);
}

// ---------------------------------------------------------------------------
// Renderização dos inimigos
//
//  CORREÇÃO P1: lx e lz de cada tipo são iguais ao raioColisao definido
//  em invocarZumbi() no GameLogic.h:
//    NORMAL    lx=lz=0.75  raioColisao=0.75
//    RAPIDO    lx=lz=0.55  raioColisao=0.55
//    TANK      lx=lz=1.10  raioColisao=1.10
//    ATIRADOR  lx=lz=0.75  raioColisao=0.75
//    EXPLOSIVO lx=lz=0.85  raioColisao=0.85
// ---------------------------------------------------------------------------
void desenharInimigos() {
    const float BASE_Y = 0.18f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (size_t i = 0; i < jogo.horda.size(); ++i) {
        const Zumbi& z = jogo.horda[i];
        if (!z.vivo) continue;

        float lx, lz, alt;
        float rT, gT, bT;

        switch (z.tipo) {
            case NORMAL:
                lx = 0.75f; lz = 0.75f; alt = 1.4f;
                rT = 1.0f;  gT = 0.10f; bT = 0.10f;
                break;
            case RAPIDO:
                lx = 0.55f; lz = 0.55f; alt = 0.85f;
                rT = 1.0f;  gT = 0.45f; bT = 0.0f;
                break;
            case TANK:
                lx = 1.10f; lz = 1.10f; alt = 2.2f;
                rT = 0.50f; gT = 0.0f;  bT = 0.85f;
                break;
            case ATIRADOR:
                lx = 0.75f; lz = 0.75f; alt = 1.4f;
                rT = 0.90f; gT = 0.0f;  bT = 0.75f;
                break;
            case EXPLOSIVO:
                lx = 0.85f; lz = 0.85f; alt = 1.6f;
                rT = 1.0f;  gT = 0.80f; bT = 0.0f;
                break;
            default:
                lx = 0.75f; lz = 0.75f; alt = 1.4f;
                rT = 1.0f;  gT = 0.10f; bT = 0.10f;
                break;
        }

        desenharSombra(z.posicao.x, z.posicao.z, lx * 1.1f, lz * 1.1f);

        desenharBloco3D(z.posicao.x, BASE_Y, z.posicao.z,
                        lx, lz, alt,
                        rT,           gT,           bT,
                        rT * 0.68f,   gT * 0.68f,   bT * 0.68f,
                        rT * 0.45f,   gT * 0.45f,   bT * 0.45f,
                        rT * 0.82f,   gT * 0.82f,   bT * 0.82f,
                        rT * 0.55f,   gT * 0.55f,   bT * 0.55f);
    }

    glDisable(GL_BLEND);
}

// ---------------------------------------------------------------------------
// Renderização dos projéteis
// ---------------------------------------------------------------------------
void desenharProjeteis() {
    const float BASE_Y = 0.5f;
    const float L      = 0.20f;
    const float ALT    = 0.20f;

    for (size_t i = 0; i < jogo.tirosNaTela.size(); ++i) {
        const Projetil& p = jogo.tirosNaTela[i];
        if (!p.ativo) continue;

        desenharBloco3D(p.posicao.x, BASE_Y, p.posicao.z,
                        L, L, ALT,
                        1.0f,  1.0f,  0.20f,
                        0.80f, 0.80f, 0.0f,
                        0.55f, 0.55f, 0.0f,
                        0.90f, 0.90f, 0.05f,
                        0.65f, 0.65f, 0.0f);
    }
}

// ---------------------------------------------------------------------------
// Renderização das gemas de XP
// ---------------------------------------------------------------------------
void desenharGemas() {
    const float BASE_Y = 0.18f;
    const float LX     = 0.32f;
    const float LZ     = 0.32f;
    const float ALT    = 0.55f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (size_t i = 0; i < jogo.gemas.size(); ++i) {
        const GemaXP& g = jogo.gemas[i];
        if (g.coletada) continue;

        float pulso = 0.85f + 0.15f * sinf(glutGet(GLUT_ELAPSED_TIME) * 0.004f + g.posicao.x);

        desenharSombra(g.posicao.x, g.posicao.z, LX, LZ);

        desenharBloco3D(g.posicao.x, BASE_Y, g.posicao.z,
                        LX, LZ, ALT,
                        0.10f * pulso, 1.0f * pulso, 0.35f * pulso,
                        0.05f,         0.65f,         0.20f,
                        0.03f,         0.40f,         0.12f,
                        0.08f,         0.80f,         0.28f,
                        0.04f,         0.50f,         0.16f);
    }

    glDisable(GL_BLEND);
}

// ---------------------------------------------------------------------------
// HUD — Interface 2D sobreposta à cena 3D
//
//  CORREÇÃO P5: barra de tensão lê sempre jogo.stand.tensaoAtual em tempo
//  real, cores com clamp(0,1) para evitar artefatos, borda vermelha pulsante
//  quando em sobrecarga, e texto diferenciado para cada estado do Parry.
// ---------------------------------------------------------------------------
void desenharHUD() {
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, JANELA_W, 0, JANELA_H);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // ---- Barra de HP ----
    {
        const int   MAX_HP = 3;
        const float BAR_W  = 120.0f;
        const float BAR_H  = 14.0f;
        const float BAR_X  = 12.0f;
        const float BAR_Y  = JANELA_H - 26.0f;
        float propHP = (float)jogo.protagonista.hp / (float)MAX_HP;
        if (propHP < 0.0f) propHP = 0.0f;
        if (propHP > 1.0f) propHP = 1.0f;

        glColor3f(0.25f, 0.1f, 0.1f);
        glBegin(GL_QUADS);
            glVertex2f(BAR_X,         BAR_Y);
            glVertex2f(BAR_X + BAR_W, BAR_Y);
            glVertex2f(BAR_X + BAR_W, BAR_Y + BAR_H);
            glVertex2f(BAR_X,         BAR_Y + BAR_H);
        glEnd();
        glColor3f(0.85f, 0.15f, 0.15f);
        glBegin(GL_QUADS);
            glVertex2f(BAR_X,                   BAR_Y);
            glVertex2f(BAR_X + BAR_W * propHP,  BAR_Y);
            glVertex2f(BAR_X + BAR_W * propHP,  BAR_Y + BAR_H);
            glVertex2f(BAR_X,                   BAR_Y + BAR_H);
        glEnd();
        char buf[32];
        sprintf(buf, "HP  %d/%d", jogo.protagonista.hp, MAX_HP);
        glColor3f(1.0f, 0.85f, 0.85f);
        glRasterPos2f(BAR_X + BAR_W + 8.0f, BAR_Y + 2.0f);
        for (const char* c = buf; *c; ++c)
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }

    // ---- Barra de Tensão ----
    // Comportamento do sistema:
    //   Normal     : sobe ao atirar (18/s), desce em repouso (5/s)
    //   Sobrecarga : drena sozinha (12/s), disparo bloqueado, dano zero
    //   Saída      : ao chegar em 0% durante sobrecarga, tudo volta ao normal
    {
        const float BAR_W = 130.0f;
        const float BAR_H = 14.0f;
        const float BAR_X = 12.0f;
        const float BAR_Y = JANELA_H - 48.0f;

        float tensaoReal = jogo.stand.tensaoAtual;  // sempre lido em tempo real
        float prop = tensaoReal / 100.0f;
        if (prop < 0.0f) prop = 0.0f;
        if (prop > 1.0f) prop = 1.0f;

        // Fundo
        glColor3f(0.15f, 0.10f, 0.05f);
        glBegin(GL_QUADS);
            glVertex2f(BAR_X,         BAR_Y);
            glVertex2f(BAR_X + BAR_W, BAR_Y);
            glVertex2f(BAR_X + BAR_W, BAR_Y + BAR_H);
            glVertex2f(BAR_X,         BAR_Y + BAR_H);
        glEnd();

        // Cor do preenchimento:
        //   Normal    : verde (0%) → amarelo (50%) → vermelho (100%)
        //   Sobrecarga: vermelho pulsante (indicando drenagem ativa)
        float cr, cg, cb;
        if (!jogo.stand.emSobrecarga) {
            cr = prop * 2.0f;           if (cr > 1.0f) cr = 1.0f;
            cg = (1.0f - prop) * 2.0f; if (cg > 1.0f) cg = 1.0f;
            cb = 0.0f;
        } else {
            bool pulso = ((glutGet(GLUT_ELAPSED_TIME) / 180) % 2 == 0);
            cr = pulso ? 1.0f : 0.55f;
            cg = 0.0f;
            cb = 0.0f;
        }

        // Preenchimento proporcional
        glColor3f(cr, cg, cb);
        glBegin(GL_QUADS);
            glVertex2f(BAR_X,                 BAR_Y);
            glVertex2f(BAR_X + BAR_W * prop,  BAR_Y);
            glVertex2f(BAR_X + BAR_W * prop,  BAR_Y + BAR_H);
            glVertex2f(BAR_X,                 BAR_Y + BAR_H);
        glEnd();

        // Borda: vermelha grossa em sobrecarga, cinza fina no normal
        if (jogo.stand.emSobrecarga) {
            glColor3f(1.0f, 0.0f, 0.0f);
            glLineWidth(2.0f);
        } else {
            glColor3f(0.4f, 0.4f, 0.4f);
            glLineWidth(1.0f);
        }
        glBegin(GL_LINE_LOOP);
            glVertex2f(BAR_X,         BAR_Y);
            glVertex2f(BAR_X + BAR_W, BAR_Y);
            glVertex2f(BAR_X + BAR_W, BAR_Y + BAR_H);
            glVertex2f(BAR_X,         BAR_Y + BAR_H);
        glEnd();
        glLineWidth(1.0f);

        // Texto de estado à direita da barra
        char buf[64];
        if (jogo.stand.emSobrecarga) {
            sprintf(buf, "SOBRECARGA! %.0f%%", tensaoReal);
            glColor3f(1.0f, 0.3f, 0.3f);
        } else if (jogo.stand.parryBemSucedido) {
            sprintf(buf, "Tensao %.0f%% [PARRY!]", tensaoReal);
            glColor3f(0.4f, 1.0f, 0.4f);
        } else if (jogo.atirandoAgora) {
            sprintf(buf, "Tensao %.0f%% ^", tensaoReal);   // ^ indica subindo
            glColor3f(cr, (cg + 0.2f > 1.0f ? 1.0f : cg + 0.2f), 0.2f);
        } else {
            sprintf(buf, "Tensao %.0f%%", tensaoReal);
            glColor3f(0.75f, 0.75f, 0.45f);
        }
        glRasterPos2f(BAR_X + BAR_W + 8.0f, BAR_Y + 2.0f);
        for (const char* c = buf; *c; ++c)
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);

        // Indicador extra abaixo: avisa que disparo está bloqueado
        if (jogo.stand.emSobrecarga) {
            bool piscaTxt = ((glutGet(GLUT_ELAPSED_TIME) / 400) % 2 == 0);
            if (piscaTxt) {
                const char* aviso = "[ DISPARO BLOQUEADO ]";
                glColor3f(1.0f, 0.2f, 0.2f);
                glRasterPos2f(BAR_X, BAR_Y - 14.0f);
                for (const char* c = aviso; *c; ++c)
                    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
            }
        }
    }

    // ---- Barra de XP ----
    {
        const float BAR_W = 220.0f;
        const float BAR_H = 10.0f;
        const float BAR_X = 12.0f;
        const float BAR_Y = JANELA_H - 66.0f;
        float prop = (jogo.protagonista.xpParaProximoNivel > 0)
                   ? (float)jogo.protagonista.xpAtual / (float)jogo.protagonista.xpParaProximoNivel
                   : 0.0f;
        if (prop > 1.0f) prop = 1.0f;

        glColor3f(0.10f, 0.10f, 0.15f);
        glBegin(GL_QUADS);
            glVertex2f(BAR_X,         BAR_Y);
            glVertex2f(BAR_X + BAR_W, BAR_Y);
            glVertex2f(BAR_X + BAR_W, BAR_Y + BAR_H);
            glVertex2f(BAR_X,         BAR_Y + BAR_H);
        glEnd();
        glColor3f(0.5f, 0.8f, 1.0f);
        glBegin(GL_QUADS);
            glVertex2f(BAR_X,                  BAR_Y);
            glVertex2f(BAR_X + BAR_W * prop,   BAR_Y);
            glVertex2f(BAR_X + BAR_W * prop,   BAR_Y + BAR_H);
            glVertex2f(BAR_X,                  BAR_Y + BAR_H);
        glEnd();
        char buf[64];
        sprintf(buf, "Nivel %d   XP %d/%d",
                jogo.protagonista.nivel,
                jogo.protagonista.xpAtual,
                jogo.protagonista.xpParaProximoNivel);
        glColor3f(0.75f, 0.90f, 1.0f);
        glRasterPos2f(BAR_X, BAR_Y - 14.0f);
        for (const char* c = buf; *c; ++c)
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }

    // ---- Tempo de sobrevivência ----
    {
        char buf[32];
        int seg = (int)jogo.tempoSobrevivido;
        sprintf(buf, "Tempo  %02d:%02d", seg / 60, seg % 60);
        glColor3f(0.80f, 0.80f, 0.80f);
        glRasterPos2f(12.0f, JANELA_H - 96.0f);
        for (const char* c = buf; *c; ++c)
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }

    // ---- Indicador de Parry ----
    {
        const char* parryTxt;
        float pr, pg, pb;
        if (jogo.stand.parryAtivo) {
            parryTxt = "PARRY ATIVO!";
            pr = 0.0f; pg = 1.0f; pb = 1.0f;
        } else if (jogo.stand.emSobrecarga) {
            parryTxt = "Parry: SOBRECARGA";
            pr = 1.0f; pg = 0.2f; pb = 0.2f;
        } else if (jogo.stand.temporizadorCooldown > 0.0f) {
            parryTxt = "Parry: recarga";
            pr = 0.5f; pg = 0.5f; pb = 0.5f;
        } else {
            parryTxt = "Parry: pronto  [ESPACO]";
            pr = 0.3f; pg = 0.9f; pb = 0.3f;
        }
        glColor3f(pr, pg, pb);
        glRasterPos2f(12.0f, JANELA_H - 112.0f);
        for (const char* c = parryTxt; *c; ++c)
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }

    // ---- Controles ----
    {
        const char* ctrl = "WASD: mover  |  Clique: atirar  |  Espaco: Parry  |  R: reiniciar";
        glColor3f(0.45f, 0.45f, 0.45f);
        glRasterPos2f(10.0f, 10.0f);
        for (const char* c = ctrl; *c; ++c)
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
    }

    // ---- Tela de morte ----
    if (!jogo.protagonista.vivo) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.0f, 0.0f, 0.0f, 0.55f);
        glBegin(GL_QUADS);
            glVertex2f(0,        0);
            glVertex2f(JANELA_W, 0);
            glVertex2f(JANELA_W, JANELA_H);
            glVertex2f(0,        JANELA_H);
        glEnd();
        glDisable(GL_BLEND);

        const char* msg1 = "VOCE MORREU";
        const char* msg2 = "Pressione R para reiniciar";
        glColor3f(1.0f, 0.20f, 0.20f);
        glRasterPos2f((float)JANELA_W / 2.0f - 70.0f, (float)JANELA_H / 2.0f + 15.0f);
        for (const char* c = msg1; *c; ++c)
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
        glColor3f(0.85f, 0.85f, 0.85f);
        glRasterPos2f((float)JANELA_W / 2.0f - 105.0f, (float)JANELA_H / 2.0f - 10.0f);
        for (const char* c = msg2; *c; ++c)
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    // ---- Tela de Level Up (CORREÇÃO P3) ----
    // Mostrada enquanto jogo.pausadoParaUpgrade == true.
    // Instrui o jogador a pressionar E para continuar (tratado em pressionarTecla).
    if (jogo.pausadoParaUpgrade) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.02f, 0.04f, 0.12f, 0.72f);
        glBegin(GL_QUADS);
            glVertex2f(0,        0);
            glVertex2f(JANELA_W, 0);
            glVertex2f(JANELA_W, JANELA_H);
            glVertex2f(0,        JANELA_H);
        glEnd();
        glDisable(GL_BLEND);

        char buf[64];
        sprintf(buf, "LEVEL UP!  Nivel %d", jogo.protagonista.nivel);
        glColor3f(1.0f, 0.9f, 0.2f);
        glRasterPos2f((float)JANELA_W / 2.0f - 95.0f, (float)JANELA_H / 2.0f + 12.0f);
        for (const char* c = buf; *c; ++c)
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

        const char* msg2 = "Pressione E para continuar";
        glColor3f(0.85f, 0.85f, 0.85f);
        glRasterPos2f((float)JANELA_W / 2.0f - 105.0f, (float)JANELA_H / 2.0f - 14.0f);
        for (const char* c = msg2; *c; ++c)
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_DEPTH_TEST);
}

// ---------------------------------------------------------------------------
// Inicialização do jogo
//
//  CORREÇÃO P1: raioColisao do jogador = 0.70 (= LARGURA visual do cubo).
// ---------------------------------------------------------------------------
void inicializarJogo() {
    jogo.protagonista.posicao.x = 0.0f;
    jogo.protagonista.posicao.y = 0.0f;
    jogo.protagonista.posicao.z = 0.0f;
    jogo.protagonista.velocidade         = 10.0f;
    jogo.protagonista.vivo               = true;
    jogo.protagonista.raioColisao        = 0.70f;  // igual à LARGURA visual (P1)
    jogo.protagonista.hp                 = 3;
    jogo.protagonista.temporizadorIframe = 0.0f;
    jogo.protagonista.duracaoIframe      = 1.2f;
    jogo.protagonista.xpAtual            = 0;
    jogo.protagonista.nivel              = 1;
    jogo.protagonista.xpParaProximoNivel = calcularXpParaNivel(1);

    jogo.stand.tensaoAtual          = 0.0f;
    jogo.stand.emSobrecarga         = false;
    jogo.stand.parryAtivo           = false;
    jogo.stand.temporizadorParry    = 0.0f;
    jogo.stand.cooldownParry        = COOLDOWN_PARRY_SEGUNDOS;
    jogo.stand.temporizadorCooldown = 0.0f;
    jogo.stand.parryBemSucedido     = false;
    jogo.stand.temporizadorFeedback = 0.0f;

    jogo.tempoSobrevivido    = 0.0f;
    jogo.tempoUltimoSpawn    = 0.0f;
    jogo.pausadoParaUpgrade  = false;
    jogo.nivelAntesDaEscolha = 0;
    jogo.atirandoAgora       = false;

    glClearColor(0.06f, 0.07f, 0.06f, 1.0f);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspecto = (float)JANELA_W / (float)JANELA_H;
    glOrtho(-CAM_ORTHO * aspecto,  CAM_ORTHO * aspecto,
            -CAM_ORTHO,             CAM_ORTHO,
             CAM_Z_NEAR,            CAM_Z_FAR);

    glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
}

// ---------------------------------------------------------------------------
// Renderização principal
// ---------------------------------------------------------------------------
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    aplicarCameraIsometrica();

    desenharChao();

    desenharGemas();
    desenharProjeteis();
    desenharInimigos();
    desenharStand();
    desenharJogador();

    if (jogo.stand.parryAtivo) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        desenharAroParry(jogo.stand.posicao.x,
                         jogo.stand.posicao.z,
                         RAIO_EXPULSAO_PARRY);
        glDisable(GL_BLEND);
    }

    gravarMatrizes();

    desenharHUD();

    glutSwapBuffers();
}

// ---------------------------------------------------------------------------
// Redimensionamento de janela
// ---------------------------------------------------------------------------
void redimensionar(int w, int h) {
    if (h == 0) h = 1;
    JANELA_W = w;
    JANELA_H = h;
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspecto = (float)w / (float)h;
    glOrtho(-CAM_ORTHO * aspecto,  CAM_ORTHO * aspecto,
            -CAM_ORTHO,             CAM_ORTHO,
             CAM_Z_NEAR,            CAM_Z_FAR);
    glMatrixMode(GL_MODELVIEW);
}

// ---------------------------------------------------------------------------
// Teclado
//
//  CORREÇÃO P3: tecla 'E' fecha a tela de level up imediatamente,
//  retomando o jogo sem nenhum timer artificial.
// ---------------------------------------------------------------------------
void pressionarTecla(unsigned char key, int x, int y) {
    teclasPressionadas[key] = true;

    if (key == ' ') {
        tentarAtivarParry(jogo.stand);
    }

    // Fechar tela de Level Up instantaneamente com E
    if ((key == 'e' || key == 'E') && jogo.pausadoParaUpgrade) {
        jogo.pausadoParaUpgrade = false;
        // Sincroniza tempoAnterior para evitar delta gigante após a pausa
        tempoAnterior = glutGet(GLUT_ELAPSED_TIME);
        std::cout << "[LEVEL UP] Nivel " << jogo.protagonista.nivel
                  << " — continuando." << std::endl;
    }

    if ((key == 'r' || key == 'R') && !jogo.protagonista.vivo) {
        jogo.horda.clear();
        jogo.tirosNaTela.clear();
        jogo.gemas.clear();
        inicializarJogo();
    }
}

void soltarTecla(unsigned char key, int x, int y) {
    teclasPressionadas[key] = false;
}

// ---------------------------------------------------------------------------
// Mouse
//
//  Cada GLUT_DOWN cria um projétil e levanta disparouNesteFrame.
//  O timer() lê essa flag para subir a tensão e a reseta imediatamente depois.
// ---------------------------------------------------------------------------
void cliqueMouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        // Só tenta atirar (e marca o frame) se não estiver em sobrecarga.
        // dispararProjetil() também tem o mesmo guard interno — as duas
        // condições devem ser idênticas para evitar dessincronização.
        if (!jogo.stand.emSobrecarga) {
            Vetor3D posicaoAlvo = cliqueParaMundo(x, y);
            dispararProjetil(jogo, posicaoAlvo);
            disparouNesteFrame = true;
        }
    }
}

// ---------------------------------------------------------------------------
// Processamento de movimento
//
//  CORREÇÃO P4: normalização vetorial correta usando sqrtf().
//  O vetor (dx, dz) é calculado somando as contribuições de cada tecla
//  (cada tecla pode contribuir +/-1 em cada eixo), depois é normalizado
//  se seu comprimento for > 0. Isso garante velocidade idêntica em qualquer
//  combinação de teclas, inclusive diagonais de 45° ou 22.5°.
//
//  Mapeamento WASD para eixos isométricos (câmera 45°Y):
//    W → frente-direita (dx+, dz-)
//    S → trás-esquerda  (dx-, dz+)
//    D → frente-esquerda(dx+, dz+)
//    A → trás-direita   (dx-, dz-)
// ---------------------------------------------------------------------------
void processarMovimento(float deltaTime) {
    float dx = 0.0f;
    float dz = 0.0f;

    if (teclasPressionadas['w'] || teclasPressionadas['W']) { dx += 1.0f; dz -= 1.0f; }
    if (teclasPressionadas['s'] || teclasPressionadas['S']) { dx -= 1.0f; dz += 1.0f; }
    if (teclasPressionadas['d'] || teclasPressionadas['D']) { dx += 1.0f; dz += 1.0f; }
    if (teclasPressionadas['a'] || teclasPressionadas['A']) { dx -= 1.0f; dz -= 1.0f; }

    // Normalização vetorial completa — garante velocidade constante
    float comprimento = sqrtf(dx * dx + dz * dz);
    if (comprimento > 0.0001f) {
        dx /= comprimento;
        dz /= comprimento;
    }

    moverJogador(jogo.protagonista, dx, dz, deltaTime);
    atualizarEntidade(jogo.stand, jogo.protagonista);
}

// ---------------------------------------------------------------------------
// Loop principal
//
//  CORREÇÃO P3: quando pausadoParaUpgrade == true, o loop não avança nenhuma
//  lógica de jogo — nem timers, nem IA, nem movimento, nem spawn.
//  O frame continua sendo redesenhado (para manter a tela de level up visível)
//  mas absolutamente nada do estado do jogo é modificado.
//  A pausa é desativada apenas quando o jogador pressiona E (em pressionarTecla).
// ---------------------------------------------------------------------------
void timer(int value) {
    // PAUSA INSTANTÂNEA: nenhuma lógica executa enquanto pausadoParaUpgrade
    if (jogo.pausadoParaUpgrade) {
        glutPostRedisplay();
        glutTimerFunc(16, timer, 0);
        return;
    }

    int tempoAtual  = glutGet(GLUT_ELAPSED_TIME);
    float deltaTime = (tempoAtual - tempoAnterior) / 1000.0f;
    tempoAnterior   = tempoAtual;

    // Limita deltaTime para evitar saltos grandes após pausa ou lag
    if (deltaTime > 0.1f) deltaTime = 0.1f;

    if (!jogo.protagonista.vivo) {
        glutPostRedisplay();
        glutTimerFunc(16, timer, 0);
        return;
    }

    jogo.tempoSobrevivido += deltaTime;

    processarMovimento(deltaTime);

    // Transfere o flag de disparo para o estado do jogo e imediatamente limpa.
    // Só fica true no frame exato em que cliqueMouse() criou um projétil.
    // Isso garante: atirar = tensão sobe; não atirar = tensão cai.
    jogo.atirandoAgora = disparouNesteFrame;
    disparouNesteFrame = false;

    atualizarTimersStand(jogo, deltaTime);
    atualizarTensao(jogo, deltaTime);
    tentarExecutarParry(jogo);

    processarSpawn(jogo, deltaTime);
    processarIA(jogo, deltaTime);

    processarColisoesTiros(jogo);
    processarColisaoZumbiJogador(jogo, deltaTime);

    atualizarProjeteis(jogo, deltaTime);

    // processarColaDeGemas pode ativar pausadoParaUpgrade neste mesmo frame.
    // O loop principal só vai pausar na PRÓXIMA chamada de timer(), mas o
    // display() já vai mostrar a tela de level up neste mesmo frame porque
    // lê jogo.pausadoParaUpgrade em tempo real.
    processarColetaDeGemas(jogo);

    static float temporizadorLimpeza = 0.0f;
    temporizadorLimpeza += deltaTime;
    if (temporizadorLimpeza >= 5.0f) {
        limparEntidadesInativas(jogo);
        temporizadorLimpeza = 0.0f;
    }

    glutPostRedisplay();
    glutTimerFunc(16, timer, 0);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(JANELA_W, JANELA_H);
    glutCreateWindow("Stand Survivor — Isometrico 3D");

    inicializarJogo();
    gravarMatrizes();

    glutDisplayFunc(display);
    glutReshapeFunc(redimensionar);
    glutKeyboardFunc(pressionarTecla);
    glutKeyboardUpFunc(soltarTecla);
    glutMouseFunc(cliqueMouse);

    tempoAnterior = glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(0, timer, 0);

    glutMainLoop();
    return 0;
}