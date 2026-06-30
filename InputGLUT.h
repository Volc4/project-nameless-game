#ifndef INPUTGLUT_H
#define INPUTGLUT_H

// =============================================================================
//  InputGLUT.h — Callbacks GLUT e projeção mouse (extraído de Main.cpp)
// =============================================================================
static Vetor3D projetarMouseNoMundo(int mx, int my) {
    int viewport[4];
    double model[16], proj[16];
    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetDoublev(GL_MODELVIEW_MATRIX, model);
    glGetDoublev(GL_PROJECTION_MATRIX, proj);

    double winY = viewport[3] - my;

    // Interseção analítica com plano Y=0 — não depende de geometria sob o cursor.
    // glReadPixels de profundidade puxava a mira para o corpo dos inimigos.
    double nx, ny, nz, fx, fy, fz;
    gluUnProject(mx, winY, 0.0, model, proj, viewport, &nx, &ny, &nz);
    gluUnProject(mx, winY, 1.0, model, proj, viewport, &fx, &fy, &fz);

    double dy = fy - ny;
    float  t  = (dy < -1e-6 || dy > 1e-6) ? (float)(-ny / dy) : 0.0f;

    Vetor3D v;
    v.x = (float)(nx + t * (fx - nx));
    v.y = 0.0f;
    v.z = (float)(nz + t * (fz - nz));
    return v;
}

static void cbDisplay() {
    if (g_emMenuInicial) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        desenharMenuInicial();
        glutSwapBuffers();
        return;
    }
    desenharCena();
    desenharHUD();
    desenharMenuLevelUp();
    desenharPause();
    glutSwapBuffers();
}

static float g_ultimoTempo = 0.0f;

static void cbIdle() {
    float agora = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float dt    = agora - g_ultimoTempo;
    g_ultimoTempo = agora;
    if (dt > 0.05f) dt = 0.05f;   // cap de 50ms para evitar pulos grandes

    if (g_emMenuInicial) { glutPostRedisplay(); return; }

    if (!g_jogo.jogoPausado && !g_jogoTerminado) {
        atualizarJogador(dt);
        atualizarDevorar(g_jogo, dt);
        atualizarZumbis(dt);
        atualizarProjeteisZumbi(dt);
        processarColisoesCenario(g_jogo);
        g_skills.atualizarTodos(g_jogo, g_grade, dt);
        atualizarParticulas(dt);
        atualizarFloatingDamage(dt);
        atualizarSpawn(dt);
        atualizarBoss(dt);
        g_jogo.houveMorteRecente = false;

        // Compacta zumbis mortos e gemas coletadas a cada 2s
        static float s_timerLimpeza = 0.0f;
        s_timerLimpeza += dt;
        if (s_timerLimpeza >= 2.0f) {
            s_timerLimpeza = 0.0f;
            size_t lw = 0;
            for (size_t li = 0; li < g_jogo.horda.size(); ++li)
                if (g_jogo.horda[li].vivo) g_jogo.horda[lw++] = g_jogo.horda[li];
            g_jogo.horda.resize(lw);
            lw = 0;
            for (size_t li = 0; li < g_jogo.gemas.size(); ++li)
                if (!g_jogo.gemas[li].coletada) g_jogo.gemas[lw++] = g_jogo.gemas[li];
            g_jogo.gemas.resize(lw);
        }
    }

    glutPostRedisplay();
}

static void cbMouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        g_cliqueMouse = (state == GLUT_DOWN);
        if (state == GLUT_DOWN && !g_jogo.jogoPausado)
            g_posicaoCursor = projetarMouseNoMundo(x, y);
    }
}

static void cbMotion(int x, int y) {
    g_posicaoCursor = projetarMouseNoMundo(x, y);
}

static void cbPassiveMotion(int x, int y) {
    g_posicaoCursor = projetarMouseNoMundo(x, y);
}

#ifdef DEBUG_PISTOLA
static void _imprimirOffsetPistola() {
    printf("[PISTOLA] Off=(%.2f,%.2f,%.2f) Rot=(%.1f,%.1f,%.1f) Esc=%.4f\n",
           g_pistolaOffX, g_pistolaOffY, g_pistolaOffZ,
           g_pistolaRotX, g_pistolaRotY, g_pistolaRotZ, g_pistolaEsc);
}
#endif



static void cbKeyboard(unsigned char key, int /*x*/, int /*y*/) {
    // Menu inicial: navegação e confirmação
    if (g_emMenuInicial) {
        switch (key) {
            case 'w': case 'W': g_menuOpcao = (g_menuOpcao + 1) % 2; break;
            case 's': case 'S': g_menuOpcao = (g_menuOpcao + 1) % 2; break;
            case '\r': case '\n': // ENTER
                if (g_menuOpcao == 0) iniciarJogo();
                else                  exit(0);
                break;
        }
        return;
    }

    switch (key) {
        // Movimento
        case 'w': case 'W': g_teclaW = true; break;
        case 's': case 'S': g_teclaS = true; break;
        case 'a': case 'A': g_teclaA = true; break;
        case 'd': case 'D': g_teclaD = true; break;

        // Reiniciar
        case 'r': case 'R':
            if (g_jogoTerminado) {
                g_jogoTerminado = false;
                g_kills = 0;
                g_skills.limparTodos();
                pararGameOver();
                inicializarEstadoJogo();

                // Reinicia a música de fundo correta de acordo com a flag do modo secreto
                if (g_musicaSecreta) {
                    tocarMusicaFundo("Sons/festa_zumbi.mp3");
                } else {
                    tocarMusicaFundo("Sons/musica_balada.mp3");
                }
            }
            break;

        // Escolha de level-up (teclas 1, 2, 3)
        case '1': case '2': case '3': {
            if (g_jogo.pausadoParaUpgrade) {
                int idx = (key - '1');
                if (idx < g_jogo.menuAtual.quantidade) {
                    aplicarEscolhaLevelUp(g_jogo.menuAtual.escolhas[idx],
                                          g_jogo.inventario,
                                          g_jogo.protagonista.upgrades,
                                          g_skills,
                                          g_jogo.protagonista,
                                          g_jogo.arquetipoArma);
                    g_jogo.pausadoParaUpgrade = false;
                    g_jogo.jogoPausado        = g_jogo.pausaManual;
                }
            }
            break;
        }

        // Música secreta (Tecla 0)
        case '0':
            if (!g_musicaSecreta) {
                g_musicaSecreta = true;
                tocarMusicaFundo("Sons/festa_zumbi.mp3"); // Caminho atualizado!
            } else {
                g_musicaSecreta = false;
                tocarMusicaFundo("Sons/musica_balada.mp3"); // Caminho atualizado!
            }
            break;

        // Devorar (ESPAÇO) — só registra o pedido; resolução é em atualizarDevorar()
        case ' ':
            g_jogo.stand.devorarPedido = true;
            break;

        // Parry (tecla Q)
        case 'q': case 'Q':
            if (g_jogo.stand.temporizadorCooldown <= 0.0f) {
                g_jogo.stand.parryAtivo          = true;
                g_jogo.stand.temporizadorParry   = 0.15f;
                g_jogo.stand.temporizadorCooldown = g_jogo.stand.cooldownParry;
            }
            break;

        // Pause manual — ESC e P (maiúsculo) alternam o pause.
        // Lowercase 'p' fica reservado para calibração de pistola (#ifdef DEBUG_PISTOLA).
        // Não alterna se estiver no menu de level-up ou em game over.
        case 27:    // ESC
        case 'P':
            if (!g_jogo.pausadoParaUpgrade && !g_jogoTerminado) {
                g_jogo.pausaManual = !g_jogo.pausaManual;
                g_jogo.jogoPausado  = g_jogo.pausaManual;
                if (g_jogo.pausaManual) pausarMusicaFundo();
                else                    retomarMusicaFundo();
            }
            break;

#ifdef DEBUG_PISTOLA
        // --- Calibração ao vivo do bone socket da pistola ---
        //   Posição  : U/I = X-/X+  |  O/P = Y-/Y+  |  K/L = Z-/Z+  (passo 1 unit)
        //   Rotação  : 7/8 = RotX   |  9/- = RotY   |  =/\ = RotZ   (passo 5°)
        //   Escala   : , = -10%     |  . = +10%
        //   Resultado impresso no console a cada tecla; copie os valores finais.
        case 'u': g_pistolaOffX -= 1.0f; _imprimirOffsetPistola(); break;
        case 'i': g_pistolaOffX += 1.0f; _imprimirOffsetPistola(); break;
        case 'o': g_pistolaOffY -= 1.0f; _imprimirOffsetPistola(); break;
        case 'p': g_pistolaOffY += 1.0f; _imprimirOffsetPistola(); break;
        case 'k': g_pistolaOffZ -= 1.0f; _imprimirOffsetPistola(); break;
        case 'l': g_pistolaOffZ += 1.0f; _imprimirOffsetPistola(); break;
        case '7': g_pistolaRotX -= 5.0f; _imprimirOffsetPistola(); break;
        case '8': g_pistolaRotX += 5.0f; _imprimirOffsetPistola(); break;
        case '9': g_pistolaRotY -= 5.0f; _imprimirOffsetPistola(); break;
        case '-': g_pistolaRotY += 5.0f; _imprimirOffsetPistola(); break;
        case '=': g_pistolaRotZ -= 5.0f; _imprimirOffsetPistola(); break;
        case '\\': g_pistolaRotZ += 5.0f; _imprimirOffsetPistola(); break;
        case ',': g_pistolaEsc *= 0.9f;  _imprimirOffsetPistola(); break;
        case '.': g_pistolaEsc *= 1.1f;  _imprimirOffsetPistola(); break;
#endif
    }
}

static void cbKeyboardUp(unsigned char key, int /*x*/, int /*y*/) {
    switch (key) {
        case 'w': case 'W': g_teclaW = false; break;
        case 's': case 'S': g_teclaS = false; break;
        case 'a': case 'A': g_teclaA = false; break;
        case 'd': case 'D': g_teclaD = false; break;
    }
}

static void cbSpecial(int key, int /*x*/, int /*y*/) {
    if (g_emMenuInicial) {
        if (key == GLUT_KEY_UP || key == GLUT_KEY_DOWN)
            g_menuOpcao = (g_menuOpcao + 1) % 2;
    }
}

static void cbReshape(int w, int h) {
    g_winW = w;
    g_winH = h;
    glViewport(0, 0, w, h);
}

#endif // INPUTGLUT_H
