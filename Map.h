#ifndef MAP_H
#define MAP_H

#include <GL/glut.h>
#include <cmath>
#include "Entities.h"

// ---------------------------------------------------------------------------
// Estrutura para obstáculos físicos
// ---------------------------------------------------------------------------
struct Obstaculo {
    float x, z;
    float raio;
};

const int NUM_OBSTACULOS = 8;
const Obstaculo OBSTACULOS[NUM_OBSTACULOS] = {
    {15.0f,  20.0f, 2.2f},
    {-25.0f, 40.0f, 2.2f},
    {30.0f, -15.0f, 2.2f},
    {-35.0f,-35.0f, 2.2f},
    {5.0f,  -50.0f, 2.2f},
    {-55.0f, 15.0f, 2.2f},
    {45.0f,  45.0f, 2.2f},
    {-10.0f,-15.0f, 2.2f}
};

// ===========================================================================
// AUXILIARES DE MATERIAL
// ===========================================================================
inline void setEmissao(float r, float g, float b) {
    GLfloat e[] = { r, g, b, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, e);
}
inline void semEmissao() { setEmissao(0.0f, 0.0f, 0.0f); }

// ===========================================================================
// BANQUINHO DE BAR (alto, com aro de apoio para os pés)
// ===========================================================================
inline void desenharBanquinho(float x, float z) {
    glPushMatrix();
    glTranslatef(x, 0.0f, z);

    setEmissao(0.25f, 0.22f, 0.20f);

    // 4 pés de metal
    glColor3f(0.55f, 0.55f, 0.60f);
    float offP = 0.18f;
    float posP[4][2] = {{ offP, offP},{ offP,-offP},{-offP, offP},{-offP,-offP}};
    for (int i = 0; i < 4; ++i) {
        glPushMatrix();
            glTranslatef(posP[i][0], 0.55f, posP[i][1]);
            glScalef(0.06f, 1.1f, 0.06f);
            glutSolidCube(1.0f);
        glPopMatrix();
    }

    // Aro de apoio
    glColor3f(0.50f, 0.50f, 0.55f);
    float hAro = 0.38f;
    glPushMatrix(); glTranslatef( 0.0f, hAro,  offP); glScalef(0.38f, 0.05f, 0.05f); glutSolidCube(1.0f); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.0f, hAro, -offP); glScalef(0.38f, 0.05f, 0.05f); glutSolidCube(1.0f); glPopMatrix();
    glPushMatrix(); glTranslatef( offP, hAro,  0.0f); glScalef(0.05f, 0.05f, 0.38f); glutSolidCube(1.0f); glPopMatrix();
    glPushMatrix(); glTranslatef(-offP, hAro,  0.0f); glScalef(0.05f, 0.05f, 0.38f); glutSolidCube(1.0f); glPopMatrix();

    // Assento estofado
    glColor3f(0.88f, 0.86f, 0.84f);
    glPushMatrix(); glTranslatef(0.0f, 1.12f, 0.0f); glScalef(0.52f, 0.09f, 0.52f); glutSolidCube(1.0f); glPopMatrix();
    // aro cromado do assento
    glColor3f(0.65f, 0.65f, 0.70f);
    glPushMatrix(); glTranslatef(0.0f, 1.07f, 0.0f); glScalef(0.60f, 0.04f, 0.60f); glutSolidCube(1.0f); glPopMatrix();

    semEmissao();
    glPopMatrix();
}

// ===========================================================================
// MESA ALTA DE BALADA
// ===========================================================================
inline void desenharMesaAlta(float x, float z) {
    glPushMatrix();
    glTranslatef(x, 0.0f, z);

    setEmissao(0.32f, 0.30f, 0.30f);

    // Base cruzada de metal
    glColor3f(0.55f, 0.55f, 0.60f);
    glPushMatrix(); glTranslatef(0.0f, 0.05f, 0.0f); glScalef(1.6f, 0.08f, 0.30f); glutSolidCube(1.0f); glPopMatrix();
    glPushMatrix(); glTranslatef(0.0f, 0.05f, 0.0f); glScalef(0.30f, 0.08f, 1.6f); glutSolidCube(1.0f); glPopMatrix();

    // Coluna central
    glColor3f(0.60f, 0.60f, 0.65f);
    glPushMatrix(); glTranslatef(0.0f, 0.85f, 0.0f); glScalef(0.14f, 1.6f, 0.14f); glutSolidCube(1.0f); glPopMatrix();

    // Anel decorativo
    glColor3f(0.70f, 0.70f, 0.75f);
    glPushMatrix(); glTranslatef(0.0f, 0.85f, 0.0f); glScalef(0.24f, 0.10f, 0.24f); glutSolidCube(1.0f); glPopMatrix();

    // Tampo acrílico branco fosco
    glColor3f(0.90f, 0.88f, 0.88f);
    glPushMatrix(); glTranslatef(0.0f, 1.66f, 0.0f); glScalef(2.20f, 0.10f, 2.20f); glutSolidCube(1.0f); glPopMatrix();
    // borda do tampo
    glColor3f(0.78f, 0.76f, 0.76f);
    glPushMatrix(); glTranslatef(0.0f, 1.60f, 0.0f); glScalef(2.30f, 0.04f, 2.30f); glutSolidCube(1.0f); glPopMatrix();

    // LED embaixo do tampo
    glDisable(GL_LIGHTING);
    setEmissao(0.85f, 0.05f, 0.85f);
    glColor3f(0.9f, 0.1f, 0.9f);
    glPushMatrix(); glTranslatef(0.0f, 1.56f, 0.0f); glScalef(1.80f, 0.05f, 1.80f); glutSolidCube(1.0f); glPopMatrix();
    glEnable(GL_LIGHTING);

    semEmissao();
    glPopMatrix();

    float dist = 1.65f;
    desenharBanquinho(x + dist, z);
    desenharBanquinho(x - dist, z);
    desenharBanquinho(x, z + dist);
    desenharBanquinho(x, z - dist);
}

// ===========================================================================
// GARRAFA DE BEBIDA
// ===========================================================================
inline void desenharGarrafa(float x, float y, float z, float r, float g, float b) {
    glPushMatrix();
    glTranslatef(x, y, z);

    setEmissao(r * 0.15f, g * 0.15f, b * 0.15f);
    // corpo
    glColor3f(r, g, b);
    glPushMatrix(); glTranslatef(0.0f, 0.18f, 0.0f); glScalef(0.13f, 0.36f, 0.13f); glutSolidCube(1.0f); glPopMatrix();
    // gargalo
    glColor3f(r * 0.7f, g * 0.7f, b * 0.7f);
    glPushMatrix(); glTranslatef(0.0f, 0.42f, 0.0f); glScalef(0.06f, 0.18f, 0.06f); glutSolidCube(1.0f); glPopMatrix();
    // tampa dourada
    glColor3f(0.8f, 0.8f, 0.1f);
    glPushMatrix(); glTranslatef(0.0f, 0.52f, 0.0f); glScalef(0.08f, 0.04f, 0.08f); glutSolidCube(1.0f); glPopMatrix();

    semEmissao();
    glPopMatrix();
}

// ===========================================================================
// BANCADA DO BAR
// ===========================================================================
inline void desenharBar(float r, float g, float b) {
    float bX       =  97.9f;
    float bZ0      = -25.0f;
    float bZ1      =  25.0f;
    float bComp    = bZ1 - bZ0;
    float bCentroZ = (bZ0 + bZ1) * 0.5f;

    glPushMatrix();
    glTranslatef(bX, 0.0f, bCentroZ);

    // -- Corpo da bancada: cinza neutro que reage à iluminação da cena --
    semEmissao();
    glColor3f(0.40f, 0.40f, 0.43f);
    glPushMatrix(); glTranslatef(0.0f, 1.15f, 0.0f); glScalef(4.2f, 2.3f, bComp); glutSolidCube(1.0f); glPopMatrix();

    // -- Tampo de pedra: cinza claro, reage ao beat via GL_LIGHT0 --
    glColor3f(0.50f, 0.50f, 0.54f);
    glPushMatrix(); glTranslatef(-0.25f, 2.38f, 0.0f); glScalef(4.8f, 0.18f, bComp + 0.4f); glutSolidCube(1.0f); glPopMatrix();

    // -- Friso neon: pulsa na cor da batida --
    glDisable(GL_LIGHTING);
    setEmissao(r * 0.9f, g * 0.5f, b * 0.9f);
    glColor3f(r, g * 0.4f, b);
    glPushMatrix(); glTranslatef(-2.72f, 2.28f, 0.0f); glScalef(0.08f, 0.14f, bComp + 0.4f); glutSolidCube(1.0f); glPopMatrix();
    glEnable(GL_LIGHTING);
    semEmissao();

    // -- Rodapé metálico --
    glColor3f(0.40f, 0.38f, 0.42f);
    glPushMatrix(); glTranslatef(0.0f, 0.08f, 0.0f); glScalef(4.0f, 0.16f, bComp); glutSolidCube(1.0f); glPopMatrix();

    semEmissao();

    // -- Parede de suporte das prateleiras --
    setEmissao(0.04f, 0.04f, 0.05f);
    glColor3f(0.22f, 0.22f, 0.25f);
    glPushMatrix(); glTranslatef(1.8f, 5.5f, 0.0f); glScalef(0.4f, 9.0f, bComp - 2.0f); glutSolidCube(1.0f); glPopMatrix();

    // -- 3 prateleiras horizontais --
    glColor3f(0.38f, 0.38f, 0.42f);
    float altPrat[] = {3.0f, 5.0f, 7.0f};
    for (int i = 0; i < 3; ++i) {
        glPushMatrix(); glTranslatef(1.5f, altPrat[i], 0.0f); glScalef(0.8f, 0.12f, bComp - 2.5f); glutSolidCube(1.0f); glPopMatrix();
    }
    semEmissao();

    // -- Garrafas: prateleira 0 --
    desenharGarrafa(1.5f, 3.06f, -16.0f, 0.55f, 0.35f, 0.10f); // whisky
    desenharGarrafa(1.5f, 3.06f, -10.0f, 0.90f, 0.92f, 0.95f); // vodka
    desenharGarrafa(1.5f, 3.06f,  -4.0f, 0.20f, 0.55f, 0.20f); // gin
    desenharGarrafa(1.5f, 3.06f,   2.0f, 0.80f, 0.20f, 0.10f); // licor vermelho
    desenharGarrafa(1.5f, 3.06f,   8.0f, 0.60f, 0.40f, 0.10f); // rum
    desenharGarrafa(1.5f, 3.06f,  14.0f, 0.10f, 0.20f, 0.60f); // blue curacao
    // prateleira 1
    desenharGarrafa(1.5f, 5.06f, -14.0f, 0.70f, 0.30f, 0.05f);
    desenharGarrafa(1.5f, 5.06f,  -6.0f, 0.85f, 0.85f, 0.90f);
    desenharGarrafa(1.5f, 5.06f,   2.0f, 0.15f, 0.50f, 0.15f);
    desenharGarrafa(1.5f, 5.06f,  10.0f, 0.75f, 0.15f, 0.08f);
    // prateleira 2
    desenharGarrafa(1.5f, 7.06f, -10.0f, 0.50f, 0.30f, 0.08f);
    desenharGarrafa(1.5f, 7.06f,   0.0f, 0.88f, 0.88f, 0.92f);
    desenharGarrafa(1.5f, 7.06f,  10.0f, 0.18f, 0.48f, 0.18f);

    glPopMatrix();

    // -- Banquinhos do lado de fora da bancada --
    float passo = 6.5f;
    for (float bz = bZ0 + 3.0f; bz <= bZ1 - 3.0f; bz += passo) {
        desenharBanquinho(bX - 5.5f, bz);
    }
}

// ===========================================================================
// PALCO COM CORTINA DETALHADA
// ===========================================================================
inline void desenharPalco() {
    float largura      = 50.0f;
    float profundidade = 18.0f;
    float alturaPalco  =  2.0f;
    float margem       =  0.2f;
    float centroZ = -100.0f + margem + (profundidade / 2.0f);

    glPushMatrix();

    // ---- BASE DO PALCO ----
    setEmissao(0.02f, 0.02f, 0.03f);
    glColor3f(0.06f, 0.06f, 0.08f);
    glPushMatrix();
        glTranslatef(0.0f, alturaPalco / 2.0f, centroZ);
        glScalef(largura, alturaPalco, profundidade);
        glutSolidCube(1.0f);
    glPopMatrix();

    // Degraus laterais
    glColor3f(0.10f, 0.10f, 0.12f);
    glPushMatrix(); glTranslatef(-27.0f, 0.5f, centroZ + 2.0f); glScalef(4.0f, 1.0f, 6.0f); glutSolidCube(1.0f); glPopMatrix();
    glPushMatrix(); glTranslatef( 27.0f, 0.5f, centroZ + 2.0f); glScalef(4.0f, 1.0f, 6.0f); glutSolidCube(1.0f); glPopMatrix();

    // LED na borda frontal do palco
    glDisable(GL_LIGHTING);
    setEmissao(0.9f, 0.05f, 0.5f);
    glColor3f(1.0f, 0.05f, 0.6f);
    glPushMatrix();
        glTranslatef(0.0f, alturaPalco + 0.05f, centroZ + profundidade * 0.5f - 0.1f);
        glScalef(largura, 0.12f, 0.18f);
        glutSolidCube(1.0f);
    glPopMatrix();
    glEnable(GL_LIGHTING);
    semEmissao();

    // ---- CORTINA DE VELUDO ----
    float alturaCortina = 16.0f;
    float zCortina = -100.0f + margem + 0.6f; // colada na parede do fundo

    // Forro liso atrás das dobras
    glColor3f(0.12f, 0.0f, 0.02f);
    setEmissao(0.06f, 0.0f, 0.01f);
    glPushMatrix();
        glTranslatef(0.0f, alturaPalco + alturaCortina * 0.5f, zCortina - 0.3f);
        glScalef(largura + 1.0f, alturaCortina, 0.4f);
        glutSolidCube(1.0f);
    glPopMatrix();

    // Dobras (32 seções com 3 profundidades diferentes para textura orgânica)
    int numDobras = 32;
    float larguraDobra = largura / (float)numDobras;
    float offsets[3] = { 0.55f, -0.20f, 0.30f };

    for (int i = 0; i < numDobras; ++i) {
        float posX   = (-largura / 2.0f) + (larguraDobra / 2.0f) + (i * larguraDobra);
        float offsetZ = offsets[i % 3];
        float brilho  = (offsetZ > 0.0f) ? 0.42f : 0.30f;

        glColor3f(brilho, 0.0f, brilho * 0.15f);
        setEmissao(brilho * 0.35f, 0.0f, 0.02f);

        glPushMatrix();
            glTranslatef(posX, alturaPalco + (alturaCortina / 2.0f), zCortina + offsetZ);
            glScalef(larguraDobra + 0.6f, alturaCortina, 1.2f);
            glutSolidCube(1.0f);
        glPopMatrix();
    }

    // Franjas douradas na base da cortina
    glColor3f(0.65f, 0.50f, 0.10f);
    setEmissao(0.12f, 0.09f, 0.01f);
    glPushMatrix();
        glTranslatef(0.0f, alturaPalco + 0.25f, zCortina + 0.1f);
        glScalef(largura + 0.5f, 0.5f, 1.0f);
        glutSolidCube(1.0f);
    glPopMatrix();

    // ---- BANDÔ / SANEFA ----
    glColor3f(0.35f, 0.0f, 0.06f);
    setEmissao(0.12f, 0.0f, 0.02f);
    glPushMatrix();
        glTranslatef(0.0f, alturaPalco + alturaCortina - 0.5f, zCortina + 0.5f);
        glScalef(largura + 1.5f, 3.2f, 2.0f);
        glutSolidCube(1.0f);
    glPopMatrix();
    // bordão dourado
    glColor3f(0.70f, 0.55f, 0.12f);
    setEmissao(0.14f, 0.10f, 0.01f);
    glPushMatrix();
        glTranslatef(0.0f, alturaPalco + alturaCortina - 1.95f, zCortina + 1.3f);
        glScalef(largura + 1.5f, 0.3f, 0.3f);
        glutSolidCube(1.0f);
    glPopMatrix();
    // franjas da sanefa
    glColor3f(0.60f, 0.45f, 0.08f);
    glPushMatrix();
        glTranslatef(0.0f, alturaPalco + alturaCortina - 2.35f, zCortina + 1.2f);
        glScalef(largura + 1.5f, 0.6f, 0.5f);
        glutSolidCube(1.0f);
    glPopMatrix();

    // ---- CAIXAS DE SOM NAS LATERAIS ----
    setEmissao(0.04f, 0.04f, 0.04f);
    glColor3f(0.05f, 0.05f, 0.05f);
    float posXCaixa[] = {-22.0f, 22.0f};
    for (int i = 0; i < 2; ++i) {
        glPushMatrix(); glTranslatef(posXCaixa[i], alturaPalco + 3.5f, centroZ + 6.0f); glScalef(3.5f, 5.0f, 3.5f); glutSolidCube(1.0f); glPopMatrix();
        glPushMatrix(); glTranslatef(posXCaixa[i], alturaPalco + 0.9f, centroZ + 6.0f); glScalef(3.0f, 1.8f, 3.0f); glutSolidCube(1.0f); glPopMatrix();
        glDisable(GL_LIGHTING);
        setEmissao(0.05f, 0.05f, 0.05f);
        glColor3f(0.20f, 0.18f, 0.18f);
        glPushMatrix(); glTranslatef(posXCaixa[i], alturaPalco + 3.5f, centroZ + 7.85f); glScalef(1.8f, 1.8f, 0.3f); glutSolidCube(1.0f); glPopMatrix();
        glEnable(GL_LIGHTING);
    }

    semEmissao();
    glPopMatrix();
}

// ===========================================================================
// CHÃO QUADRICULADO
// ===========================================================================
inline void desenharChaoQuadriculado(float rLuz, float gLuz, float bLuz) {
    (void)rLuz; (void)gLuz; (void)bLuz;

    // Emissive fornece a cor base cinza (não é afetado pela intensidade da cena);
    // diffuse pequeno permite reação sutil ao GL_LIGHT0 e às PointLights das balas.
    glColorMaterial(GL_FRONT_AND_BACK, GL_EMISSION);
    GLfloat matDiff[] = { 0.07f, 0.07f, 0.08f, 1.0f };
    GLfloat matAmb[]  = { 0.04f, 0.04f, 0.05f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matDiff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, matAmb);

    glPushMatrix();
    glTranslatef(0.0f, -0.05f, 0.0f);
    float tamanhoTile = 2.0f;
    float limite      = 100.0f;

    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    for (float x = -limite; x < limite; x += tamanhoTile) {
        for (float z = -limite; z < limite; z += tamanhoTile) {
            int gridX = (int)((x + limite) / tamanhoTile);
            int gridZ = (int)((z + limite) / tamanhoTile);
            if ((gridX + gridZ) % 2 == 0)
                glColor3f(0.14f, 0.14f, 0.16f);  // tile claro: emissive base
            else
                glColor3f(0.04f, 0.04f, 0.05f);  // tile escuro: emissive base
            glVertex3f(x,               0.0f, z);
            glVertex3f(x + tamanhoTile, 0.0f, z);
            glVertex3f(x + tamanhoTile, 0.0f, z + tamanhoTile);
            glVertex3f(x,               0.0f, z + tamanhoTile);
        }
    }
    glEnd();
    glPopMatrix();

    // Restaura GL_COLOR_MATERIAL para AMBIENT_AND_DIFFUSE (padrão do resto do mapa)
    semEmissao();
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
}

// ===========================================================================
// PAREDES
// ===========================================================================
inline void desenharParedes(float rLuz, float gLuz, float bLuz) {
    float limite       = 100.0f;
    float alturaParede =  18.0f;
    float espessura    =   2.0f;
    float tamanhoTotal = limite * 2.0f;

    setEmissao(0.05f, 0.05f, 0.05f);

    glPushMatrix();
        glTranslatef(0.0f, alturaParede / 2.0f, -limite - espessura / 2.0f);
        glColor3f(0.08f, 0.08f, 0.10f);
        glScalef(tamanhoTotal + espessura, alturaParede, espessura);
        glutSolidCube(1.0f);
    glPopMatrix();

    glPushMatrix();
        glTranslatef(limite + espessura / 2.0f, alturaParede / 2.0f, 0.0f);
        glColor3f(0.08f, 0.08f, 0.10f);
        glScalef(espessura, alturaParede, tamanhoTotal);
        glutSolidCube(1.0f);
    glPopMatrix();

    glColor3f(0.05f, 0.05f, 0.05f);
    glPushMatrix(); glTranslatef( 80.0f, 14.0f, -limite - 0.5f); glScalef(8.0f, 6.0f, 4.0f); glutSolidCube(1.0f); glPopMatrix();
    glPushMatrix(); glTranslatef(-80.0f, 14.0f, -limite - 0.5f); glScalef(8.0f, 6.0f, 4.0f); glutSolidCube(1.0f); glPopMatrix();

    glColor3f(0.02f, 0.02f, 0.02f);
    glPushMatrix(); glTranslatef(-35.0f, 4.5f, -limite - 0.9f); glScalef(6.0f, 9.0f, 0.5f); glutSolidCube(1.0f); glPopMatrix();
    glPushMatrix(); glTranslatef(-20.0f, 4.5f, -limite - 0.9f); glScalef(6.0f, 9.0f, 0.5f); glutSolidCube(1.0f); glPopMatrix();

    glDisable(GL_LIGHTING);

    setEmissao(0.1f, 0.8f, 0.8f);
    glPushMatrix(); glTranslatef(-35.0f, 11.0f, -limite - 0.8f); glColor3f(0.1f, 0.8f, 0.8f); glScalef(4.0f, 2.0f, 0.5f); glutSolidCube(1.0f); glPopMatrix();

    setEmissao(0.8f, 0.1f, 0.8f);
    glPushMatrix(); glTranslatef(-20.0f, 11.0f, -limite - 0.8f); glColor3f(0.8f, 0.1f, 0.8f); glScalef(4.0f, 2.0f, 0.5f); glutSolidCube(1.0f); glPopMatrix();

    GLfloat emissaoDinamica[] = { rLuz * 0.9f, gLuz * 0.9f, bLuz * 0.9f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emissaoDinamica);
    glColor3f(rLuz, gLuz, bLuz);
    for (float posZ = -40.0f; posZ <= 40.0f; posZ += 40.0f) {
        glPushMatrix(); glTranslatef(limite + 0.8f, 8.0f, posZ); glScalef(0.5f, 5.0f, 12.0f); glutSolidCube(1.0f); glPopMatrix();
    }

    GLfloat emissaoNeon[] = { rLuz * 0.8f, gLuz * 0.8f, bLuz * 0.8f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emissaoNeon);
    float grossuraNeon   = 0.4f;
    float posNeon        = limite;
    float tamanhoInterno = tamanhoTotal - grossuraNeon;
    glPushMatrix(); glTranslatef( 0.0f, 0.5f, -posNeon); glScalef(tamanhoTotal, 1.0f, grossuraNeon);  glutSolidCube(1.0f); glPopMatrix();
    glPushMatrix(); glTranslatef( 0.0f, 0.5f,  posNeon); glScalef(tamanhoTotal, 1.0f, grossuraNeon);  glutSolidCube(1.0f); glPopMatrix();
    glPushMatrix(); glTranslatef( posNeon, 0.5f, 0.0f);  glScalef(grossuraNeon, 1.0f, tamanhoInterno); glutSolidCube(1.0f); glPopMatrix();
    glPushMatrix(); glTranslatef(-posNeon, 0.5f, 0.0f);  glScalef(grossuraNeon, 1.0f, tamanhoInterno); glutSolidCube(1.0f); glPopMatrix();

    glEnable(GL_LIGHTING);
    semEmissao();
}

// ===========================================================================
// MAPA PRINCIPAL
// ===========================================================================
inline void desenharMapaBalada() {
    float tempo = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    int batida  = (int)(tempo * 3.0f) % 3;

    float r = 0.0f, g = 0.0f, b = 0.0f;
    if      (batida == 0) { r = 1.0f; b = 0.1f; }
    else if (batida == 1) { r = 0.1f; b = 1.0f; }
    else                  { r = 0.8f; b = 1.0f; }

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    GLfloat luzAmbiente[] = { r * 0.2f, g * 0.2f, b * 0.2f, 1.0f };
    GLfloat luzDifusa[]   = { r * 0.9f, g * 0.9f, b * 0.9f, 1.0f };
    GLfloat luzDirecao[]  = { 0.0f, 1.0f, 0.0f, 0.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT,  luzAmbiente);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  luzDifusa);
    glLightfv(GL_LIGHT0, GL_POSITION, luzDirecao);

    desenharChaoQuadriculado(r, g, b);

    desenharParedes(r, g, b);
    desenharPalco();
    desenharBar(r, g, b);

    for (int i = 0; i < NUM_OBSTACULOS; ++i) {
        desenharMesaAlta(OBSTACULOS[i].x, OBSTACULOS[i].z);
    }
}

// ===========================================================================
// COLISÕES (sem alteração na lógica)
// ===========================================================================
inline void resolverColisaoRetangular(Vetor3D& pos, float raio,
                                       float minX, float maxX,
                                       float minZ, float maxZ) {
    if (pos.x + raio > minX && pos.x - raio < maxX &&
        pos.z + raio > minZ && pos.z - raio < maxZ) {
        float distEsq    = (pos.x + raio) - minX;
        float distDir    = maxX - (pos.x - raio);
        float distFrente = maxZ - (pos.z - raio);
        if (distFrente < distEsq && distFrente < distDir)
            pos.z = maxZ + raio;
        else if (distEsq < distDir)
            pos.x = minX - raio;
        else
            pos.x = maxX + raio;
    }
}

inline void processarColisoesCenario(EstadoDoJogo& jogo) {
    float limiteMapa = 98.0f;

    if (jogo.protagonista.posicao.x >  limiteMapa) jogo.protagonista.posicao.x =  limiteMapa;
    if (jogo.protagonista.posicao.x < -limiteMapa) jogo.protagonista.posicao.x = -limiteMapa;
    if (jogo.protagonista.posicao.z >  limiteMapa) jogo.protagonista.posicao.z =  limiteMapa;
    if (jogo.protagonista.posicao.z < -limiteMapa) jogo.protagonista.posicao.z = -limiteMapa;

    for (size_t j = 0; j < jogo.horda.size(); ++j) {
        if (!jogo.horda[j].vivo) continue;
        if (jogo.horda[j].posicao.x >  limiteMapa) jogo.horda[j].posicao.x =  limiteMapa;
        if (jogo.horda[j].posicao.x < -limiteMapa) jogo.horda[j].posicao.x = -limiteMapa;
        if (jogo.horda[j].posicao.z >  limiteMapa) jogo.horda[j].posicao.z =  limiteMapa;
        if (jogo.horda[j].posicao.z < -limiteMapa) jogo.horda[j].posicao.z = -limiteMapa;
    }

    float pMinX = -25.0f, pMaxX = 25.0f, pMinZ = -100.0f, pMaxZ = -81.8f;
    resolverColisaoRetangular(jogo.protagonista.posicao, jogo.protagonista.raioColisao, pMinX, pMaxX, pMinZ, pMaxZ);
    for (size_t j = 0; j < jogo.horda.size(); ++j) {
        if (!jogo.horda[j].vivo) continue;
        resolverColisaoRetangular(jogo.horda[j].posicao, jogo.horda[j].raioColisao, pMinX, pMaxX, pMinZ, pMaxZ);
    }

    // Bar (parede direita): bX=97.9, corpo de 4.2 de largura → front face em ~95.8
    float barMinX = 95.5f, barMaxX = 102.0f, barMinZ = -25.5f, barMaxZ = 25.5f;
    resolverColisaoRetangular(jogo.protagonista.posicao, jogo.protagonista.raioColisao, barMinX, barMaxX, barMinZ, barMaxZ);
    for (size_t j = 0; j < jogo.horda.size(); ++j) {
        if (!jogo.horda[j].vivo) continue;
        resolverColisaoRetangular(jogo.horda[j].posicao, jogo.horda[j].raioColisao, barMinX, barMaxX, barMinZ, barMaxZ);
    }

    for (int i = 0; i < NUM_OBSTACULOS; ++i) {
        float dx = jogo.protagonista.posicao.x - OBSTACULOS[i].x;
        float dz = jogo.protagonista.posicao.z - OBSTACULOS[i].z;
        float distancia = std::sqrt(dx * dx + dz * dz);
        float raioCombinado = jogo.protagonista.raioColisao + OBSTACULOS[i].raio;
        if (distancia < raioCombinado) {
            if (distancia == 0.0f) { dx = 0.1f; distancia = 0.1f; }
            float penetracao = raioCombinado - distancia;
            jogo.protagonista.posicao.x += (dx / distancia) * penetracao;
            jogo.protagonista.posicao.z += (dz / distancia) * penetracao;
        }
    }

    for (size_t j = 0; j < jogo.horda.size(); ++j) {
        if (!jogo.horda[j].vivo) continue;
        for (int i = 0; i < NUM_OBSTACULOS; ++i) {
            float dx = jogo.horda[j].posicao.x - OBSTACULOS[i].x;
            float dz = jogo.horda[j].posicao.z - OBSTACULOS[i].z;
            float distancia = std::sqrt(dx * dx + dz * dz);
            float raioCombinado = jogo.horda[j].raioColisao + OBSTACULOS[i].raio;
            if (distancia < raioCombinado) {
                if (distancia == 0.0f) { dx = 0.1f; distancia = 0.1f; }
                float penetracao = raioCombinado - distancia;
                jogo.horda[j].posicao.x += (dx / distancia) * penetracao;
                jogo.horda[j].posicao.z += (dz / distancia) * penetracao;
            }
        }
    }
}

#endif // MAP_H