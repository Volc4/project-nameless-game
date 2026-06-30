#ifndef DRAWPRIMITIVES3D_H
#define DRAWPRIMITIVES3D_H

// =============================================================================
//  PRIMITIVAS DE DESENHO  — exigidas por SkillRender.h
//  (declaradas extern lá; definidas aqui)
// =============================================================================

// ---------------------------------------------------------------------------
// desenharBloco3D — cubo colorido nas 4 faces laterais + topo.
//   cx,cy_base,cz : centro na base; lx,lz,altura : dimensões.
//   Ordem de cores: Topo, Frente (Z+), Trás (Z-), Direita (X+), Esquerda (X-).
// ---------------------------------------------------------------------------
void desenharBloco3D(float cx, float cy_base, float cz,
                     float lx, float lz, float altura,
                     float rT, float gT, float bT,
                     float rF, float gF, float bF,
                     float rB, float gB, float bB,
                     float rR, float gR, float bR,
                     float rL, float gL, float bL)
{
    float x0 = cx - lx * 0.5f, x1 = cx + lx * 0.5f;
    float y0 = cy_base,         y1 = cy_base + altura;
    float z0 = cz - lz * 0.5f, z1 = cz + lz * 0.5f;

    glBegin(GL_QUADS);
        // Topo
        glNormal3f( 0.0f,  1.0f,  0.0f);
        glColor3f(rT, gT, bT);
        glVertex3f(x0, y1, z0); glVertex3f(x1, y1, z0);
        glVertex3f(x1, y1, z1); glVertex3f(x0, y1, z1);
        // Frente (Z+)
        glNormal3f( 0.0f,  0.0f,  1.0f);
        glColor3f(rF, gF, bF);
        glVertex3f(x0, y0, z1); glVertex3f(x1, y0, z1);
        glVertex3f(x1, y1, z1); glVertex3f(x0, y1, z1);
        // Trás (Z-)
        glNormal3f( 0.0f,  0.0f, -1.0f);
        glColor3f(rB, gB, bB);
        glVertex3f(x1, y0, z0); glVertex3f(x0, y0, z0);
        glVertex3f(x0, y1, z0); glVertex3f(x1, y1, z0);
        // Direita (X+)
        glNormal3f( 1.0f,  0.0f,  0.0f);
        glColor3f(rR, gR, bR);
        glVertex3f(x1, y0, z1); glVertex3f(x1, y0, z0);
        glVertex3f(x1, y1, z0); glVertex3f(x1, y1, z1);
        // Esquerda (X-)
        glNormal3f(-1.0f,  0.0f,  0.0f);
        glColor3f(rL, gL, bL);
        glVertex3f(x0, y0, z0); glVertex3f(x0, y0, z1);
        glVertex3f(x0, y1, z1); glVertex3f(x0, y1, z0);
    glEnd();
}

// ---------------------------------------------------------------------------
// desenharLinha3D — segmento de largura variável no plano XZ (Beam/Wall/Wave).
// ---------------------------------------------------------------------------
void desenharLinha3D(float x0, float y, float z0,
                     float x1, float z1,
                     float largura,
                     float r, float g, float b)
{
    // Perpendicular 2D ao segmento
    float dx = x1 - x0, dz = z1 - z0;
    float len = std::sqrt(dx*dx + dz*dz);
    if (len < 0.0001f) return;
    float px = -dz / len * largura * 0.5f;
    float pz =  dx / len * largura * 0.5f;

    glColor3f(r, g, b);
    glBegin(GL_QUADS);
        glVertex3f(x0 + px, y, z0 + pz);
        glVertex3f(x1 + px, y, z1 + pz);
        glVertex3f(x1 - px, y, z1 - pz);
        glVertex3f(x0 - px, y, z0 - pz);
    glEnd();
}

// ---------------------------------------------------------------------------
// desenharCirculo3D — disco no plano XZ (Area/Aura/Explosion).
// ---------------------------------------------------------------------------
void desenharCirculo3D(float cx, float y, float cz,
                       float raio,
                       float r, float g, float b)
{
    const int SEG = 24;
    glColor3f(r, g, b);
    glBegin(GL_TRIANGLE_FAN);
        glVertex3f(cx, y, cz);
        for (int i = 0; i <= SEG; ++i) {
            float ang = (float)i / SEG * 2.0f * 3.14159265f;
            glVertex3f(cx + std::cos(ang) * raio, y, cz + std::sin(ang) * raio);
        }
    glEnd();
}

// ---------------------------------------------------------------------------
// desenharArco3D — setor de anel no plano XZ (Arc/Meia-Lua).
// ---------------------------------------------------------------------------
void desenharArco3D(float cx, float y, float cz,
                    float raioInterno, float raioExterno,
                    float anguloCentral, float meiaAbertura,
                    float r, float g, float b)
{
    const int SEG = 20;
    float angIni = anguloCentral - meiaAbertura;
    float angFim = anguloCentral + meiaAbertura;

    glColor3f(r, g, b);
    glBegin(GL_TRIANGLE_STRIP);
        for (int i = 0; i <= SEG; ++i) {
            float ang = angIni + (angFim - angIni) * (float)i / (float)SEG;
            float cosA = std::cos(ang), sinA = std::sin(ang);
            glVertex3f(cx + cosA * raioInterno, y, cz + sinA * raioInterno);
            glVertex3f(cx + cosA * raioExterno, y, cz + sinA * raioExterno);
        }
    glEnd();
}

// ---------------------------------------------------------------------------
// desenharMissilOrientado3D — paralelepípedo colorido orientado pela direção.
//   cx, cz   : centro do míssil no plano XZ
//   yaw      : atan2(direcao.z, direcao.x) — ângulo do eixo longo no plano XZ
//   cr,cg,cb : cor base do míssil
// ---------------------------------------------------------------------------
void desenharMissilOrientado3D(float cx, float cy, float cz, float yaw,
                                float cr, float cg, float cb)
{
    const float COMP = 0.55f;   // comprimento (eixo longo, orientado)
    const float LARG = 0.15f;   // seção transversal (largura e altura)

    glPushMatrix();
    glTranslatef(cx, cy, cz);
    // Rotação em Y: -yaw converte o ângulo do plano XZ para glRotatef (dir +X = yaw 0).
    glRotatef(-(yaw * (180.0f / 3.14159265f)), 0.0f, 1.0f, 0.0f);
    glScalef(COMP, LARG, LARG);

    // Cubo unitário centrado (−0.5 a +0.5 em cada eixo).
    // O eixo X local corresponde ao eixo longo do míssil após glScalef.
    float x0 = -0.5f, x1 = 0.5f;
    float y0 = -0.5f, y1 = 0.5f;
    float z0 = -0.5f, z1 = 0.5f;

    glBegin(GL_QUADS);
        glColor3f(cr,          cg,          cb);
        glVertex3f(x0, y1, z0); glVertex3f(x1, y1, z0);
        glVertex3f(x1, y1, z1); glVertex3f(x0, y1, z1);

        glColor3f(cr * 0.80f,  cg * 0.80f,  cb * 0.80f);
        glVertex3f(x0, y0, z1); glVertex3f(x1, y0, z1);
        glVertex3f(x1, y1, z1); glVertex3f(x0, y1, z1);

        glColor3f(cr * 0.55f,  cg * 0.55f,  cb * 0.55f);
        glVertex3f(x1, y0, z0); glVertex3f(x0, y0, z0);
        glVertex3f(x0, y1, z0); glVertex3f(x1, y1, z0);

        glColor3f(cr * 0.90f,  cg * 0.90f,  cb * 0.90f);
        glVertex3f(x1, y0, z1); glVertex3f(x1, y0, z0);
        glVertex3f(x1, y1, z0); glVertex3f(x1, y1, z1);

        glColor3f(cr * 0.65f,  cg * 0.65f,  cb * 0.65f);
        glVertex3f(x0, y0, z0); glVertex3f(x0, y0, z1);
        glVertex3f(x0, y1, z1); glVertex3f(x0, y1, z0);
    glEnd();

    glPopMatrix();
}

// ---------------------------------------------------------------------------
// desenharTrailBala3D — rastro luminoso atrás do projétil (blending aditivo).
//   Desenha N quads semi-transparentes ao longo de -direção, cada um menor e
//   mais transparente que o anterior. GL_ONE no destino: auto-accumula glow.
// ---------------------------------------------------------------------------
void desenharTrailBala3D(float cx, float cy, float cz,
                          float dx, float dz,
                          float cr, float cg, float cb)
{
    const int   STEPS     = 7;
    const float STEP_DIST = 0.20f;
    const float TAM_BASE  = 0.24f;
    // perpendicular ao movimento no plano XZ (para o billboard flat)
    float nx = -dz, nz = dx;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);   // aditivo: empilha glow
    glDepthMask(GL_FALSE);
    glDisable(GL_LIGHTING);

    glBegin(GL_QUADS);
    for (int i = 1; i <= STEPS; ++i) {
        float t     = (float)i / (float)STEPS;
        float alpha = (1.0f - t) * 0.55f;
        float h     = TAM_BASE * (1.0f - t * 0.55f);
        float px    = cx - dx * STEP_DIST * (float)i;
        float pz    = cz - dz * STEP_DIST * (float)i;
        glColor4f(cr, cg, cb, alpha);
        glVertex3f(px + nx * h, cy - h * 0.5f, pz + nz * h);
        glVertex3f(px - nx * h, cy - h * 0.5f, pz - nz * h);
        glVertex3f(px - nx * h, cy + h * 0.5f, pz + nz * h);
        glVertex3f(px + nx * h, cy + h * 0.5f, pz + nz * h);
    }
    glEnd();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

#endif // DRAWPRIMITIVES3D_H
