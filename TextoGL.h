#ifndef TEXTOGL_H
#define TEXTOGL_H

static void entrarModo2D() {
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0.0, (double)g_winW, 0.0, (double)g_winH);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
}

static void sairModo2D() {
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
}

static void desenharTexto(float x, float y, const char* str,
                           float r, float g, float b) {
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (const char* c = str; *c; ++c)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
}

// Mede largura em pixels da string no bitmap HELVETICA_18.
static int larguraTexto(const char* s) {
    return (int)glutBitmapLength(GLUT_BITMAP_HELVETICA_18,
                                 (const unsigned char*)s);
}

// Centraliza horizontalmente em cx.
static void desenharTextoCentralizado(float cx, float y, const char* s,
                                      float r, float g, float b) {
    desenharTexto(cx - larguraTexto(s) * 0.5f, y, s, r, g, b);
}

// Sombra 1px (cópia escura deslocada) + texto colorido sobre ela.
static void desenharTextoSombra(float x, float y, const char* s,
                                float r, float g, float b) {
    desenharTexto(x + 1.0f, y - 1.0f, s, 0.0f, 0.0f, 0.0f);
    desenharTexto(x, y, s, r, g, b);
}

// Quebra texto por palavras dentro de larguraMax pixels, descendo alturaLinha por linha.
// Retorna o número de linhas desenhadas.
static int desenharTextoQuebrado(float x, float yTopo, float larguraMax,
                                 float alturaLinha, const char* s,
                                 float r, float g, float b) {
    char copia[512];
    std::strncpy(copia, s, 511);
    copia[511] = '\0';

    char linha[512];
    linha[0] = '\0';
    int nLinhas = 0;
    float y = yTopo;

    char* tok = std::strtok(copia, " ");
    while (tok != NULL) {
        char tentativa[512];
        if (linha[0] == '\0') {
            std::strncpy(tentativa, tok, 511);
        } else {
            std::snprintf(tentativa, sizeof(tentativa), "%s %s", linha, tok);
        }
        tentativa[511] = '\0';

        if (larguraTexto(tentativa) <= (int)larguraMax || linha[0] == '\0') {
            std::strncpy(linha, tentativa, 511);
            linha[511] = '\0';
        } else {
            desenharTexto(x, y, linha, r, g, b);
            y -= alturaLinha;
            ++nLinhas;
            std::strncpy(linha, tok, 511);
            linha[511] = '\0';
        }
        tok = std::strtok(NULL, " ");
    }
    if (linha[0] != '\0') {
        desenharTexto(x, y, linha, r, g, b);
        ++nLinhas;
    }
    return nLinhas;
}

#endif // TEXTOGL_H
