#ifndef RENDERCENA_H
#define RENDERCENA_H

// =============================================================================
//  RenderCena.h — Desenho 3D da cena (extraído de Main.cpp)
// =============================================================================
// =============================================================================
//  DESENHO DE CENA (OPENGL)
// =============================================================================

// Câmera isométrica seguindo o jogador
static void configurarCamera() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)g_winW / (double)g_winH, 0.5, 600.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    Vetor3D& p = g_jogo.protagonista.posicao;
    
    // Distância da câmera (offset)
    float dist = CAM_DIST; 
    
    // Para alinhar o Norte (Z-) com o topo da tela, 
    // a câmera deve estar posicionada no lado positivo do Z e olhar para o negativo.
    // Ajuste o ângulo de visão para uma perspectiva isométrica limpa (45 graus de inclinação)
    float eyeX = p.x;              // Câmera alinhada ao X do jogador
    float eyeZ = p.z + dist;       // Câmera recuada no Z
    float eyeY = dist * 0.8f;      // Altura da câmera
    
    gluLookAt(eyeX, eyeY, eyeZ,    // Posição da câmera
              p.x, 0.0f, p.z,      // Ponto focal (onde a câmera olha)
              0.0f, 1.0f, 0.0f);   // Vetor "UP" (para cima)
}

// Configura GL_LIGHT0 para personagens. Mantém EXATAMENTE os valores recebidos
// de cada chamador (não unificar os números — passá-los por parâmetro).
static void configurarLuzPersonagem(const GLfloat pos[4],
                                     const GLfloat amb[4],
                                     const GLfloat dif[4]) {
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  amb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  dif);
    glEnable(GL_LIGHT0);
}

// Jogador — modelo animado da Sofia (fallback: cubo branco-azulado)
static void desenharJogador() {
    Jogador& p = g_jogo.protagonista;
    if (!p.vivo) return;

    if (!g_sofiaCarregada) {
        float flash = (p.temporizadorIframe > 0.0f) ? 0.5f : 1.0f;
        desenharBloco3D(p.posicao.x, 0.0f, p.posicao.z,
                        0.8f, 0.8f, 1.2f,
                        0.9f*flash, 0.9f*flash, 1.0f*flash,
                        0.7f*flash, 0.7f*flash, 0.9f*flash,
                        0.5f*flash, 0.5f*flash, 0.7f*flash,
                        0.8f*flash, 0.8f*flash, 1.0f*flash,
                        0.6f*flash, 0.6f*flash, 0.8f*flash);
        return;
    }

    // Pisca durante i-frames
    if (p.temporizadorIframe > 0.0f) {
        int ciclo = (int)(p.temporizadorIframe * 10.0f);
        if (ciclo % 2 == 0) return;
    }

    glPushMatrix();
    
    // CORREÇÃO 1: Levanta a Sofia no eixo Y para não ficar enterrada
    glTranslatef(p.posicao.x, SOFIA_Y_OFFSET, p.posicao.z);

    float angGraus = g_jogo.stand.anguloMira * (180.0f / 3.14159265f);
    
    // CORREÇÃO 2: Sinal negativo em -angGraus conserta o espelhamento Cima/Baixo
    glRotatef(-angGraus + SOFIA_ROT_OFFSET, 0.0f, 1.0f, 0.0f);
    
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f); // Blender exporta Z-up; corrige para Y-up
    glScalef(SOFIA_ESCALA, SOFIA_ESCALA, SOFIA_ESCALA);

    // Iluminação mínima para o modelo texturizado
    glEnable(GL_LIGHTING);
    GLfloat lpos[4] = { 0.0f, 20.0f,  5.0f, 1.0f };
    GLfloat lamb[4] = { 0.5f,  0.5f,  0.5f, 1.0f };
    GLfloat ldif[4] = { 1.0f,  1.0f,  1.0f, 1.0f };
    configurarLuzPersonagem(lpos, lamb, ldif);

    g_sofia.renderizar();

    // Pistola — bone socket: herda a matriz global do osso da mão da Sofia.
    // Ainda dentro do glPushMatrix da Sofia, então as transforms dela já estão na pilha.
    if (g_pistolaCarregada && g_sofia.temMao()) {
        glPushMatrix();
        glm::mat4 socket = g_sofia.obterMatrizSocketMao();
        glMultMatrixf(glm::value_ptr(socket));
        glTranslatef(g_pistolaOffX, g_pistolaOffY, g_pistolaOffZ);
        glRotatef(g_pistolaRotX, 1.0f, 0.0f, 0.0f);
        glRotatef(g_pistolaRotY, 0.0f, 1.0f, 0.0f);
        glRotatef(g_pistolaRotZ, 0.0f, 0.0f, 1.0f);
        glScalef(g_pistolaEsc, g_pistolaEsc, g_pistolaEsc);
        g_pistola.renderizar();
        glPopMatrix();
    }

    glDisable(GL_LIGHTING);
    glPopMatrix();
}

// Stand — fantasma translúcido: teapot verde (tensão baixa) → vermelho (tensão alta).
// Bico segue o cursor. Ao Devorar, avança na direção do cursor e cresce.
static void desenharStand() {
    Entidade& s = g_jogo.stand;

    // Cor: verde → vermelho conforme tensão
    float maxT  = maxTensaoDoNivel(g_jogo.protagonista.upgrades.niveis[TENSAO_UP]);
    float ratio = (maxT > 0.0f) ? (s.tensaoAtual / maxT) : 0.0f;
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;
    float cr = ratio;
    float cg = 1.0f - ratio;

    // ---- Animação de Devorar ------------------------------------------------
    // progresso: 1.0 no início da janela → 0.0 no fim
    bool  devorarAtivo = (s.temporizadorDevorar > 0.0f);
    float progresso    = devorarAtivo
                         ? (s.temporizadorDevorar / DEVORAR_DURACAO_JANELA) : 0.0f;
    // Curva suave: 0 → 1 → 0 ao longo da janela (pico no meio)
    float arco = std::sin(progresso * 3.14159265f);

    // Lunge na direção da mira (bico vai "morder" o alvo)
    float lungeX = std::cos(s.anguloMira) * arco * 2.5f;
    float lungeZ = std::sin(s.anguloMira) * arco * 2.5f;

    // Escala e opacidade aumentam durante o Devorar
    float escala = 0.5f  + arco * 0.35f;
    float alpha  = 0.45f + arco * 0.40f;

    // Bob de levitação
    float floatY = 3.5f + std::sin(g_jogo.tempoSobrevivido * 2.0f) * 0.12f;

    // Bico segue o mouse: -angGraus aponta o spout (+X no teapot do GLUT) para
    // a direção do cursor. Se aparecer invertido, ajuste adicionando ±90° ou ±180°.
    float angGraus = s.anguloMira * (180.0f / 3.14159265f);

    // --- Halo no chão (aditivo, aparece antes do corpo) ----------------------
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);
    glDisable(GL_LIGHTING);
    {
        const int SEG = 28;
        float haloR = escala * 2.2f;
        float pulso  = 0.18f + std::sin(g_jogo.tempoSobrevivido * 4.0f) * 0.08f;
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(cr, cg, 0.05f, pulso * alpha);
        glVertex3f(s.posicao.x + lungeX, 0.02f, s.posicao.z + lungeZ);
        glColor4f(cr, cg, 0.05f, 0.0f);
        for (int i = 0; i <= SEG; ++i) {
            float a = (float)i / SEG * 2.0f * 3.14159265f;
            glVertex3f(s.posicao.x + lungeX + std::cos(a) * haloR,
                       0.02f,
                       s.posicao.z + lungeZ + std::sin(a) * haloR);
        }
        glEnd();
    }

    // --- Corpo fantasma: teapot translúcido + emissivo -----------------------
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    // Luz de fill para o teapot (mesma posição do LIGHT0 do mapa)
    GLfloat lpos[4] = { 0.0f, 20.0f,  5.0f, 1.0f };
    GLfloat lamb[4] = { 0.35f, 0.35f, 0.35f, 1.0f };
    GLfloat ldif[4] = { 1.0f,  1.0f,  1.0f, 1.0f };
    configurarLuzPersonagem(lpos, lamb, ldif);

    // Material emissivo: o fantasma "brilha" na cor da tensão
    GLfloat emissao[] = { cr * 0.55f, cg * 0.55f, 0.0f, 1.0f };
    GLfloat especular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emissao);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, especular);
    glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, 90.0f);

    glPushMatrix();
    glTranslatef(s.posicao.x + lungeX, floatY, s.posicao.z + lungeZ);
    glRotatef(-angGraus, 0.0f, 1.0f, 0.0f);
    glColor4f(cr, cg, 0.15f, alpha);
    glutSolidTeapot(escala);
    glPopMatrix();

    // --- Restaura estado GL --------------------------------------------------
    GLfloat emZero[]  = { 0.0f, 0.0f, 0.0f, 1.0f };
    GLfloat specZero[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emZero);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specZero);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_LIGHTING);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

// ---------------------------------------------------------------------------
// desenharBoss — renderiza o Boss com visual diferenciado:
//   • Bloco principal roxo escuro ~5x maior que zumbi normal
//   • 4 espigões dourados no topo (coroa)
//   • Aura pulsante no chão
//   • Efeito de "aparição" durante tempoEntradaBoss (pulso de escala)
// ---------------------------------------------------------------------------
static void desenharBoss(const Zumbi& z) {


    // Efeito de entrada: escala pulsa nos primeiros BOSS_ENTRADA_DURACAO segundos
    float escalaEntrada = 1.0f;
    if (g_jogo.tempoEntradaBoss > 0.0f) {
        float t    = 1.0f - g_jogo.tempoEntradaBoss / BOSS_ENTRADA_DURACAO;
        float arco = std::sin(t * 3.14159265f * 5.0f);
        escalaEntrada = 1.0f + arco * 0.35f;
    }

    // Aura pulsante no chão (mantida mesmo com o modelo 3D)
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    float pulso = 0.25f + std::sin(g_jogo.tempoSobrevivido * 5.0f) * 0.12f;
    glColor4f(0.55f, 0.0f, 1.0f, pulso);
    desenharCirculo3D(z.posicao.x, 0.02f, z.posicao.z, BOSS_RAIO_COLISAO * 1.8f,
                      0.55f, 0.0f, 1.0f);
    glDisable(GL_BLEND);

    if (!g_cabecaCarregada) {
        // Fallback: cubo roxo original para quando o modelo não carregou
        float s = z.raioColisao * 1.8f * escalaEntrada;
        float cR = 0.45f, cG = 0.0f, cB = 0.85f;
        float alturaCorpo = s * 1.8f;
        desenharBloco3D(z.posicao.x, 0.0f, z.posicao.z,
                        s, s, alturaCorpo,
                        cR,       cG, cB,
                        cR*0.80f, cG, cB*0.80f,
                        cR*0.60f, cG, cB*0.60f,
                        cR*0.90f, cG, cB*0.90f,
                        cR*0.70f, cG, cB*0.70f);
        return;
    }

    // Direção boss → jogador para orientar o modelo de frente ao alvo
    Jogador& jp = g_jogo.protagonista;
    float dx = jp.posicao.x - z.posicao.x;
    float dz = jp.posicao.z - z.posicao.z;
    float angGraus = std::atan2(dz, dx) * (180.0f / 3.14159265f);
    float esc = BOSS_ESCALA * escalaEntrada;

    glPushMatrix();
    glTranslatef(z.posicao.x, BOSS_MODELO_Y, z.posicao.z);
    glRotatef(-angGraus + BOSS_ROT_OFFSET, 0.0f, 1.0f, 0.0f);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f); // Z-up do modelo → Y-up do OpenGL
    glScalef(esc, esc, esc);

    glEnable(GL_LIGHTING);
    GLfloat lpos[4] = { 0.0f, 20.0f, 5.0f, 1.0f };
    GLfloat lamb[4] = { 0.5f,  0.5f,  0.5f, 1.0f };
    GLfloat ldif[4] = { 1.0f,  1.0f,  1.0f, 1.0f };
    configurarLuzPersonagem(lpos, lamb, ldif);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    g_cabeca.renderizar();

    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_LIGHTING);
    glPopMatrix();
}

// Visual por tipo de zumbi: escala relativa e tint multiplicativo (GL_MODULATE)
struct ZumbiVisual { float escala; float tR, tG, tB; };
static ZumbiVisual visualZumbi(TipoZumbi tipo) {
    switch (tipo) {
        case NORMAL:    return { 1.00f, 1.00f, 1.00f, 1.00f }; // original
        case RAPIDO:    return { 0.82f, 0.25f, 1.00f, 0.25f }; // verde vivo, menor
        case TANK:      return { 1.50f, 0.25f, 0.45f, 1.00f }; // azul forte, grande
        case ATIRADOR:  return { 0.95f, 1.00f, 1.00f, 0.10f }; // amarelo intenso
        case EXPLOSIVO: return { 1.15f, 1.00f, 0.25f, 0.15f }; // laranja-vermelho
        default:        return { 1.00f, 1.00f, 1.00f, 1.00f };
    }
}

// Zumbis (e Boss, redirecionado para desenharBoss)
static void desenharZumbis() {
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    GLfloat lpos[4] = { 0.0f, 20.0f,  0.0f, 0.0f };
    GLfloat lamb[4] = { 0.5f,  0.5f,  0.5f, 1.0f };
    GLfloat ldif[4] = { 0.9f,  0.9f,  0.9f, 1.0f };
    configurarLuzPersonagem(lpos, lamb, ldif);

    float tempoBase = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

    // Calcula CPU skinning UMA vez por frame — reutilizado para todos os zumbis.
    // Sem isso, N zumbis = N recalculações completas de bone matrices + skinning.
    if (g_zumbiCarregado)
        g_zumbi.atualizarPose(tempoBase, g_zumbiNomeAnim);

    // Distância máxima para renderizar o modelo 3D; além disso usa bloco simples (LOD)
    const float LOD_DIST_SQ = 70.0f * 70.0f;

    for (size_t i = 0; i < g_jogo.horda.size(); ++i) {
        const Zumbi& z = g_jogo.horda[i];
        if (!z.vivo) continue;

        if (z.ehBoss) {
            desenharBoss(z);
            continue;
        }

        float dx = g_jogo.protagonista.posicao.x - z.posicao.x;
        float dz = g_jogo.protagonista.posicao.z - z.posicao.z;
        float distSq = dx*dx + dz*dz;

        // LOD: bloco simples para zumbis longe da câmera
        if (!g_zumbiCarregado || distSq > LOD_DIST_SQ) {
            float cR, cG, cB;
            obterCorBaseZumbi(z.tipo, cR, cG, cB);
            float s = z.raioColisao * 1.4f;
            desenharBloco3D(z.posicao.x, 0.0f, z.posicao.z,
                            s, s, s * 1.4f,
                            cR, cG, cB,
                            cR*0.8f, cG*0.8f, cB*0.8f,
                            cR*0.6f, cG*0.6f, cB*0.6f,
                            cR*0.9f, cG*0.9f, cB*0.9f,
                            cR*0.7f, cG*0.7f, cB*0.7f);
            continue;
        }

        ZumbiVisual vis = visualZumbi(z.tipo);
        float esc = ZUMBI_ESCALA * vis.escala;
        float angGraus = std::atan2(dz, dx) * (180.0f / 3.14159265f);

        glPushMatrix();
        glTranslatef(z.posicao.x, ZUMBI_Y_OFFSET, z.posicao.z);
        glRotatef(-angGraus + ZUMBI_ROT_OFFSET, 0.0f, 1.0f, 0.0f);
        glScalef(esc, esc, esc);

        g_zumbi.definirTint(vis.tR, vis.tG, vis.tB);
        g_zumbi.renderizar(); // pose já calculada — só desenha
        glPopMatrix();
    }

    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_NORMALIZE);
    glDisable(GL_LIGHTING);
}

// Partículas
static void desenharParticulas() {
    glPointSize(4.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_POINTS);
    for (size_t i = 0; i < g_jogo.particulas.size(); ++i) {
        const Particula& p = g_jogo.particulas[i];
        if (!p.ativa) continue;
        glColor4f(p.corR, p.corG, p.corB, p.transparencia);
        glVertex3f(p.posicao.x, p.posicao.y, p.posicao.z);
    }
    glEnd();
    glDisable(GL_BLEND);
}

// Contorno do setor de Devorar — geometria idêntica à detecção (honesto com a hitbox)
static void desenharHitboxDevorar() {
    if (!g_jogo.protagonista.vivo) return;

    Entidade& stand  = g_jogo.stand;
    Jogador&  p      = g_jogo.protagonista;
    bool janelaAtiva = (stand.temporizadorDevorar > 0.0f);

    const int SEG = 16;
    float meiaAb  = DEVORAR_ANGULO * 0.5f;
    float angIni  = stand.anguloMira - meiaAb;
    float angFim  = stand.anguloMira + meiaAb;
    float cx = p.posicao.x, cz = p.posicao.z, y = 0.05f;

    if (!janelaAtiva) return;  // só desenha enquanto a janela está ativa

    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 0.10f, 0.05f);
    glLineWidth(2.5f);

    // GL_LINE_LOOP: centro → arco → fecha automaticamente (os dois raios + arco)
    glBegin(GL_LINE_LOOP);
        glVertex3f(cx, y, cz);
        for (int i = 0; i <= SEG; ++i) {
            float ang = angIni + (angFim - angIni) * (float)i / (float)SEG;
            glVertex3f(cx + std::cos(ang) * DEVORAR_ALCANCE, y,
                       cz + std::sin(ang) * DEVORAR_ALCANCE);
        }
    glEnd();

    glLineWidth(1.5f);  // restaura espessura padrão do jogo
}

// Projéteis do Atirador (círculo laranja pequeno no plano XZ)
static void desenharProjeteisZumbi() {
    glDisable(GL_LIGHTING);
    for (size_t i = 0; i < g_jogo.projeteisZumbi.size(); ++i) {
        const ProjetilZumbi& pz = g_jogo.projeteisZumbi[i];
        if (!pz.ativo) continue;
        if (pz.ehDoBoss) {
            // Projétil do Boss: roxo, maior
            desenharCirculo3D(pz.posicao.x, 0.15f, pz.posicao.z, 0.55f,
                              0.75f, 0.0f, 1.0f);
        } else {
            desenharCirculo3D(pz.posicao.x, 0.1f, pz.posicao.z, 0.3f,
                              1.0f, 0.55f, 0.0f);
        }
    }
}

// Skills (todos os slots)
static void desenharSkills() {
    for (int i = 0; i < g_skills.numSlots(); ++i) {
        const SlotSkill& sl = g_skills.slot(i);
        if (!sl.ativo) continue;
        for (size_t k = 0; k < sl.pool.size(); ++k)
            desenharSkill(sl.build, sl.pool[k]);
    }
}

// ---------------------------------------------------------------------------
// ativarMaterialGlow / desativarMaterialGlow
//   Ativa GL_EMISSION + GL_SPECULAR alto na cor da bala para que o projétil
//   brilhe independente da iluminação ambiente. GL_NORMALIZE evita distorção
//   de normais quando glScalef é usado (ex: missiles).
// ---------------------------------------------------------------------------
void ativarMaterialGlow(float cr, float cg, float cb) {
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    GLfloat em[]   = { cr * 0.85f, cg * 0.85f, cb * 0.85f, 1.0f };
    GLfloat spec[] = { 1.0f,       1.0f,       1.0f,       1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,  em);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  spec);
    glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, 128.0f);
}

void desativarMaterialGlow() {
    GLfloat emZero[]   = { 0.0f, 0.0f, 0.0f, 1.0f };
    GLfloat specZero[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,  emZero);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  specZero);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_NORMALIZE);
    glDisable(GL_LIGHTING);
}

// ---------------------------------------------------------------------------
// configurarLuzesDinamicas — GL_LIGHT1 (Stand) + GL_LIGHT2–5 (até 4 balas).
//   Deve ser chamada APÓS configurarCamera() (posição em espaço de mundo).
//   GL_LIGHT0 fica reservado ao mapa; GL_LIGHT1–7 são de uso dinâmico.
// ---------------------------------------------------------------------------
static void configurarLuzesDinamicas() {
    // --- GL_LIGHT1: Stand/Entidade -------------------------------------------
    float maxT  = maxTensaoDoNivel(g_jogo.protagonista.upgrades.niveis[TENSAO_UP]);
    float ratio = (maxT > 0.0f) ? (g_jogo.stand.tensaoAtual / maxT) : 0.0f;
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;
    float sr = ratio, sg = 1.0f - ratio;

    GLfloat posS[] = { g_jogo.stand.posicao.x,
                       g_jogo.stand.posicao.y + 0.5f,
                       g_jogo.stand.posicao.z, 1.0f };
    GLfloat difS[] = { sr * 2.0f, sg * 2.0f, 0.05f, 1.0f };
    GLfloat ambS[] = { sr * 0.4f, sg * 0.4f, 0.01f, 1.0f };
    glEnable(GL_LIGHT1);
    glLightfv(GL_LIGHT1, GL_POSITION,             posS);
    glLightfv(GL_LIGHT1, GL_DIFFUSE,              difS);
    glLightfv(GL_LIGHT1, GL_AMBIENT,              ambS);
    glLightf (GL_LIGHT1, GL_CONSTANT_ATTENUATION,  1.0f);
    glLightf (GL_LIGHT1, GL_LINEAR_ATTENUATION,    0.0f);
    glLightf (GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.05f);

    // --- GL_LIGHT2–5: até 4 balas ativas (mais balas = mais luz no cenário) --
    const int LUZ_MAX = 5;   // GL_LIGHT2 … GL_LIGHT5
    int luzIdx = 2;
    for (int i = 0; i < g_skills.numSlots() && luzIdx <= LUZ_MAX; ++i) {
        const SlotSkill& sl = g_skills.slot(i);
        if (!sl.ativo) continue;
        bool temCor = (sl.build.forma.corR > 0.001f ||
                       sl.build.forma.corG > 0.001f ||
                       sl.build.forma.corB > 0.001f);
        float cr = temCor ? sl.build.forma.corR : 1.0f;
        float cg = temCor ? sl.build.forma.corG : 1.0f;
        float cb = temCor ? sl.build.forma.corB : 0.2f;
        for (size_t k = 0; k < sl.pool.size() && luzIdx <= LUZ_MAX; ++k) {
            if (!sl.pool[k].ativo) continue;
            GLenum luz = (GLenum)(GL_LIGHT0 + luzIdx);
            GLfloat posB[] = { sl.pool[k].posicao.x,
                               sl.pool[k].posicao.y + 0.15f,
                               sl.pool[k].posicao.z, 1.0f };
            // Difusa alta para iluminar paredes/personagens próximos
            GLfloat difB[] = { cr * 3.5f, cg * 3.5f, cb * 3.5f, 1.0f };
            GLfloat ambB[] = { cr * 0.6f, cg * 0.6f, cb * 0.6f, 1.0f };
            glEnable(luz);
            glLightfv(luz, GL_POSITION,             posB);
            glLightfv(luz, GL_DIFFUSE,              difB);
            glLightfv(luz, GL_AMBIENT,              ambB);
            glLightf (luz, GL_CONSTANT_ATTENUATION,  1.0f);
            glLightf (luz, GL_LINEAR_ATTENUATION,    0.0f);
            glLightf (luz, GL_QUADRATIC_ATTENUATION, 0.12f);
            luzIdx++;
        }
    }
    for (int j = luzIdx; j <= LUZ_MAX; ++j)
        glDisable((GLenum)(GL_LIGHT0 + j));
}

static void desligarLuzesDinamicas() {
    for (int i = 1; i <= 5; ++i)
        glDisable((GLenum)(GL_LIGHT0 + i));
}

static void desenharCena() {
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    configurarCamera();
    configurarLuzesDinamicas();   // ponto de luz do Stand + balas afetam o mapa

    desenharMapaBalada();
    glDisable(GL_LIGHTING);       // mapa usa lighting; o resto do jogo usa glColor3f direto
    desenharZumbis();
    desenharProjeteisZumbi();
    desenharSkills();
    desenharParticulas();
    desenharHitboxDevorar();
    desenharJogador();
    desenharStand();
    desligarLuzesDinamicas();
}

#endif // RENDERCENA_H
