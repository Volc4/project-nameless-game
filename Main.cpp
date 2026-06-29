// =============================================================================
//  Main.cpp — Loop principal data-driven (Fase 6.5)
//
//  ARQUITETURA
//  -----------
//  O jogo usa OpenGL/GLUT para renderização e entrada. A lógica está
//  distribuída nos headers data-driven; este arquivo é o host que:
//    1. Define as funções "contrato" exigidas por SkillExecutor.h
//    2. Implementa as primitivas de desenho exigidas por SkillRender.h
//    3. Mantém o EstadoDoJogo global e o SkillManager global
//    4. Inicializa o catálogo (registrarSkillsPadrao) e o inventário
//    5. Implementa os callbacks GLUT: display, idle, mouse, keyboard
//    6. Gerencia os menus de level-up e a câmera isométrica
//
//  ORDEM DE INCLUDES (crítica para evitar ciclos):
//    Entities.h  →  SkillTypes.h (inclui RuntimeSkill.h)
//    SkillManager.h (inclui SpatialGrid.h ao final, após a forward decl)
//    ProgressionSystem.h (inclui tudo da progressão)
//    GameLogic.h (lógica de jogo: spawn, XP, colisão jogador-zumbi)
//    SpatialGrid.h (grade espacial; já incluída por SkillManager.h)
//    SkillRender.h (desenho data-driven)
//    SkillRegistry.h (catálogo de skills concretas)
// =============================================================================

// --- OpenGL/GLUT -------------------------------------------------------------
#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

// --- Cabeçalhos do projeto ---------------------------------------------------
#include "Entities.h"
#include "SkillTypes.h"        // FormaType, MovimentoType…  + RuntimeSkill.h
#include "SkillFactory.h"
#include "SkillCatalog.h"
#include "SkillInventory.h"
#include "SkillRegistry.h"     // registrarSkillsPadrao()
#include "MathUtils.h"         // calcularDistanciaQuadrada, verificarColisao…
#include "SkillExecutor.h"     // executarSkill, resolverEfeitos…
#include "SkillManager.h"      // SkillManager + SpatialGrid.h (incluído internamente)
#include "ProgressionSystem.h" // montarBuildCompleta, sortearRecompensas…
#include "ArquetipoSystem.h"   // especialização irreversível da arma
#include "ArmaInteligente.h"   // classe Arma Inteligente (exclusiva, 3 níveis)
#include "SkillRender.h"       // desenharSkill, desenharTodasSkills
#include "Map.h"               // mapa balada: desenharMapaBalada, processarColisoesCenario
#include "Audio.h"

// --- Modelo animado da protagonista (Assimp + GLM + stb_image) ---------------
#define STB_IMAGE_IMPLEMENTATION
#include "ModeloAnimado.h"

// --- STL / CRT ---------------------------------------------------------------
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <ctime>

// =============================================================================
//  CONSTANTES GLOBAIS
// =============================================================================

// --- Devorar (ESPAÇO) — parâmetros centralizados aqui ---------------------------
static const float DEVORAR_DURACAO_JANELA           = 0.35f;  // s de janela ativa
static const float DEVORAR_ALCANCE                  = 3.0f;   // raio do setor
static const float DEVORAR_ANGULO                   = 1.67f;// abertura (~90°)
static const float DEVORAR_COOLDOWN                 = 1.5f;   // s entre usos
static const float DEVORAR_LUNGE                    = 0.5f;   // avanço na ativação
static const bool  DEVORAR_SUCESSO_LIMPA_SOBRECARGA = true;   // sucesso sai do overload

// --- Inimigos ---------------------------------------------------------------
static const float SPAWN_INTERVALO    = 3.0f;  // segundos entre ticks de spawn
static const int   SPAWN_BASE         = 3;     // zumbis base por tipo por tick (multiplicado pelo mult exponencial)
static const int   SPAWN_MULT_MAX     = 64;    // multiplicador máximo (6 dobramentos)
static const int   HORDA_MAX          = 1500;   // cap total de zumbis ativos
static const float SPAWN_RAIO_MIN     = 32.0f; // distância mínima do jogador no spawn
static const float SPAWN_RAIO_MAX     = 45.0f; // distância máxima do jogador no spawn (fora da tela)
static const float ATIRADOR_DIST      = 15.0f; // distância preferida do Atirador
static const float ATIRADOR_COOLDOWN  = 2.5f;  // s entre disparos do Atirador
static const float PROJETIL_ZUMBI_VEL = 12.0f; // velocidade dos projéteis
static const int   PROJETIL_ZUMBI_DANO = 1;    // dano dos projéteis
static const float EXPLOSAO_RAIO      = 3.0f;  // raio da explosão do Explosivo

static const float ARENA_HALF   = 150.0f;   // metade do lado da arena quadrada
static const float CAM_DIST     = 30.0f;    // distância da câmera ao ponto focal
static const float CAM_ANGLE_X  = 45.0f;   // inclinação da câmera (graus)
static const int   JANELA_W     = 1024;
static const int   JANELA_H     = 768;

// Tamanho real da janela (atualizado em cbReshape)
static int g_winW = JANELA_W, g_winH = JANELA_H;

// Constante exigida por SkillExecutor.h (extern)
const float FATOR_DANO_SOBRECARGA = 2.5f;

// =============================================================================
//  ESTADO GLOBAL
// =============================================================================

static bool g_musicaSecreta = false; // Flag da música secreta



static EstadoDoJogo  g_jogo;
static SkillManager  g_skills;
static GradeEspacial g_grade;

// Posição do cursor no mundo (plano XZ, y=0) — atualizada pelo mouse
static Vetor3D g_posicaoCursor = {0.0f, 0.0f, 0.0f};

// Teclas pressionadas
static bool g_teclaW = false, g_teclaA = false,
            g_teclaS = false, g_teclaD = false;

// Botão esquerdo do mouse pressionado (disparo contínuo)
static bool g_cliqueMouse = false;

// Flag de estado
static bool g_jogoTerminado = false;
static int  g_kills         = 0;

// --- Modelo da protagonista Sofia -------------------------------------------
static ModeloAnimado g_sofia;
static bool          g_sofiaCarregada = false;

// --- Pistola (prop estático, bone socket na mão da Sofia) -------------------
static ModeloAnimado g_pistola;
static bool          g_pistolaCarregada = false;

// Offset de encaixe — calibre com as teclas DEBUG_PISTOLA e fixe os valores finais.
// Para desabilitar as teclas de debug, comente o #define abaixo.
//#define DEBUG_PISTOLA
static float g_pistolaOffX = -32.00f;
static float g_pistolaOffY =  27.00f;
static float g_pistolaOffZ =  10.00f;
static float g_pistolaRotX = -60.0f;
static float g_pistolaRotY = -20.0f;
static float g_pistolaRotZ = -100.0f;
static float g_pistolaEsc  =  21.2095f;

// Diminuímos a escala em 20% (de 0.01f para 0.008f)
static const float   SOFIA_ESCALA     = 0.008f;

// Mantém a correção de rotação
static const float   SOFIA_ROT_OFFSET = 90.0f;

// Subimos ela mais no eixo Y. Como ela estava nas coxas com 0.9f, 
// 1.3f deve ser o suficiente para puxar as canelas e as botas pra fora.
static const float   SOFIA_Y_OFFSET   = 2.0f;

// =============================================================================
//  PROTÓTIPOS INTERNOS
// =============================================================================

static void inicializarEstadoJogo();
static void atualizarJogador(float dt);
static void atualizarZumbis(float dt);
static void atualizarDevorar(EstadoDoJogo& jogo, float dt);
static void atualizarProjeteisZumbi(float dt);
static int  multiplicadorSpawn(TipoZumbi tipo, float t);
static void atualizarParticulas(float dt);
static void atualizarFloatingDamage(float dt);
static void desenharCena();
static void desenharHUD();
static void desenharMenuLevelUp();
static void desenharHitboxDevorar();
static void desenharProjeteisZumbi();
static Vetor3D projetarMouseNoMundo(int mx, int my);

// =============================================================================
//  FUNÇÕES "CONTRATO" EXIGIDAS POR SkillExecutor.h
//  (declaradas extern lá; definidas aqui)
// =============================================================================

// ---------------------------------------------------------------------------
// processarMorteZumbi — concede XP, registra posição, marca morto.
// ---------------------------------------------------------------------------
void processarMorteZumbi(EstadoDoJogo& jogo, Zumbi& z) {
    if (!z.vivo) return;
    z.vivo = false;
    g_kills++;

    tocarMorteZumbi();

    // XP por tipo (×10 temporário para teste de arquétipos)
    int xp = 1;
    switch (z.tipo) {
        case RAPIDO:    xp = 20; break;
        case TANK:      xp = 50; break;
        case ATIRADOR:  xp = 30; break;
        case EXPLOSIVO: xp = 30; break;
        default:        xp = 10; break;
    }
    jogo.protagonista.xpAtual += xp;

    // Gema de XP
    GemaXP gema;
    gema.posicao  = z.posicao;
    gema.valorXP  = xp;
    gema.coletada = false;
    jogo.gemas.push_back(gema);

    // Hook para ORIG_KILLEDENEMY
    jogo.ultimaPosicaoMorte = z.posicao;
    jogo.houveMorteRecente  = true;

    // EXPLOSIVO: explosão em área ao morrer
    if (z.tipo == EXPLOSIVO) {
        Jogador& ep = jogo.protagonista;
        if (ep.vivo && ep.temporizadorIframe <= 0.0f) {
            float edx = ep.posicao.x - z.posicao.x;
            float edz = ep.posicao.z - z.posicao.z;
            if ((edx*edx + edz*edz) <= EXPLOSAO_RAIO * EXPLOSAO_RAIO) {
                ep.hp -= z.dano;
                ep.temporizadorIframe = ep.duracaoIframe;
                if (ep.hp <= 0) {
                    ep.hp   = 0;
                    ep.vivo = false;
                    g_jogoTerminado = true;
                    pausarMusicaFundo();
                    tocarGameOver();
                }
            }
        }
        float cR, cG, cB;
        obterCorBaseZumbi(z.tipo, cR, cG, cB);
        criarParticulasMorte(jogo, z.posicao, cR, cG, cB);
    }

    // Verifica level-up
    while (jogo.protagonista.xpAtual >= jogo.protagonista.xpParaProximoNivel) {
        jogo.protagonista.xpAtual      -= jogo.protagonista.xpParaProximoNivel;
        jogo.protagonista.xpParaProximoNivel =
            (int)(jogo.protagonista.xpParaProximoNivel * 1.4f);
        jogo.protagonista.nivel++;

        sortearRecompensas(jogo.menuAtual, jogo.inventario,
                           jogo.arquetipoArma, jogo.protagonista.upgrades);
        jogo.pausadoParaUpgrade = true;
        jogo.jogoPausado        = true;
    }
}

// ---------------------------------------------------------------------------
// criarFloatingDamage — número de dano flutuante visual.
// ---------------------------------------------------------------------------
void criarFloatingDamage(EstadoDoJogo& jogo, Vetor3D pos, int dano) {
    FloatingDamage fd;
    fd.posicao              = pos;
    fd.valorDano            = dano;
    fd.tempoRestante        = 1.2f;
    fd.tempoTotal           = 1.2f;
    fd.deslocamentoVertical = 0.0f;
    fd.corR = 1.0f; fd.corG = 0.9f; fd.corB = 0.1f;
    fd.transparencia = 1.0f;
    jogo.numerosFlutuantes.push_back(fd);
}

// ---------------------------------------------------------------------------
// criarParticulasMorte — burst de partículas coloridas.
// ---------------------------------------------------------------------------
void criarParticulasMorte(EstadoDoJogo& jogo, Vetor3D pos,
                          float cR, float cG, float cB) {
    const int N = 8;
    for (int i = 0; i < N; ++i) {
        Particula p;
        p.posicao  = pos;
        float ang  = (float)(rand() % 360) * (3.14159265f / 180.0f);
        float vel  = 3.0f + (rand() % 50) * 0.1f;
        p.velocidade.x = std::cos(ang) * vel;
        p.velocidade.y = 2.0f + (rand() % 30) * 0.1f;
        p.velocidade.z = std::sin(ang) * vel;
        p.tempoVida       = 0.4f + (rand() % 40) * 0.01f;
        p.tempoVidaMaximo = p.tempoVida;
        p.corR = cR; p.corG = cG; p.corB = cB;
        p.tamanho       = 0.15f + (rand() % 20) * 0.01f;
        p.transparencia = 1.0f;
        p.ativa = true;
        jogo.particulas.push_back(p);
    }
}

// ---------------------------------------------------------------------------
// obterCorBaseZumbi — cor RGB por tipo de zumbi (usada em partículas).
// ---------------------------------------------------------------------------
void obterCorBaseZumbi(TipoZumbi tipo, float& cR, float& cG, float& cB) {
    switch (tipo) {
        case RAPIDO:    cR = 0.2f; cG = 0.9f; cB = 0.3f; break;
        case TANK:      cR = 0.6f; cG = 0.1f; cB = 0.1f; break;
        case ATIRADOR:  cR = 0.9f; cG = 0.6f; cB = 0.1f; break;
        case EXPLOSIVO: cR = 1.0f; cG = 0.3f; cB = 0.0f; break;
        default:        cR = 0.2f; cG = 0.7f; cB = 0.2f; break;
    }
}

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
        glColor3f(rT, gT, bT);
        glVertex3f(x0, y1, z0); glVertex3f(x1, y1, z0);
        glVertex3f(x1, y1, z1); glVertex3f(x0, y1, z1);
        // Frente (Z+)
        glColor3f(rF, gF, bF);
        glVertex3f(x0, y0, z1); glVertex3f(x1, y0, z1);
        glVertex3f(x1, y1, z1); glVertex3f(x0, y1, z1);
        // Trás (Z-)
        glColor3f(rB, gB, bB);
        glVertex3f(x1, y0, z0); glVertex3f(x0, y0, z0);
        glVertex3f(x0, y1, z0); glVertex3f(x1, y1, z0);
        // Direita (X+)
        glColor3f(rR, gR, bR);
        glVertex3f(x1, y0, z1); glVertex3f(x1, y0, z0);
        glVertex3f(x1, y1, z0); glVertex3f(x1, y1, z1);
        // Esquerda (X-)
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

// =============================================================================
//  INICIALIZAÇÃO DO ESTADO DO JOGO
// =============================================================================

static void inicializarEstadoJogo() {
    // --- Jogador ---
    g_jogo.protagonista.posicao.x = 0.0f;
    g_jogo.protagonista.posicao.y = 0.0f;
    g_jogo.protagonista.posicao.z = 0.0f;
    g_jogo.protagonista.velocidade    = 10.0f;
    g_jogo.protagonista.raioColisao   = 0.5f;
    g_jogo.protagonista.emDash        = false;
    g_jogo.protagonista.vivo          = true;
    g_jogo.protagonista.hp            = 5;
    g_jogo.protagonista.hpMaximo      = 5;
    g_jogo.protagonista.temporizadorIframe = 0.0f;
    g_jogo.protagonista.duracaoIframe      = 0.35f;
    g_jogo.protagonista.xpAtual            = 0;
    g_jogo.protagonista.xpParaProximoNivel = 20;
    g_jogo.protagonista.nivel              = 1;
    for (int i = 0; i < TOTAL_UPGRADES; ++i)
        g_jogo.protagonista.upgrades.niveis[i] = 0;

    // --- Stand ---
    g_jogo.stand.posicao.x = 1.5f;
    g_jogo.stand.posicao.y = 0.0f;
    g_jogo.stand.posicao.z = 0.0f;
    g_jogo.stand.anguloMira       = 0.0f;
    g_jogo.stand.tensaoAtual      = 0.0f;
    g_jogo.stand.emSobrecarga     = false;
    g_jogo.stand.parryAtivo       = false;
    g_jogo.stand.temporizadorParry = 0.0f;
    g_jogo.stand.cooldownParry     = 1.5f;
    g_jogo.stand.temporizadorCooldown = 0.0f;
    g_jogo.stand.parryBemSucedido  = false;
    g_jogo.stand.temporizadorFeedback = 0.0f;

    g_jogo.stand.devorarPedido               = false;
    g_jogo.stand.temporizadorDevorar         = 0.0f;
    g_jogo.stand.temporizadorCooldownDevorar = 0.0f;

    // --- Horda / Gemas / Partículas ---
    g_jogo.horda.clear();
    g_jogo.gemas.clear();
    g_jogo.particulas.clear();
    g_jogo.numerosFlutuantes.clear();
    g_jogo.projeteisZumbi.clear();

    // --- Timers ---
    g_jogo.tempoSobrevivido       = 0.0f;
    g_jogo.tempoUltimoSpawn       = 0.0f;
    g_jogo.cooldownAtual          = 2.0f;
    g_jogo.quantidadeSpawnAtual   = 1;
    g_jogo.atirandoAgora          = false;
    g_jogo.pausadoParaUpgrade     = false;
    g_jogo.nivelAntesDaEscolha    = 1;
    g_jogo.jogoPausado            = false;
    g_jogo.houveMorteRecente      = false;
    g_jogo.ultimaPosicaoMorte.x = 0.0f;
    g_jogo.ultimaPosicaoMorte.y = 0.0f;
    g_jogo.ultimaPosicaoMorte.z = 0.0f;

    // --- Sistema de Arquétipos: arma começa sem especialização ---
    inicializarArquetipo(g_jogo.arquetipoArma);

    // --- Desbloqueios ---
    inicializarDesbloqueio(g_jogo.desbloqueios);

    // --- Inventário ---
    inicializarInventario(g_jogo.inventario);

    // --- Menu ---
    limparMenu(g_jogo.menuAtual);

    // --- Modo de teste (ativa se SKILL_TESTE estiver definido) ---
    activarSkillTeste(g_jogo.inventario);

    // --- Monta a build inicial ---
    montarBuildCompleta(g_jogo.inventario,
                        g_jogo.protagonista.upgrades,
                        g_skills);
}

// =============================================================================
//  SPAWN DE ZUMBIS
// =============================================================================

static Zumbi criarZumbi(TipoZumbi tipo) {
    Zumbi z;
    z.tipo        = tipo;
    z.estadoAtual = WANDER;
    z.vivo        = true;

    // Spawn em anel ao redor do jogador — fora da tela, dentro da arena
    {
        const float PI = 3.14159265f;
        float ang = (rand() % 360) * (PI / 180.0f);
        float r   = SPAWN_RAIO_MIN + (rand() % 1000) * (SPAWN_RAIO_MAX - SPAWN_RAIO_MIN) / 1000.0f;
        Jogador& jp = g_jogo.protagonista;
        float sx = jp.posicao.x + std::cos(ang) * r;
        float sz = jp.posicao.z + std::sin(ang) * r;
        float lim = ARENA_HALF - 1.0f;
        if (sx >  lim) sx =  lim;
        if (sx < -lim) sx = -lim;
        if (sz >  lim) sz =  lim;
        if (sz < -lim) sz = -lim;
        z.posicao.x = sx;
        z.posicao.y = 0.0f;
        z.posicao.z = sz;
    }

    switch (tipo) {
        case RAPIDO:
            z.velocidade = 10.0f; z.raioColisao = 0.4f; z.vida = 1;  z.dano = 1; break;
        case TANK:
            z.velocidade =  5.0f; z.raioColisao = 0.9f; z.vida = 10; z.dano = 3; break;
        case ATIRADOR:
            z.velocidade =  6.0f; z.raioColisao = 0.5f; z.vida = 2;  z.dano = 3; break;
        case EXPLOSIVO:
            z.velocidade = 10.0f; z.raioColisao = 0.6f; z.vida = 2;  z.dano = 3; break;
        default: // NORMAL
            z.velocidade = 8.0f; z.raioColisao = 0.5f; z.vida = 2;  z.dano = 1; break;
    }
    z.tiroTimer = ATIRADOR_COOLDOWN * ((rand() % 100) / 100.0f); // offset inicial aleatório
    return z;
}

// =============================================================================
//  ATUALIZAÇÃO: JOGADOR
// =============================================================================

static void atualizarJogador(float dt) {
    Jogador& p = g_jogo.protagonista;
    if (!p.vivo) return;

    if (p.temporizadorIframe > 0.0f) {
        p.temporizadorIframe -= dt;
    }

    // Movimentação WASD
    float dx = 0.0f, dz = 0.0f;
    if (g_teclaW) dz -= 1.0f;
    if (g_teclaS) dz += 1.0f;
    if (g_teclaA) dx -= 1.0f;
    if (g_teclaD) dx += 1.0f;

    float len = std::sqrt(dx * dx + dz * dz);
    if (len > 0.0001f) {
        dx /= len; dz /= len;
        float velFinal = p.velocidade * fatorVelocidadeDoNivel(p.upgrades.niveis[VELOCIDADE_UP]);
        p.posicao.x += dx * velFinal * dt;
        p.posicao.z += dz * velFinal * dt;

        float lim = ARENA_HALF - p.raioColisao;
        if (p.posicao.x >  lim) p.posicao.x =  lim;
        if (p.posicao.x < -lim) p.posicao.x = -lim;
        if (p.posicao.z >  lim) p.posicao.z =  lim;
        if (p.posicao.z < -lim) p.posicao.z = -lim;
    }

    // Mira e orientação seguem o mouse; stand fica nas costas (oposto ao cursor)
    Vetor3D dirMira = obterDirecaoNormalizada(p.posicao, g_posicaoCursor);
    const float distanciaStand = 1.5f;
    g_jogo.stand.posicao.x = p.posicao.x - dirMira.x * distanciaStand;
    g_jogo.stand.posicao.z = p.posicao.z - dirMira.z * distanciaStand;
    g_jogo.stand.anguloMira = std::atan2(dirMira.z, dirMira.x);

    // Mantém o Y do stand na mesma altura do fantasma (bob incluso)
    g_jogo.stand.posicao.y = 3.5f + std::sin(g_jogo.tempoSobrevivido * 2.0f) * 0.12f;

    // Posição da pistola no mundo — replica a cadeia de transforms de desenharJogador()
    // mais o bone socket, para que projéteis manuais saiam do cano da pistola.
    if (g_sofiaCarregada && g_sofia.temMao()) {
        float angRad = -g_jogo.stand.anguloMira + glm::radians(SOFIA_ROT_OFFSET);
        glm::mat4 m = glm::translate(glm::mat4(1.0f),
                                     glm::vec3(p.posicao.x, SOFIA_Y_OFFSET, p.posicao.z));
        m = glm::rotate(m, angRad, glm::vec3(0.0f, 1.0f, 0.0f));
        m = glm::rotate(m, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        m = glm::scale(m, glm::vec3(SOFIA_ESCALA, SOFIA_ESCALA, SOFIA_ESCALA));
        m = m * g_sofia.obterMatrizSocketMao();
        m = glm::translate(m, glm::vec3(g_pistolaOffX, g_pistolaOffY, g_pistolaOffZ));
        g_jogo.posicaoPistola.x = m[3].x;
        g_jogo.posicaoPistola.y = m[3].y;
        g_jogo.posicaoPistola.z = m[3].z;
    } else {
        g_jogo.posicaoPistola.x = p.posicao.x;
        g_jogo.posicaoPistola.y = 1.5f;
        g_jogo.posicaoPistola.z = p.posicao.z;
    }

// --- Controle de Sobrecarga e Resfriamento de Tensão ---
    float maxTensao      = maxTensaoDoNivel(g_jogo.protagonista.upgrades.niveis[TENSAO_UP]);
    float taxaDecaimento = taxaDecaimentoTensaoDoNivel(g_jogo.protagonista.upgrades.niveis[TENSAO_UP]);

    if (g_jogo.stand.emSobrecarga) {
        g_jogo.stand.tensaoAtual -= (taxaDecaimento * (4.0f / 3.0f)) * dt;
        if (g_jogo.stand.tensaoAtual <= 0.0f) {
            g_jogo.stand.tensaoAtual  = 0.0f;
            g_jogo.stand.emSobrecarga = false;
        }
    } else {
        if (g_jogo.stand.tensaoAtual >= maxTensao) {
            g_jogo.stand.emSobrecarga = true;
            g_jogo.stand.tensaoAtual  = maxTensao;
        }
        else if (!g_cliqueMouse) {
            // Esvazia a barra continuamente quando o botão não está pressionado
            g_jogo.stand.tensaoAtual -= taxaDecaimento * dt;
            if (g_jogo.stand.tensaoAtual < 0.0f) {
                g_jogo.stand.tensaoAtual = 0.0f;
            }
        }
    }

    // --- Disparo Contínuo por Clique ---
    static float s_cooldownDisparo = 0.0f;
    if (s_cooldownDisparo > 0.0f) {
        s_cooldownDisparo -= dt;
    }

    if (g_cliqueMouse && !g_jogo.stand.emSobrecarga && !g_jogo.jogoPausado) {
        g_jogo.atirandoAgora = true;
        if (s_cooldownDisparo <= 0.0f) {
            g_skills.executar(g_jogo, g_posicaoCursor);
            
            // Reproduz o efeito sonoro de disparo usando o arquivo renomeado
            tocarEfeito("Sons/disparo.mp3");

            // Cooldown do clique manual afetado pela velocidade da cadência.
            // ARQ_ESPINGARDA_TATICA seta cooldownManual > 0 para impor base ×4 mais lento.
            float baseCooldown = 0.50f;
            {
                const SlotSkill& slot0 = g_skills.slot(0);
                if (slot0.ativo && slot0.build.cooldownManual > 0.0f)
                    baseCooldown = slot0.build.cooldownManual;
            }
            int nivelCadencia  = g_jogo.protagonista.upgrades.niveis[CADENCIA];
            s_cooldownDisparo  = baseCooldown * fatorCadenciaDoNivel(nivelCadencia);  
        }
    } else {
        // A flag atirandoAgora só deve ser falsa quando o jogador não estiver clicando
        // ou estiver em sobrecarga/pausa.
        g_jogo.atirandoAgora = false;
    }

    // --- Animação da Sofia ---------------------------------------------------
    if (g_sofiaCarregada) {
        bool movendo = (g_teclaW || g_teclaS || g_teclaA || g_teclaD);
        if (movendo) {
            // Direção normalizada do input
            float idx = 0.0f, idz = 0.0f;
            if (g_teclaW) idz -= 1.0f;
            if (g_teclaS) idz += 1.0f;
            if (g_teclaA) idx -= 1.0f;
            if (g_teclaD) idx += 1.0f;
            float ilen = std::sqrt(idx*idx + idz*idz);
            if (ilen > 0.0001f) { idx /= ilen; idz /= ilen; }
            // Componente na direção de mira: positivo=frente, negativo=costas
            float ang  = g_jogo.stand.anguloMira;
            float dotFwd = idx * std::cos(ang) + idz * std::sin(ang);
            g_sofia.definirVelocidade(dotFwd >= 0.0f ? 1.0f : -1.0f);
        } else {
            // CORREÇÃO: Congela exatamente no frame 1 quando o jogador parar
            g_sofia.congelarNoFrame(14); 
        }
        g_sofia.atualizar(dt);
    }
}

// =============================================================================
//  ATUALIZAÇÃO: ZUMBIS (IA simples WANDER→CHASE)
// =============================================================================

static void atualizarZumbis(float dt) {
    Jogador& p = g_jogo.protagonista;
    int zumbisAtivos = 0; // Contador para o gerenciador de áudio

    for (size_t i = 0; i < g_jogo.horda.size(); ++i) {
        Zumbi& z = g_jogo.horda[i];
        if (!z.vivo) continue;

        zumbisAtivos++; // Registra que há zumbis vivos na tela

        float dx   = p.posicao.x - z.posicao.x;
        float dz   = p.posicao.z - z.posicao.z;
        float dist = std::sqrt(dx * dx + dz * dz);
        float nx   = (dist > 0.001f) ? dx / dist : 0.0f;
        float nz   = (dist > 0.001f) ? dz / dist : 0.0f;

        if (z.tipo == ATIRADOR) {
            // IA do Atirador
            if (dist > ATIRADOR_DIST * 1.3f) {
                z.posicao.x += nx * z.velocidade * dt;
                z.posicao.z += nz * z.velocidade * dt;
            } else if (dist < ATIRADOR_DIST * 0.7f) {
                z.posicao.x -= nx * z.velocidade * dt;
                z.posicao.z -= nz * z.velocidade * dt;
            }
            
            z.tiroTimer -= dt;
            if (z.tiroTimer <= 0.0f) {
                z.tiroTimer = ATIRADOR_COOLDOWN;
                ProjetilZumbi pz;
                pz.posicao    = z.posicao;
                pz.direcao.x  = (dist > 0.001f) ? nx : 1.0f;
                pz.direcao.y  = 0.0f;
                pz.direcao.z  = (dist > 0.001f) ? nz : 0.0f;
                pz.velocidade = PROJETIL_ZUMBI_VEL;
                pz.dano       = PROJETIL_ZUMBI_DANO;
                pz.ativo      = true;
                g_jogo.projeteisZumbi.push_back(pz);
            }
        } else {
            // IA de Perseguição Direta
            z.posicao.x += nx * z.velocidade * dt;
            z.posicao.z += nz * z.velocidade * dt;
        }

        // Limites da arena
        if (z.posicao.x >  ARENA_HALF) z.posicao.x =  ARENA_HALF;
        if (z.posicao.x < -ARENA_HALF) z.posicao.x = -ARENA_HALF;
        if (z.posicao.z >  ARENA_HALF) z.posicao.z =  ARENA_HALF;
        if (z.posicao.z < -ARENA_HALF) z.posicao.z = -ARENA_HALF;
    }

    // --- Gerenciador de Áudio Ambiente dos Zumbis ---
    static float timerSomZumbi = 2.0f; 
    
    // Só consome canal de áudio se a horda estiver ativa no mapa
    if (zumbisAtivos > 0) {
        timerSomZumbi -= dt;
        if (timerSomZumbi <= 0.0f) {
            // Sorteia um dos três sons
            int sorteio = rand() % 3;
            if (sorteio == 0)      tocarEfeito("Sons/brains1.mp3");
            else if (sorteio == 1) tocarEfeito("Sons/brains2.mp3");
            else                   tocarEfeito("Sons/passos_zumbi.mp3");
            
            // Define o tempo para o próximo som aleatoriamente entre 1.5s e 3.5s
            timerSomZumbi = 1.5f + ((rand() % 200) / 100.0f);
        }
    }
}

// Colisão zumbi→jogador (chamada dentro de SkillManager::atualizarTodos)
void processarColisaoZumbiJogador_Grade(EstadoDoJogo& jogo,
                                        GradeEspacial& /*grade*/,
                                        float dt) {
    Jogador& p = jogo.protagonista;
    if (!p.vivo || p.temporizadorIframe > 0.0f) return;

    for (size_t i = 0; i < jogo.horda.size(); ++i) {
        Zumbi& z = jogo.horda[i];
        if (!z.vivo) continue;
        if (verificarColisao(p.posicao, p.raioColisao, z.posicao, z.raioColisao)) {
            p.hp -= z.dano;
            p.temporizadorIframe = p.duracaoIframe;
            if (p.hp <= 0) {
                p.hp   = 0;
                p.vivo = false;
                g_jogoTerminado = true;
                pausarMusicaFundo();
                tocarEfeito("Sons/gameover.mp3");
            }
            break;
        }
    }
    (void)dt;
}

// =============================================================================
//  ATUALIZAÇÃO: PARTÍCULAS E FLOATING DAMAGE
// =============================================================================

static void atualizarParticulas(float dt) {
    for (size_t i = 0; i < g_jogo.particulas.size(); ++i) {
        Particula& p = g_jogo.particulas[i];
        if (!p.ativa) continue;
        p.posicao.x    += p.velocidade.x * dt;
        p.posicao.y    += p.velocidade.y * dt;
        p.posicao.z    += p.velocidade.z * dt;
        p.velocidade.y -= 9.8f * dt;     // gravidade
        p.tempoVida    -= dt;
        p.transparencia = p.tempoVida / p.tempoVidaMaximo;
        if (p.tempoVida <= 0.0f) p.ativa = false;
    }
}

static void atualizarFloatingDamage(float dt) {
    for (size_t i = 0; i < g_jogo.numerosFlutuantes.size(); ++i) {
        FloatingDamage& fd = g_jogo.numerosFlutuantes[i];
        fd.tempoRestante        -= dt;
        fd.deslocamentoVertical += 2.0f * dt;
        fd.transparencia = fd.tempoRestante / fd.tempoTotal;
    }
}

static void atualizarProjeteisZumbi(float dt) {
    Jogador& p = g_jogo.protagonista;
    for (size_t i = 0; i < g_jogo.projeteisZumbi.size(); ++i) {
        ProjetilZumbi& pz = g_jogo.projeteisZumbi[i];
        if (!pz.ativo) continue;

        pz.posicao.x += pz.direcao.x * pz.velocidade * dt;
        pz.posicao.z += pz.direcao.z * pz.velocidade * dt;

        if (std::abs(pz.posicao.x) > ARENA_HALF || std::abs(pz.posicao.z) > ARENA_HALF) {
            pz.ativo = false;
            continue;
        }

        if (p.vivo && p.temporizadorIframe <= 0.0f) {
            float cdx = pz.posicao.x - p.posicao.x;
            float cdz = pz.posicao.z - p.posicao.z;
            float r   = p.raioColisao + 0.2f;
            if ((cdx*cdx + cdz*cdz) <= r * r) {
                pz.ativo = false;
                p.hp -= pz.dano;
                p.temporizadorIframe = p.duracaoIframe;
                if (p.hp <= 0) {
                    p.hp   = 0;
                    p.vivo = false;
                    g_jogoTerminado = true;
                    pausarMusicaFundo();
                    tocarGameOver();
                }
                }
            }
        }
    }


// =============================================================================
//  SPAWN PERIÓDICO DE ZUMBIS
// =============================================================================

// Multiplicador de spawn para um tipo de zumbi no tempo t.
// Dobra a cada minuto desde o desbloqueio; 0 = ainda não aparece.
static int multiplicadorSpawn(TipoZumbi tipo, float t) {
    float desbloqueio = 0.0f;
    switch (tipo) {
        case NORMAL:    desbloqueio =   0.0f; break;
        case RAPIDO:    desbloqueio =  60.0f; break;
        case TANK:      desbloqueio = 120.0f; break;
        case ATIRADOR:  desbloqueio = 180.0f; break;
        case EXPLOSIVO: desbloqueio = 180.0f; break;
        default:        desbloqueio =   0.0f; break;
    }
    if (t < desbloqueio) return 0;
    int minutos = (int)((t - desbloqueio) / 60.0f);
    int mult = 1;
    for (int i = 0; i < minutos; ++i) {
        mult *= 2;
        if (mult >= SPAWN_MULT_MAX) { mult = SPAWN_MULT_MAX; break; }
    }
    return mult;
}

static void atualizarSpawn(float dt) {
    if (g_jogo.jogoPausado) return;

    g_jogo.tempoSobrevivido += dt;
    g_jogo.tempoUltimoSpawn += dt;
    if (g_jogo.tempoUltimoSpawn < SPAWN_INTERVALO) return;
    g_jogo.tempoUltimoSpawn = 0.0f;

    // Conta apenas zumbis VIVOS — mortos ficam no vetor como slots reaproveitáveis.
    // horda.size() cresce sem parar se usarmos push_back sempre; por isso contamos
    // vivos para o cap e reaproveitamos slots mortos antes de alocar novos.
    int ativosNaHorda = 0;
    for (size_t s = 0; s < g_jogo.horda.size(); ++s)
        if (g_jogo.horda[s].vivo) ativosNaHorda++;

    const TipoZumbi tipos[5] = { NORMAL, RAPIDO, TANK, ATIRADOR, EXPLOSIVO };
    float t = g_jogo.tempoSobrevivido;

    for (int ti = 0; ti < 5; ++ti) {
        int mult = multiplicadorSpawn(tipos[ti], t);
        if (mult == 0) continue;
        int n = SPAWN_BASE * mult;
        for (int i = 0; i < n; ++i) {
            if (ativosNaHorda >= HORDA_MAX) break;

            // Tenta reaproveitar um slot morto para não crescer o vetor ao infinito.
            bool reutilizou = false;
            for (size_t s = 0; s < g_jogo.horda.size(); ++s) {
                if (!g_jogo.horda[s].vivo) {
                    g_jogo.horda[s] = criarZumbi(tipos[ti]);
                    reutilizou = true;
                    break;
                }
            }
            if (!reutilizou)
                g_jogo.horda.push_back(criarZumbi(tipos[ti]));

            ativosNaHorda++;
        }
        if (ativosNaHorda >= HORDA_MAX) break;
    }
}

// =============================================================================
//  ATUALIZAÇÃO: DEVORAR (corpo a corpo alto-risco/alta-recompensa, ESPAÇO)
// =============================================================================

static void atualizarDevorar(EstadoDoJogo& jogo, float dt) {
    Entidade& stand = jogo.stand;
    Jogador&  p     = jogo.protagonista;

    // -- Cooldown entre usos
    if (stand.temporizadorCooldownDevorar > 0.0f) {
        stand.temporizadorCooldownDevorar -= dt;
        if (stand.temporizadorCooldownDevorar < 0.0f)
            stand.temporizadorCooldownDevorar = 0.0f;
    }

    // -- Ativação pelo teclado (pedido registrado em cbKeyboard)
    if (stand.devorarPedido) {
        stand.devorarPedido = false;
        if (stand.temporizadorDevorar <= 0.0f &&
            stand.temporizadorCooldownDevorar <= 0.0f &&
            !stand.emSobrecarga) {
            stand.temporizadorDevorar         = DEVORAR_DURACAO_JANELA;
            stand.temporizadorCooldownDevorar = DEVORAR_COOLDOWN;
            if (DEVORAR_LUNGE > 0.0f) {
                p.posicao.x += std::cos(stand.anguloMira) * DEVORAR_LUNGE;
                p.posicao.z += std::sin(stand.anguloMira) * DEVORAR_LUNGE;
            }
        }
    }

    // -- Janela inativa: nada a fazer
    if (stand.temporizadorDevorar <= 0.0f) return;

    stand.temporizadorDevorar -= dt;
    bool janelaExpirou = (stand.temporizadorDevorar <= 0.0f);
    if (janelaExpirou) stand.temporizadorDevorar = 0.0f;

    // -- Procura o alvo mais próximo dentro do setor frontal (zumbi ou projétil)
    const float PI         = 3.14159265f;
    float meiaAb           = DEVORAR_ANGULO * 0.5f;
    Zumbi*        alvoZ    = NULL;
    ProjetilZumbi* alvoP   = NULL;
    float  menorDist       = DEVORAR_ALCANCE * 2.0f;

    for (size_t i = 0; i < jogo.horda.size(); ++i) {
        Zumbi& z = jogo.horda[i];
        if (!z.vivo) continue;
        float dx   = z.posicao.x - p.posicao.x;
        float dz   = z.posicao.z - p.posicao.z;
        float dist = std::sqrt(dx * dx + dz * dz);
        if (dist > DEVORAR_ALCANCE) continue;
        float angZ = std::atan2(dz, dx);
        float diff = angZ - stand.anguloMira;
        while (diff >  PI) diff -= 2.0f * PI;
        while (diff < -PI) diff += 2.0f * PI;
        if (diff < -meiaAb || diff > meiaAb) continue;
        if (dist < menorDist) { menorDist = dist; alvoZ = &z; alvoP = NULL; }
    }

    // Projéteis do Atirador também são devoráveis
    for (size_t i = 0; i < jogo.projeteisZumbi.size(); ++i) {
        ProjetilZumbi& pz = jogo.projeteisZumbi[i];
        if (!pz.ativo) continue;
        float dx   = pz.posicao.x - p.posicao.x;
        float dz   = pz.posicao.z - p.posicao.z;
        float dist = std::sqrt(dx * dx + dz * dz);
        if (dist > DEVORAR_ALCANCE) continue;
        float angZ = std::atan2(dz, dx);
        float diff = angZ - stand.anguloMira;
        while (diff >  PI) diff -= 2.0f * PI;
        while (diff < -PI) diff += 2.0f * PI;
        if (diff < -meiaAb || diff > meiaAb) continue;
        if (dist < menorDist) { menorDist = dist; alvoP = &pz; alvoZ = NULL; }
    }

    if (alvoZ != NULL) {
        // SUCESSO (zumbi): morte instantânea + purga tensão
        processarMorteZumbi(jogo, *alvoZ);
        stand.tensaoAtual = 0.0f;
        if (DEVORAR_SUCESSO_LIMPA_SOBRECARGA) stand.emSobrecarga = false;
        stand.temporizadorDevorar = 0.0f;
    } else if (alvoP != NULL) {
        // SUCESSO (projétil): destrói o projétil + purga tensão
        alvoP->ativo = false;
        stand.tensaoAtual = 0.0f;
        if (DEVORAR_SUCESSO_LIMPA_SOBRECARGA) stand.emSobrecarga = false;
        stand.temporizadorDevorar = 0.0f;
    } else if (janelaExpirou) {
        // ERRO: janela expirou sem acerto → sobrecarga imediata
        float maxT = maxTensaoDoNivel(jogo.protagonista.upgrades.niveis[TENSAO_UP]);
        stand.tensaoAtual  = maxT;
        stand.emSobrecarga = true;
    }
}

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

// Chão da arena
static void desenharChao() {
    const float L = ARENA_HALF;
    const int GRID = 20;
    float passo = (2.0f * L) / GRID;

    // Fundo
    glColor3f(0.08f, 0.08f, 0.10f);
    glBegin(GL_QUADS);
        glVertex3f(-L, -0.01f, -L);
        glVertex3f( L, -0.01f, -L);
        glVertex3f( L, -0.01f,  L);
        glVertex3f(-L, -0.01f,  L);
    glEnd();

    // Grade
    glColor3f(0.15f, 0.15f, 0.20f);
    glBegin(GL_LINES);
    for (int i = 0; i <= GRID; ++i) {
        float v = -L + i * passo;
        glVertex3f(v, 0.0f, -L); glVertex3f(v, 0.0f,  L);
        glVertex3f(-L, 0.0f, v); glVertex3f( L, 0.0f, v);
    }
    glEnd();

    // Borda
    glColor3f(0.5f, 0.1f, 0.1f);
    glBegin(GL_LINE_LOOP);
        glVertex3f(-L, 0.02f, -L); glVertex3f( L, 0.02f, -L);
        glVertex3f( L, 0.02f,  L); glVertex3f(-L, 0.02f,  L);
    glEnd();
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
    glLightfv(GL_LIGHT0, GL_POSITION, lpos);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  lamb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  ldif);
    glEnable(GL_LIGHT0);

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

    // --- Estado GL translúcido -----------------------------------------------
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_TEXTURE_2D);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    GLfloat lpos[4] = { 0.0f, 20.0f,  5.0f, 1.0f };
    GLfloat lamb[4] = { 0.35f, 0.35f, 0.35f, 1.0f };
    GLfloat ldif[4] = { 1.0f,  1.0f,  1.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lpos);
    glLightfv(GL_LIGHT0, GL_AMBIENT,  lamb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  ldif);

    glPushMatrix();
    glTranslatef(s.posicao.x + lungeX, floatY, s.posicao.z + lungeZ);
    glRotatef(-angGraus, 0.0f, 1.0f, 0.0f);
    glColor4f(cr, cg, 0.0f, alpha);
    glutSolidTeapot(escala);
    glPopMatrix();

    // --- Restaura estado GL --------------------------------------------------
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_LIGHTING);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

// Zumbis
static void desenharZumbis() {
    for (size_t i = 0; i < g_jogo.horda.size(); ++i) {
        const Zumbi& z = g_jogo.horda[i];
        if (!z.vivo) continue;
        float cR, cG, cB;
        obterCorBaseZumbi(z.tipo, cR, cG, cB);
        float s = z.raioColisao * 1.8f;
        desenharBloco3D(z.posicao.x, 0.0f, z.posicao.z,
                        s, s, s * 1.2f,
                        cR, cG, cB,
                        cR*0.8f, cG*0.8f, cB*0.8f,
                        cR*0.6f, cG*0.6f, cB*0.6f,
                        cR*0.9f, cG*0.9f, cB*0.9f,
                        cR*0.7f, cG*0.7f, cB*0.7f);
    }
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
        desenharCirculo3D(pz.posicao.x, 0.1f, pz.posicao.z, 0.3f,
                          1.0f, 0.55f, 0.0f);
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

static void desenharCena() {
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    configurarCamera();

    desenharMapaBalada();
    glDisable(GL_LIGHTING);   // mapa usa lighting; o resto do jogo usa glColor3f direto
    desenharZumbis();
    desenharProjeteisZumbi();
    desenharSkills();
    desenharParticulas();
    desenharHitboxDevorar();
    desenharJogador();
    desenharStand();
}

// =============================================================================
//  HUD (ortográfico 2D)
// =============================================================================

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

    // Barra de HP
    desenharBarra(10.0f, H - 30.0f, 200.0f, 18.0f,
                  (float)p.hp, (float)p.hpMaximo,
                  0.9f, 0.1f, 0.1f,   0.3f, 0.0f, 0.0f);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "HP %d/%d", p.hp, p.hpMaximo);
    desenharTexto(12.0f, H - 27.0f, buf, 1.0f, 1.0f, 1.0f);

    // Barra de Tensão
    desenharBarra(10.0f, H - 60.0f, 200.0f, 18.0f,
                  g_jogo.stand.tensaoAtual, maxTensaoDoNivel(g_jogo.protagonista.upgrades.niveis[TENSAO_UP]),
                  g_jogo.stand.emSobrecarga ? 1.0f : 0.3f,
                  g_jogo.stand.emSobrecarga ? 0.3f : 0.5f,
                  0.0f,
                  0.1f, 0.1f, 0.3f);
    std::snprintf(buf, sizeof(buf), "Tensão %.0f%%",
                  g_jogo.stand.tensaoAtual);
    desenharTexto(12.0f, H - 57.0f, buf, 1.0f, 1.0f, 1.0f);

    // Barra de XP
    desenharBarra(10.0f, H - 90.0f, 200.0f, 12.0f,
                  (float)p.xpAtual, (float)p.xpParaProximoNivel,
                  0.2f, 0.8f, 1.0f,   0.05f, 0.1f, 0.2f);
    std::snprintf(buf, sizeof(buf), "Nv %d  XP %d/%d",
                  p.nivel, p.xpAtual, p.xpParaProximoNivel);
    desenharTexto(12.0f, H - 88.0f, buf, 0.7f, 0.9f, 1.0f);

    // Status do Devorar
    {
        Entidade& st = g_jogo.stand;
        if (st.temporizadorDevorar > 0.0f) {
            desenharTexto(10.0f, H - 120.0f, "DEVORANDO!", 1.0f, 0.15f, 0.0f);
        } else if (st.temporizadorCooldownDevorar > 0.0f) {
            std::snprintf(buf, sizeof(buf), "Devorar [recarga %.1fs]",
                          st.temporizadorCooldownDevorar);
            desenharTexto(10.0f, H - 120.0f, buf, 0.55f, 0.40f, 0.40f);
        } else {
            desenharTexto(10.0f, H - 120.0f, "ESPACO: Devorar [Pronto]", 0.85f, 0.85f, 0.85f);
        }
    }

    // Temporizador (MM:SS) — canto superior direito
    int t_total = (int)g_jogo.tempoSobrevivido;
    int t_min   = t_total / 60;
    int t_sec   = t_total % 60;
    std::snprintf(buf, sizeof(buf), "%02d:%02d", t_min, t_sec);
    desenharTexto(W - 70.0f, H - 20.0f, buf, 1.0f, 1.0f, 0.6f);

    // Contador de kills — abaixo do timer
    std::snprintf(buf, sizeof(buf), "Kills: %d", g_kills);
    desenharTexto(W - 80.0f, H - 45.0f, buf, 1.0f, 0.5f, 0.5f);

    // Skills equipadas (slots)
    float sx = 10.0f, sy = 10.0f;
    for (int i = 0; i < MAX_SLOTS_SKILL; ++i) {
        const SlotSkill& sl = g_skills.slot(i);
        if (!sl.ativo) continue;
        const char* nome = SkillFactory::nomeDe(sl.build.id);
        std::snprintf(buf, sizeof(buf), "[%d] %s", i+1, nome);
        desenharTexto(sx, sy, buf, 0.6f, 1.0f, 0.6f);
        sy += 22.0f;
    }

    // Sobrecarga
    if (g_jogo.stand.emSobrecarga) {
        desenharTexto(W * 0.5f - 80.0f, H * 0.5f + 60.0f,
                      "SOBRECARGA!", 1.0f, 0.2f, 0.0f);
    }

    // Game Over
    if (g_jogoTerminado) {
        desenharTexto(W * 0.5f - 80.0f, H * 0.5f,
                      "GAME OVER — [R] Reiniciar", 1.0f, 0.2f, 0.2f);
    }

    sairModo2D();
}

// =============================================================================
//  MENU DE LEVEL-UP
// =============================================================================

static void desenharMenuLevelUp() {
    if (!g_jogo.pausadoParaUpgrade) return;

    entrarModo2D();

    // Fundo semi-transparente
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    const float W = (float)g_winW, H = (float)g_winH;
    glColor4f(0.0f, 0.0f, 0.0f, 0.65f);
    glBegin(GL_QUADS);
        glVertex2f(0, 0); glVertex2f(W, 0);
        glVertex2f(W, H); glVertex2f(0, H);
    glEnd();
    glDisable(GL_BLEND);

    desenharTexto(W * 0.5f - 80.0f, H - 120.0f,
                  "LEVEL UP! Escolha uma melhoria:", 1.0f, 0.9f, 0.2f);

    MenuLevelUp& menu = g_jogo.menuAtual;
    float cardW = 260.0f, cardH = 100.0f;
    float totalW = menu.quantidade * cardW + (menu.quantidade - 1) * 20.0f;
    float startX = (W - totalW) * 0.5f;
    float startY = H * 0.5f - cardH * 0.5f;

    for (int i = 0; i < menu.quantidade; ++i) {
        LevelUpChoice& c = menu.escolhas[i];
        float cx = startX + i * (cardW + 20.0f);

        // Cor da raridade
        float rR, rG, rB;
        corRaridade(c.raridade, rR, rG, rB);

        // Fundo do card
        glColor3f(0.1f, 0.1f, 0.15f);
        glBegin(GL_QUADS);
            glVertex2f(cx, startY);
            glVertex2f(cx+cardW, startY);
            glVertex2f(cx+cardW, startY+cardH);
            glVertex2f(cx, startY+cardH);
        glEnd();
        // Borda colorida
        glColor3f(rR, rG, rB);
        glBegin(GL_LINE_LOOP);
            glVertex2f(cx, startY);
            glVertex2f(cx+cardW, startY);
            glVertex2f(cx+cardW, startY+cardH);
            glVertex2f(cx, startY+cardH);
        glEnd();

        // Número da tecla
        char buf[4];
        std::snprintf(buf, sizeof(buf), "[%d]", i+1);
        desenharTexto(cx + 8.0f, startY + cardH - 22.0f, buf, rR, rG, rB);

        // Descrição e subtítulo
        desenharTexto(cx + 8.0f, startY + 60.0f,
                      c.descricao, 1.0f, 1.0f, 1.0f);
        desenharTexto(cx + 8.0f, startY + 35.0f,
                      c.subtitulo, 0.7f, 0.7f, 0.8f);
    }

    sairModo2D();
}

// =============================================================================
//  PROJEÇÃO DO MOUSE NO MUNDO
// =============================================================================

static Vetor3D projetarMouseNoMundo(int mx, int my) {
    // Inversão de Y (OpenGL: y=0 no fundo, GLUT: y=0 no topo)
    int viewport[4];
    double model[16], proj[16];
    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetDoublev(GL_MODELVIEW_MATRIX, model);
    glGetDoublev(GL_PROJECTION_MATRIX, proj);

    double winY = viewport[3] - my;
    float depth;
    glReadPixels(mx, (int)winY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

    double wx, wy, wz;
    gluUnProject(mx, winY, depth, model, proj, viewport, &wx, &wy, &wz);

    Vetor3D v;
    v.x = (float)wx; v.y = 0.0f; v.z = (float)wz;
    return v;
}

// =============================================================================
//  CALLBACKS GLUT
// =============================================================================

static void cbDisplay() {
    desenharCena();
    desenharHUD();
    desenharMenuLevelUp();
    glutSwapBuffers();
}

static float g_ultimoTempo = 0.0f;

static void cbIdle() {
    float agora = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float dt    = agora - g_ultimoTempo;
    g_ultimoTempo = agora;
    if (dt > 0.05f) dt = 0.05f;   // cap de 50ms para evitar pulos grandes

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
        g_jogo.houveMorteRecente = false;   // resetar hook KILLEDENEMY
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
                    g_jogo.jogoPausado        = false;
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

        // Sair
        case 27:   // ESC
            std::exit(0);
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

static void cbReshape(int w, int h) {
    g_winW = w;
    g_winH = h;
    glViewport(0, 0, w, h);
}

// =============================================================================
//  STUB: aplicarEscolhaMenu (declarado em SkillInventory.h — implementado aqui)
// =============================================================================

void aplicarEscolhaMenu(EstadoDoJogo& jogo, int indiceEscolha) {
    if (indiceEscolha < 0 || indiceEscolha >= jogo.menuAtual.quantidade) return;
    aplicarEscolhaLevelUp(jogo.menuAtual.escolhas[indiceEscolha],
                          jogo.inventario,
                          jogo.protagonista.upgrades,
                          g_skills,
                          jogo.protagonista,
                          jogo.arquetipoArma);
    jogo.pausadoParaUpgrade = false;
    jogo.jogoPausado        = false;
}

// =============================================================================
//  MAIN
// =============================================================================

int main(int argc, char** argv) {
    std::srand((unsigned int)std::time(NULL));

    // 1. GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(JANELA_W, JANELA_H);
    glutCreateWindow("Stand — Fase 6.5");

    // 1.5. Carrega o modelo da Sofia (precisa de contexto OpenGL ativo)
    g_sofiaCarregada = g_sofia.carregar("Sofia.glb");
    if (g_sofiaCarregada) {
        g_sofia.definirTexturaManual("Sofia.png");
        g_sofia.configurarRootMotion(true, "mixamorig:Hips");
        g_sofia.definirOssoMao("mixamorig:RightHand"); // bone socket da pistola
        g_sofia.listarOssos(); // lista ossos no console — confirme o nome; ajuste definirOssoMao se preciso
        // Toca a primeira animação encontrada no modelo
        const std::map<std::string,int>& anims = g_sofia.animacoesDisponiveis();
        if (!anims.empty()) {
            g_sofia.tocarAnimacao(anims.begin()->first, true);
            g_sofia.congelarNoFrame(14); // Começa parada no frame 1
        }
    }
    g_pistolaCarregada = g_pistola.carregar("pistola.glb");

    // 2. Catálogo de skills
    registrarSkillsPadrao();
    registrarArmasInteligentes();   // 3 armas inteligentes (builds por nível)

    // 3. Estado do jogo e build inicial
    inicializarEstadoJogo();

    // 4. Callbacks
    glutDisplayFunc(cbDisplay);
    glutIdleFunc(cbIdle);
    glutMouseFunc(cbMouse);
    glutMotionFunc(cbMotion);
    glutPassiveMotionFunc(cbPassiveMotion);
    glutKeyboardFunc(cbKeyboard);
    glutKeyboardUpFunc(cbKeyboardUp);
    glutReshapeFunc(cbReshape);

    // 5. OpenGL
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
    glLineWidth(1.5f);

    g_ultimoTempo = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

    // 6. Áudio
    tocarMusicaFundo("Sons/musica_balada.mp3"); // Caminho atualizado!

    g_ultimoTempo = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

    glutMainLoop();
    return 0;
}