#ifndef HUD_H
#define HUD_H

// =============================================================================
//  Hud.h — HUD, menus de level-up e pausa (extraído de Main.cpp)
// =============================================================================

// ---------------------------------------------------------------------------
// Primitivas 2D reutilizáveis no HUD
// ---------------------------------------------------------------------------
static void desenharRetanguloPreenchido(float x, float y, float w, float h,
                                         float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
        glVertex2f(x,   y);    glVertex2f(x+w, y);
        glVertex2f(x+w, y+h);  glVertex2f(x,   y+h);
    glEnd();
}

static void desenharContorno(float x, float y, float w, float h,
                              float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x,   y);    glVertex2f(x+w, y);
        glVertex2f(x+w, y+h);  glVertex2f(x,   y+h);
    glEnd();
}

// Ícone bala: corpo retangular + ponta triangular, centrado em (cx, cy)
static void desenharIconeBala(float cx, float cy,
                               float r, float g, float b, float esc) {
    float cw = 8.0f * esc, ch = 14.0f * esc, pt = 8.0f * esc;
    float bx = cx - cw * 0.5f;
    float by = cy - (ch + pt) * 0.5f;
    desenharRetanguloPreenchido(bx, by, cw, ch, r, g, b);
    glColor3f(r, g, b);
    glBegin(GL_TRIANGLES);
        glVertex2f(bx,      by + ch);
        glVertex2f(bx + cw, by + ch);
        glVertex2f(cx,      by + ch + pt);
    glEnd();
}

// Ícone "+": dois retângulos cruzados, centrado em (cx, cy)
static void desenharIconePlus(float cx, float cy,
                               float r, float g, float b, float esc) {
    float aw = 5.0f * esc, ah = 14.0f * esc;
    desenharRetanguloPreenchido(cx - aw*0.5f, cy - ah*0.5f, aw, ah, r, g, b);
    desenharRetanguloPreenchido(cx - ah*0.5f, cy - aw*0.5f, ah, aw, r, g, b);
}

// ---------------------------------------------------------------------------
// HUD: fileira de 4 quadrados de atributo da arma (inferior-central)
// ---------------------------------------------------------------------------
static void desenharAtributosArma() {
    const float W = (float)g_winW;

    // Layout — ajuste aqui para reposicionar tudo
    const float SQ   = 56.0f;   // lado do quadrado (px)
    const float GAP  = 16.0f;   // espaço entre quadrados
    const float SB_W = 16.0f;   // largura de cada barrinha de nível
    const float SB_H =  6.0f;   // altura das barrinhas
    const float SB_G =  4.0f;   // gap entre barrinhas
    const float Y_SQ = 18.0f;   // Y base dos quadrados (y=0 = rodapé)
    const float Y_BAR=  7.0f;   // Y base das barrinhas

    const float totalW = 4.0f * SQ + 3.0f * GAP;
    const float startX = (W - totalW) * 0.5f;

    // {índice no array niveis[], R, G, B, tipo-ícone: 0=bala 1=plus 2=bala+plus}
    struct SlotAtrib { int idx; float r, g, b; int icone; };
    SlotAtrib slots[4] = {
        { DANO,       0.89f, 0.29f, 0.29f, 0 },
        { QUANTIDADE, 0.39f, 0.60f, 0.13f, 2 },
        { CADENCIA,   0.95f, 0.95f, 0.95f, 1 },
        { PERFURACAO, 0.22f, 0.54f, 0.87f, 0 }
    };

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (int i = 0; i < 4; ++i) {
        const float qx = startX + i * (SQ + GAP);
        const float qy = Y_SQ;
        const float cx = qx + SQ * 0.5f;
        const float cy = qy + SQ * 0.5f;
        const float aR = slots[i].r, aG = slots[i].g, aB = slots[i].b;
        int nivel = g_jogo.protagonista.upgrades.niveis[slots[i].idx];

        // Fundo escuro semi-transparente
        glColor4f(0.0f, 0.0f, 0.0f, 0.45f);
        glBegin(GL_QUADS);
            glVertex2f(qx,    qy);     glVertex2f(qx+SQ, qy);
            glVertex2f(qx+SQ, qy+SQ); glVertex2f(qx,    qy+SQ);
        glEnd();

        glDisable(GL_BLEND);

        // Contorno na cor do atributo
        glLineWidth(1.5f);
        desenharContorno(qx, qy, SQ, SQ, aR, aG, aB);
        glLineWidth(1.0f);

        // Ícone centrado
        if (slots[i].icone == 0) {
            desenharIconeBala(cx, cy, aR, aG, aB, 1.0f);
        } else if (slots[i].icone == 1) {
            desenharIconePlus(cx, cy, aR, aG, aB, 1.0f);
        } else {
            // Bala ligeiramente deslocada + "+" pequeno no canto superior direito
            desenharIconeBala(cx - 4.0f, cy, aR, aG, aB, 1.0f);
            desenharIconePlus(qx + SQ - 11.0f, qy + SQ - 11.0f, aR, aG, aB, 0.55f);
        }

        // 3 barrinhas de nível sob o quadrado
        const float barsW = 3.0f * SB_W + 2.0f * SB_G;
        const float bx0   = cx - barsW * 0.5f;
        for (int b = 0; b < 3; ++b) {
            float bx   = bx0 + b * (SB_W + SB_G);
            bool  acesa = (nivel > b);
            desenharRetanguloPreenchido(bx, Y_BAR, SB_W, SB_H,
                acesa ? aR : 0.25f,
                acesa ? aG : 0.25f,
                acesa ? aB : 0.23f);
        }

        glEnable(GL_BLEND);
    }

    glDisable(GL_BLEND);
}

static void desenharBarra(float x, float y, float w, float h,
                           float valor, float maximo,
                           float rF, float gF, float bF,
                           float rB, float gB, float bB) {
    // Fundo
    glColor3f(rB, gB, bB);
    glBegin(GL_QUADS);
        glVertex2f(x, y);       glVertex2f(x+w, y);
        glVertex2f(x+w, y+h);   glVertex2f(x, y+h);
    glEnd();
    // Preenchimento
    float fill = (maximo > 0.0f) ? (valor / maximo) * w : 0.0f;
    if (fill < 0.0f) fill = 0.0f;
    glColor3f(rF, gF, bF);
    glBegin(GL_QUADS);
        glVertex2f(x, y);       glVertex2f(x+fill, y);
        glVertex2f(x+fill, y+h);glVertex2f(x, y+h);
    glEnd();
}

static void desenharHUD() {
    entrarModo2D();

    Jogador& p = g_jogo.protagonista;
    const float W = (float)g_winW, H = (float)g_winH;

    // --- Constantes de layout (ajuste aqui para reposicionar tudo de uma vez) ---
    const float MARG_E    = 10.0f;   // margem esquerda
    const float MARG_D    = 12.0f;   // margem direita
    const float BARRA_W   = 200.0f;  // largura de todas as barras
    const float BARRA_HP  = 18.0f;   // altura barra HP
    const float BARRA_T   = 18.0f;   // altura barra Tensão
    const float BARRA_XP  = 12.0f;   // altura barra XP
    const float PASSO     = 30.0f;   // espaçamento vertical entre barras (bottom)
    const float ROT_DENT  = 14.0f;   // offset Y do rótulo dentro da barra

    // Posições Y (bottom de cada barra; Y cresce para cima)
    float yHP  = H - PASSO;           // H - 30
    float yT   = yHP - PASSO;         // H - 60
    float yXP  = yT  - PASSO;         // H - 90
    float yDev = yXP - BARRA_XP - PASSO + 2.0f; // H - 120

    // Painel semi-transparente atrás das barras (opcional: remove se não quiser)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.0f, 0.0f, 0.0f, 0.40f);
        glBegin(GL_QUADS);
            glVertex2f(MARG_E - 4.0f, yDev - 4.0f);
            glVertex2f(MARG_E + BARRA_W + 4.0f, yDev - 4.0f);
            glVertex2f(MARG_E + BARRA_W + 4.0f, H);
            glVertex2f(MARG_E - 4.0f, H);
        glEnd();
        glDisable(GL_BLEND);
    }

    char buf[96];

    // Barra de HP
    desenharBarra(MARG_E, yHP, BARRA_W, BARRA_HP,
                  (float)p.hp, (float)p.hpMaximo,
                  0.9f, 0.1f, 0.1f,   0.3f, 0.0f, 0.0f);
    std::snprintf(buf, sizeof(buf), "HP %d/%d", p.hp, p.hpMaximo);
    desenharTextoSombra(MARG_E + 2.0f, yHP + ROT_DENT, buf, 1.0f, 1.0f, 1.0f);

    // Barra de Tensão
    desenharBarra(MARG_E, yT, BARRA_W, BARRA_T,
                  g_jogo.stand.tensaoAtual,
                  maxTensaoDoNivel(g_jogo.protagonista.upgrades.niveis[TENSAO_UP]),
                  g_jogo.stand.emSobrecarga ? 1.0f : 0.3f,
                  g_jogo.stand.emSobrecarga ? 0.3f : 0.5f,
                  0.0f,
                  0.1f, 0.1f, 0.3f);
    std::snprintf(buf, sizeof(buf), "Tensao %.0f%%", g_jogo.stand.tensaoAtual);
    desenharTextoSombra(MARG_E + 2.0f, yT + ROT_DENT, buf, 1.0f, 1.0f, 1.0f);

    // Barra de XP
    desenharBarra(MARG_E, yXP, BARRA_W, BARRA_XP,
                  (float)p.xpAtual, (float)p.xpParaProximoNivel,
                  0.2f, 0.8f, 1.0f,   0.05f, 0.1f, 0.2f);
    std::snprintf(buf, sizeof(buf), "Nv %d  XP %d/%d",
                  p.nivel, p.xpAtual, p.xpParaProximoNivel);
    desenharTextoSombra(MARG_E + 2.0f, yXP + 8.0f, buf, 0.7f, 0.9f, 1.0f);

    // Status do Devorar
    {
        Entidade& st = g_jogo.stand;
        if (st.temporizadorDevorar > 0.0f) {
            desenharTextoSombra(MARG_E, yDev, "DEVORANDO!", 1.0f, 0.15f, 0.0f);
        } else if (st.temporizadorCooldownDevorar > 0.0f) {
            std::snprintf(buf, sizeof(buf), "Devorar [recarga %.1fs]",
                          st.temporizadorCooldownDevorar);
            desenharTextoSombra(MARG_E, yDev, buf, 0.55f, 0.40f, 0.40f);
        } else {
            desenharTextoSombra(MARG_E, yDev, "ESPACO: Devorar [Pronto]", 0.75f, 0.75f, 0.75f);
        }
    }

    // --- Canto superior direito: timer e kills alinhados à direita ---
    float xDir = W - MARG_D;
    {
        int t_total = (int)g_jogo.tempoSobrevivido;
        int t_min   = t_total / 60;
        int t_sec   = t_total % 60;
        std::snprintf(buf, sizeof(buf), "%02d:%02d", t_min, t_sec);
        desenharTextoSombra(xDir - (float)larguraTexto(buf), H - 22.0f,
                            buf, 1.0f, 1.0f, 0.6f);
    }
    std::snprintf(buf, sizeof(buf), "Kills: %d", g_kills);
    desenharTextoSombra(xDir - (float)larguraTexto(buf), H - 46.0f,
                        buf, 1.0f, 0.5f, 0.5f);

    // --- Skills equipadas (canto inferior esquerdo, nomes truncados com ...) ---
    {
        float sx = MARG_E, sy = 10.0f;
        const int LIM_PX = 180;
        for (int i = 0; i < MAX_SLOTS_SKILL; ++i) {
            const SlotSkill& sl = g_skills.slot(i);
            if (!sl.ativo) continue;
            const char* nome = SkillFactory::nomeDe(sl.build.id);
            char nomeT[48];
            std::strncpy(nomeT, nome, 47);
            nomeT[47] = '\0';
            std::snprintf(buf, sizeof(buf), "[%d] %s", i+1, nomeT);
            // Reduz até caber, acrescentando reticências
            while (larguraTexto(buf) > LIM_PX && std::strlen(nomeT) > 0) {
                nomeT[std::strlen(nomeT) - 1] = '\0';
                std::snprintf(buf, sizeof(buf), "[%d] %s...", i+1, nomeT);
            }
            desenharTextoSombra(sx, sy, buf, 0.6f, 1.0f, 0.6f);
            sy += 22.0f;
        }
    }

    // --- Sobrecarga (centralizado) ---
    if (g_jogo.stand.emSobrecarga) {
        desenharTextoCentralizado(W * 0.5f, H * 0.5f + 60.0f,
                                  "SOBRECARGA!", 1.0f, 0.2f, 0.0f);
    }

    // --- Game Over (centralizado) ---
    if (g_jogoTerminado) {
        desenharTextoCentralizado(W * 0.5f, H * 0.5f,
                                  "GAME OVER  [R] Reiniciar", 1.0f, 0.2f, 0.2f);
    }

    // =========================================================================
    // HUD DO SISTEMA DE BOSS
    // =========================================================================

    // --- FASE_AGUARDANDO_BOSS: aviso + contador de zumbis restantes ----------
    if (g_jogo.fasePartida == FASE_AGUARDANDO_BOSS) {
        // Conta apenas zumbis comuns vivos (não conta Boss)
        int vivos = 0;
        for (size_t s2 = 0; s2 < g_jogo.horda.size(); ++s2)
            if (g_jogo.horda[s2].vivo && !g_jogo.horda[s2].ehBoss) vivos++;

        // Painel de aviso no topo
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.4f, 0.0f, 0.0f, 0.65f);
        glBegin(GL_QUADS);
            glVertex2f(W*0.25f, H - 70.0f);
            glVertex2f(W*0.75f, H - 70.0f);
            glVertex2f(W*0.75f, H - 10.0f);
            glVertex2f(W*0.25f, H - 10.0f);
        glEnd();
        glDisable(GL_BLEND);

        desenharTextoCentralizado(W*0.5f, H - 30.0f,
                                  "BOSS SE APROXIMA!", 1.0f, 0.3f, 0.0f);
        {
            char bufA[64];
            std::snprintf(bufA, sizeof(bufA),
                          "Elimine os zumbis restantes: %d", vivos);
            desenharTextoCentralizado(W*0.5f, H - 52.0f,
                                      bufA, 1.0f, 0.75f, 0.0f);
        }
    }

    // --- FASE_BOSS: barra de vida do Boss no topo da tela --------------------
    if (g_jogo.fasePartida == FASE_BOSS) {
        for (size_t bi = 0; bi < g_jogo.horda.size(); ++bi) {
            if (!g_jogo.horda[bi].ehBoss || !g_jogo.horda[bi].vivo) continue;
            const Zumbi& bss = g_jogo.horda[bi];

            const float bBarW = 520.0f;
            const float bBarH = 24.0f;
            const float bBarX = (W - bBarW) * 0.5f;
            const float bBarY = H - 58.0f;

            // Fundo do painel
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(0.08f, 0.0f, 0.14f, 0.88f);
            glBegin(GL_QUADS);
                glVertex2f(bBarX - 8.0f, bBarY - 26.0f);
                glVertex2f(bBarX + bBarW + 8.0f, bBarY - 26.0f);
                glVertex2f(bBarX + bBarW + 8.0f, bBarY + bBarH + 6.0f);
                glVertex2f(bBarX - 8.0f, bBarY + bBarH + 6.0f);
            glEnd();
            glDisable(GL_BLEND);

            // Nome do Boss
            desenharTextoCentralizado(W * 0.5f, bBarY + bBarH + 2.0f - 24.0f,
                                      "CHEFE DA HORDA",
                                      0.85f, 0.2f, 1.0f);

            // Borda da barra (roxo)
            glColor3f(0.5f, 0.0f, 0.9f);
            glLineWidth(2.0f);
            glBegin(GL_LINE_LOOP);
                glVertex2f(bBarX - 2.0f, bBarY - 2.0f);
                glVertex2f(bBarX + bBarW + 2.0f, bBarY - 2.0f);
                glVertex2f(bBarX + bBarW + 2.0f, bBarY + bBarH + 2.0f);
                glVertex2f(bBarX - 2.0f, bBarY + bBarH + 2.0f);
            glEnd();
            glLineWidth(1.5f);

            // Barra de HP (roxa → preta quando baixa)
            float fracHP = (float)bss.vida / (float)BOSS_VIDA_MAXIMA;
            if (fracHP < 0.0f) fracHP = 0.0f;
            float hpR = 0.4f + fracHP * 0.2f;
            float hpG = 0.0f;
            float hpB = 0.8f + fracHP * 0.1f;
            desenharBarra(bBarX, bBarY, bBarW, bBarH,
                          (float)bss.vida, (float)BOSS_VIDA_MAXIMA,
                          hpR, hpG, hpB,
                          0.08f, 0.0f, 0.12f);

            // Texto "vida_atual / vida_max" centralizado na barra
            {
                char bufB[64];
                std::snprintf(bufB, sizeof(bufB),
                              "%d / %d", bss.vida, BOSS_VIDA_MAXIMA);
                desenharTextoCentralizado(W * 0.5f, bBarY + 4.0f,
                                          bufB, 1.0f, 1.0f, 1.0f);
            }
            break; // apenas um Boss na horda
        }
    }

    // --- Mensagens de evento (BOSS APARECEU! / BOSS DERROTADO!) --------------
    if (g_jogo.tempMensagemBoss > 0.0f) {
        // Proporção de vida restante da mensagem para fade (de 1→0)
        float fade = g_jogo.tempMensagemBoss / BOSS_MSG_DURACAO;
        if (fade > 1.0f) fade = 1.0f;

        const char* msgBoss = "";
        float mR = 1.0f, mG = 1.0f, mB = 1.0f;

        if (g_jogo.fasePartida == FASE_BOSS) {
            msgBoss = "BOSS APARECEU!";
            mR = 1.0f; mG = 0.15f; mB = 1.0f;
        } else if (g_jogo.fasePartida == FASE_VITORIA) {
            msgBoss = "BOSS DERROTADO!";
            mR = 0.1f; mG = 1.0f;  mB = 0.2f;
        }

        if (msgBoss[0] != '\0') {
            // Sombra + texto principal com fade de opacidade simulado por cor
            float rS = mR * fade, gS = mG * fade, bS = mB * fade;
            desenharTexto(W*0.5f - larguraTexto(msgBoss)*0.5f + 2.0f,
                          H*0.5f + 30.0f - 2.0f,
                          msgBoss, 0.0f, 0.0f, 0.0f);
            desenharTexto(W*0.5f - larguraTexto(msgBoss)*0.5f,
                          H*0.5f + 30.0f,
                          msgBoss, rS, gS, bS);
        }
    }
    // =========================================================================

    // --- Quadrados de atributo da arma (inferior-central) ---
    desenharAtributosArma();

    sairModo2D();
}

// =============================================================================
//  MENU DE LEVEL-UP
// =============================================================================

static void desenharMenuLevelUp() {
    if (!g_jogo.pausadoParaUpgrade) return;

    entrarModo2D();

    const float W = (float)g_winW, H = (float)g_winH;

    // Fundo semi-transparente
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.65f);
    glBegin(GL_QUADS);
        glVertex2f(0, 0); glVertex2f(W, 0);
        glVertex2f(W, H); glVertex2f(0, H);
    glEnd();
    glDisable(GL_BLEND);

    desenharTextoCentralizado(W * 0.5f, H - 80.0f,
                              "LEVEL UP!  Escolha uma melhoria:", 1.0f, 0.9f, 0.2f);

    MenuLevelUp& menu = g_jogo.menuAtual;

    // Layout do card
    const float PADDING  = 12.0f;   // margem interna
    const float cardW    = 270.0f;  // largura do card
    const float cardH    = 130.0f;  // altura — acomoda faixa de título + 3 linhas + subtítulo
    const float TITULO_H = 28.0f;   // faixa de título colorida no topo do card
    const float LINHA_H  = 20.0f;   // altura de linha do texto quebrado
    const float GAPCARD  = 20.0f;   // espaço entre cards

    const float totalW  = menu.quantidade * cardW + (menu.quantidade - 1) * GAPCARD;
    const float startX  = (W - totalW) * 0.5f;
    const float startY  = H * 0.5f - cardH * 0.5f;

    for (int i = 0; i < menu.quantidade; ++i) {
        LevelUpChoice& c = menu.escolhas[i];
        const float cx = startX + i * (cardW + GAPCARD);

        float rR, rG, rB;
        corRaridade(c.raridade, rR, rG, rB);

        // Fundo do card
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.08f, 0.08f, 0.14f, 0.96f);
        glBegin(GL_QUADS);
            glVertex2f(cx,       startY);
            glVertex2f(cx+cardW, startY);
            glVertex2f(cx+cardW, startY+cardH);
            glVertex2f(cx,       startY+cardH);
        glEnd();

        // Faixa de título colorida (topo do card)
        glColor4f(rR * 0.35f, rG * 0.35f, rB * 0.35f, 0.92f);
        glBegin(GL_QUADS);
            glVertex2f(cx,       startY + cardH - TITULO_H);
            glVertex2f(cx+cardW, startY + cardH - TITULO_H);
            glVertex2f(cx+cardW, startY + cardH);
            glVertex2f(cx,       startY + cardH);
        glEnd();
        glDisable(GL_BLEND);

        // Borda dupla na cor da raridade (mais visível)
        glColor3f(rR, rG, rB);
        glBegin(GL_LINE_LOOP);
            glVertex2f(cx,       startY);
            glVertex2f(cx+cardW, startY);
            glVertex2f(cx+cardW, startY+cardH);
            glVertex2f(cx,       startY+cardH);
        glEnd();
        glBegin(GL_LINE_LOOP);
            glVertex2f(cx+1.0f,       startY+1.0f);
            glVertex2f(cx+cardW-1.0f, startY+1.0f);
            glVertex2f(cx+cardW-1.0f, startY+cardH-1.0f);
            glVertex2f(cx+1.0f,       startY+cardH-1.0f);
        glEnd();

        // Número da tecla — centralizado na faixa de título
        char buf[8];
        std::snprintf(buf, sizeof(buf), "[%d]", i+1);
        desenharTextoCentralizado(cx + cardW * 0.5f,
                                  startY + cardH - TITULO_H + 6.0f,
                                  buf, rR, rG, rB);

        // Descrição quebrada — área útil abaixo da faixa de título
        const float textoW = cardW - 2.0f * PADDING;
        const float descY  = startY + cardH - TITULO_H - LINHA_H;
        int nLinhas = desenharTextoQuebrado(cx + PADDING, descY, textoW,
                                            LINHA_H, c.descricao,
                                            1.0f, 1.0f, 1.0f);

        // Subtítulo logo abaixo das linhas de descrição
        float subY = descY - (float)nLinhas * LINHA_H;
        if (subY < startY + 4.0f) subY = startY + 4.0f;
        desenharTextoCentralizado(cx + cardW * 0.5f, subY,
                                  c.subtitulo, 0.70f, 0.70f, 0.85f);
    }

    sairModo2D();
}

// Overlay de pause manual — escurece a tela e exibe "PAUSADO".
// Só é chamado quando g_jogo.pausaManual é true (e, por construção,
// nunca ao mesmo tempo que pausadoParaUpgrade).
static void desenharPause() {
    if (!g_jogo.pausaManual) return;

    entrarModo2D();

    const float W = (float)g_winW, H = (float)g_winH;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.55f);
    glBegin(GL_QUADS);
        glVertex2f(0, 0); glVertex2f(W, 0);
        glVertex2f(W, H); glVertex2f(0, H);
    glEnd();
    glDisable(GL_BLEND);

    desenharTextoCentralizado(W * 0.5f, H * 0.5f + 18.0f,
                              "PAUSADO", 1.0f, 1.0f, 0.35f);
    desenharTextoCentralizado(W * 0.5f, H * 0.5f - 10.0f,
                              "[ESC] ou [P] para continuar", 0.75f, 0.75f, 0.75f);

    sairModo2D();
}

#endif // HUD_H
