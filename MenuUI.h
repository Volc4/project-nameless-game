#ifndef MENUUI_H
#define MENUUI_H
static void desenharMenuInicial() {
    entrarModo2D();
    const float W = (float)g_winW, H = (float)g_winH;

    // Fundo preto sólido
    glDisable(GL_BLEND);
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
        glVertex2f(0, 0); glVertex2f(W, 0);
        glVertex2f(W, H); glVertex2f(0, H);
    glEnd();

    // Título
    const char* titulo = "Festa Zumbi";
    float tw = (float)larguraTexto(titulo);
    float tx = W * 0.5f - tw * 0.5f;
    float ty = H * 0.65f;
    desenharTexto(tx + 2.0f, ty - 2.0f, titulo, 0.4f, 0.0f, 0.2f); // sombra
    desenharTexto(tx, ty, titulo, 1.0f, 0.35f, 0.75f);             // texto rosa

    // Opções
    const char* opcoes[2] = { "Iniciar Jogo", "Sair" };
    for (int i = 0; i < 2; ++i) {
        float ow = (float)larguraTexto(opcoes[i]);
        float ox = W * 0.5f - ow * 0.5f;
        float oy = H * 0.42f - i * 40.0f;
        float corR, corG, corB;

        if (g_menuOpcao == i) {
            // Highlight: fundo semi-transparente
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(1.0f, 0.15f, 0.15f, 0.20f);
            glBegin(GL_QUADS);
                glVertex2f(ox - 12.0f, oy - 4.0f);
                glVertex2f(ox + ow + 12.0f, oy - 4.0f);
                glVertex2f(ox + ow + 12.0f, oy + 16.0f);
                glVertex2f(ox - 12.0f, oy + 16.0f);
            glEnd();
            glDisable(GL_BLEND);

            desenharTexto(ox - 22.0f, oy, ">", 1.0f, 0.9f, 0.0f); // seta indicadora
            corR = 1.0f; corG = 1.0f; corB = 1.0f;                 // texto selecionado: branco
        } else {
            corR = 0.55f; corG = 0.55f; corB = 0.55f;              // texto normal: cinza
        }
        desenharTexto(ox, oy, opcoes[i], corR, corG, corB);
    }

    // Dica de navegação
    const char* dica = "W/S ou Setas para navegar  |  ENTER para confirmar";
    float dw = (float)larguraTexto(dica);
    desenharTexto(W * 0.5f - dw * 0.5f, H * 0.12f, dica, 0.35f, 0.35f, 0.35f);

    sairModo2D();
}

static void iniciarJogo() {
    g_emMenuInicial = false;
    tocarMusicaFundo(g_musicaSecreta ? "Sons/festa_zumbi.mp3" : "Sons/musica_balada.mp3");
}

#endif // MENUUI_H
