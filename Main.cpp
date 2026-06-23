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

static const float ARENA_HALF   = 150.0f;   // metade do lado da arena quadrada
static const float CAM_DIST     = 30.0f;    // distância da câmera ao ponto focal
static const float CAM_ANGLE_X  = 45.0f;   // inclinação da câmera (graus)
static const int   JANELA_W     = 1024;
static const int   JANELA_H     = 768;

// Constante exigida por SkillExecutor.h (extern)
const float FATOR_DANO_SOBRECARGA = 2.5f;

// =============================================================================
//  ESTADO GLOBAL
// =============================================================================

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

// =============================================================================
//  PROTÓTIPOS INTERNOS
// =============================================================================

static void inicializarEstadoJogo();
static void atualizarJogador(float dt);
static void atualizarZumbis(float dt);
static void spawnarZumbi();
static void atualizarParticulas(float dt);
static void atualizarFloatingDamage(float dt);
static void desenharCena();
static void desenharHUD();
static void desenharMenuLevelUp();
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

    // XP por tipo
    int xp = 5;
    switch (z.tipo) {
        case RAPIDO:    xp =  8; break;
        case TANK:      xp = 15; break;
        case ATIRADOR:  xp = 12; break;
        case EXPLOSIVO: xp = 10; break;
        default:        xp =  5; break;
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
    g_jogo.protagonista.duracaoIframe      = 1.0f;
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

    // --- Horda / Gemas / Partículas ---
    g_jogo.horda.clear();
    g_jogo.gemas.clear();
    g_jogo.particulas.clear();
    g_jogo.numerosFlutuantes.clear();

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

    // Spawn na borda da arena
    int lado = rand() % 4;
    float pos = ((rand() % 2000) / 1000.0f - 1.0f) * ARENA_HALF;
    switch (lado) {
        case 0: z.posicao.x = -ARENA_HALF; z.posicao.y = 0.0f; z.posicao.z = pos; break;
        case 1: z.posicao.x =  ARENA_HALF; z.posicao.y = 0.0f; z.posicao.z = pos; break;
        case 2: z.posicao.x = pos; z.posicao.y = 0.0f; z.posicao.z = -ARENA_HALF; break;
        default:z.posicao.x = pos; z.posicao.y = 0.0f; z.posicao.z =  ARENA_HALF; break;
    }

    switch (tipo) {
        case RAPIDO:
            z.velocidade  = 5.0f; z.raioColisao = 0.4f; z.vida = 3;  break;
        case TANK:
            z.velocidade  =  0.8f; z.raioColisao = 0.9f; z.vida = 20; break;
        case ATIRADOR:
            z.velocidade  =  2.0f; z.raioColisao = 0.5f; z.vida = 6;  break;
        case EXPLOSIVO:
            z.velocidade  =  2.0f; z.raioColisao = 0.6f; z.vida = 5;  break;
        default:
            z.velocidade  =  1.0f; z.raioColisao = 0.5f; z.vida = 5;  break;
    }
    return z;
}

static void spawnarZumbi() {
    TipoZumbi tipo = NORMAL;
    int r = rand() % 100;
    float t = g_jogo.tempoSobrevivido;
    if (t > 120.0f && r < 10) tipo = EXPLOSIVO;
    else if (t > 60.0f && r < 15) tipo = TANK;
    else if (t > 30.0f && r < 20) tipo = ATIRADOR;
    else if (r < 25) tipo = RAPIDO;

    g_jogo.horda.push_back(criarZumbi(tipo));
}

// =============================================================================
//  ATUALIZAÇÃO: JOGADOR
// =============================================================================

static void atualizarJogador(float dt) {
    Jogador& p = g_jogo.protagonista;
    if (!p.vivo) return;

    // Movimento WASD
    float dx = 0.0f, dz = 0.0f;
    if (g_teclaW) dz -= 1.0f;
    if (g_teclaS) dz += 1.0f;
    if (g_teclaA) dx -= 1.0f;
    if (g_teclaD) dx += 1.0f;

    float len = std::sqrt(dx*dx + dz*dz);
    if (len > 0.0001f) {
        dx /= len; dz /= len;
        p.posicao.x += dx * p.velocidade * dt;
        p.posicao.z += dz * p.velocidade * dt;
        // Limite da arena
        if (p.posicao.x >  ARENA_HALF) p.posicao.x =  ARENA_HALF;
        if (p.posicao.x < -ARENA_HALF) p.posicao.x = -ARENA_HALF;
        if (p.posicao.z >  ARENA_HALF) p.posicao.z =  ARENA_HALF;
        if (p.posicao.z < -ARENA_HALF) p.posicao.z = -ARENA_HALF;
    }

    // Stand segue o jogador com offset em direção ao cursor
    Vetor3D dir = obterDirecaoNormalizada(p.posicao, g_posicaoCursor);
    g_jogo.stand.posicao.x = p.posicao.x + dir.x * 1.5f;
    g_jogo.stand.posicao.z = p.posicao.z + dir.z * 1.5f;
    g_jogo.stand.anguloMira = std::atan2(dir.z, dir.x);

    // Iframe
    if (p.temporizadorIframe > 0.0f)
        p.temporizadorIframe -= dt;

    // Coleta de gemas
    for (size_t i = 0; i < g_jogo.gemas.size(); ++i) {
        GemaXP& g = g_jogo.gemas[i];
        if (g.coletada) continue;
        float d2 = calcularDistanciaQuadrada(p.posicao, g.posicao);
        if (d2 < 2.5f * 2.5f) {
            g.coletada = true;
            // XP já creditado em processarMorteZumbi; a gema é só visual.
        }
    }

    // Sobrecarga de tensão
    if (g_jogo.stand.tensaoAtual >= 100.0f && !g_jogo.stand.emSobrecarga) {
        g_jogo.stand.emSobrecarga = true;
        g_jogo.stand.tensaoAtual  = 100.0f;
    }
    if (g_jogo.stand.emSobrecarga) {
        g_jogo.stand.tensaoAtual -= 20.0f * dt; // drena durante sobrecarga
        if (g_jogo.stand.tensaoAtual <= 0.0f) {
            g_jogo.stand.tensaoAtual  = 0.0f;
            g_jogo.stand.emSobrecarga = false;
        }
    }

    // Disparo contínuo por clique com cooldown
static float s_cooldownDisparo = 0.0f;
s_cooldownDisparo -= dt;

if (g_cliqueMouse && !g_jogo.stand.emSobrecarga && !g_jogo.jogoPausado) {
    g_jogo.atirandoAgora = true;
    if (s_cooldownDisparo <= 0.0f) {
        g_skills.executar(g_jogo, g_posicaoCursor);
        s_cooldownDisparo = 0.3f;  // dispara a cada 0.3 segundos
    }
} else {
    g_jogo.atirandoAgora = false;
}
}

// =============================================================================
//  ATUALIZAÇÃO: ZUMBIS (IA simples WANDER→CHASE)
// =============================================================================

static void atualizarZumbis(float dt) {
    Jogador& p = g_jogo.protagonista;

    for (size_t i = 0; i < g_jogo.horda.size(); ++i) {
        Zumbi& z = g_jogo.horda[i];
        if (!z.vivo) continue;

        // Sempre persegue o jogador (IA CHASE)
        Vetor3D dir = obterDirecaoNormalizada(z.posicao, p.posicao);
        z.posicao.x += dir.x * z.velocidade * dt;
        z.posicao.z += dir.z * z.velocidade * dt;

        // Limites da arena
        if (z.posicao.x >  ARENA_HALF) z.posicao.x =  ARENA_HALF;
        if (z.posicao.x < -ARENA_HALF) z.posicao.x = -ARENA_HALF;
        if (z.posicao.z >  ARENA_HALF) z.posicao.z =  ARENA_HALF;
        if (z.posicao.z < -ARENA_HALF) z.posicao.z = -ARENA_HALF;
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
            p.hp--;
            p.temporizadorIframe = p.duracaoIframe;
            if (p.hp <= 0) {
                p.hp   = 0;
                p.vivo = false;
                g_jogoTerminado = true;
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

// =============================================================================
//  SPAWN PERIÓDICO DE ZUMBIS
// =============================================================================

static void atualizarSpawn(float dt) {
    if (g_jogo.jogoPausado) return;

    g_jogo.tempoSobrevivido += dt;
    g_jogo.tempoUltimoSpawn += dt;

    // Cooldown diminui com o tempo (fica mais difícil)
    float cooldown = 2.5f - g_jogo.tempoSobrevivido * 0.005f;
    if (cooldown < 0.5f) cooldown = 0.5f;

    if (g_jogo.tempoUltimoSpawn >= cooldown) {
        g_jogo.tempoUltimoSpawn = 0.0f;
        int n = 1 + (int)(g_jogo.tempoSobrevivido / 30.0f);
        if (n > 5) n = 5;
        for (int i = 0; i < n; ++i) spawnarZumbi();
    }
}

// =============================================================================
//  DESENHO DE CENA (OPENGL)
// =============================================================================

// Câmera isométrica seguindo o jogador
static void configurarCamera() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)JANELA_W / JANELA_H, 0.5, 600.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    Vetor3D& p = g_jogo.protagonista.posicao;
    float eyeX = p.x + CAM_DIST * std::cos(CAM_ANGLE_X * 3.14159265f / 180.0f);
    float eyeY = CAM_DIST * std::sin(CAM_ANGLE_X * 3.14159265f / 180.0f) * 1.5f;
    float eyeZ = p.z + CAM_DIST;
    gluLookAt(eyeX, eyeY, eyeZ,
              p.x,   1.0f,  p.z,
              0.0,   1.0,   0.0);
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

// Jogador (cubo branco-azulado)
static void desenharJogador() {
    Jogador& p = g_jogo.protagonista;
    if (!p.vivo) return;
    float flash = (p.temporizadorIframe > 0.0f) ? 0.5f : 1.0f;
    desenharBloco3D(p.posicao.x, 0.0f, p.posicao.z,
                    0.8f, 0.8f, 1.2f,
                    0.9f*flash, 0.9f*flash, 1.0f*flash,
                    0.7f*flash, 0.7f*flash, 0.9f*flash,
                    0.5f*flash, 0.5f*flash, 0.7f*flash,
                    0.8f*flash, 0.8f*flash, 1.0f*flash,
                    0.6f*flash, 0.6f*flash, 0.8f*flash);
}

// Stand (cubo dourado menor)
static void desenharStand() {
    Entidade& s = g_jogo.stand;
    bool sob = s.emSobrecarga;
    float r = sob ? 1.0f : 0.9f;
    float g = sob ? 0.3f : 0.7f;
    float b = sob ? 0.0f : 0.0f;
    desenharBloco3D(s.posicao.x, 0.0f, s.posicao.z,
                    0.6f, 0.6f, 0.9f,
                    r, g, b,  r*0.8f, g*0.8f, b,
                    r*0.6f, g*0.6f, b,  r*0.9f, g*0.9f, b,  r*0.7f, g*0.7f, b);
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

// Gemas de XP
static void desenharGemas() {
    for (size_t i = 0; i < g_jogo.gemas.size(); ++i) {
        const GemaXP& g = g_jogo.gemas[i];
        if (g.coletada) continue;
        desenharCirculo3D(g.posicao.x, 0.05f, g.posicao.z, 0.25f,
                          0.1f, 0.9f, 0.5f);
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

    desenharChao();
    desenharGemas();
    desenharZumbis();
    desenharSkills();
    desenharParticulas();
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
    gluOrtho2D(0.0, JANELA_W, 0.0, JANELA_H);
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

    // Barra de HP
    desenharBarra(10.0f, JANELA_H - 30.0f, 200.0f, 18.0f,
                  (float)p.hp, (float)p.hpMaximo,
                  0.9f, 0.1f, 0.1f,   0.3f, 0.0f, 0.0f);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "HP %d/%d", p.hp, p.hpMaximo);
    desenharTexto(12.0f, JANELA_H - 27.0f, buf, 1.0f, 1.0f, 1.0f);

    // Barra de Tensão
    desenharBarra(10.0f, JANELA_H - 60.0f, 200.0f, 18.0f,
                  g_jogo.stand.tensaoAtual, 100.0f,
                  g_jogo.stand.emSobrecarga ? 1.0f : 0.3f,
                  g_jogo.stand.emSobrecarga ? 0.3f : 0.5f,
                  0.0f,
                  0.1f, 0.1f, 0.3f);
    std::snprintf(buf, sizeof(buf), "Tensão %.0f%%",
                  g_jogo.stand.tensaoAtual);
    desenharTexto(12.0f, JANELA_H - 57.0f, buf, 1.0f, 1.0f, 1.0f);

    // Barra de XP
    desenharBarra(10.0f, JANELA_H - 90.0f, 200.0f, 12.0f,
                  (float)p.xpAtual, (float)p.xpParaProximoNivel,
                  0.2f, 0.8f, 1.0f,   0.05f, 0.1f, 0.2f);
    std::snprintf(buf, sizeof(buf), "Nv %d  XP %d/%d",
                  p.nivel, p.xpAtual, p.xpParaProximoNivel);
    desenharTexto(12.0f, JANELA_H - 88.0f, buf, 0.7f, 0.9f, 1.0f);

    // Tempo
    std::snprintf(buf, sizeof(buf), "%.1fs", g_jogo.tempoSobrevivido);
    desenharTexto(JANELA_W - 80.0f, JANELA_H - 28.0f, buf, 0.8f, 0.8f, 0.8f);

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
        desenharTexto(JANELA_W * 0.5f - 80.0f, JANELA_H * 0.5f + 60.0f,
                      "SOBRECARGA!", 1.0f, 0.2f, 0.0f);
    }

    // Game Over
    if (g_jogoTerminado) {
        desenharTexto(JANELA_W * 0.5f - 80.0f, JANELA_H * 0.5f,
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
    glColor4f(0.0f, 0.0f, 0.0f, 0.65f);
    glBegin(GL_QUADS);
        glVertex2f(0, 0); glVertex2f(JANELA_W, 0);
        glVertex2f(JANELA_W, JANELA_H); glVertex2f(0, JANELA_H);
    glEnd();
    glDisable(GL_BLEND);

    desenharTexto(JANELA_W * 0.5f - 80.0f, JANELA_H - 120.0f,
                  "LEVEL UP! Escolha uma melhoria:", 1.0f, 0.9f, 0.2f);

    MenuLevelUp& menu = g_jogo.menuAtual;
    float cardW = 260.0f, cardH = 100.0f;
    float totalW = menu.quantidade * cardW + (menu.quantidade - 1) * 20.0f;
    float startX = (JANELA_W - totalW) * 0.5f;
    float startY = JANELA_H * 0.5f - cardH * 0.5f;

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
        atualizarZumbis(dt);
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
                g_skills.limparTodos();
                inicializarEstadoJogo();
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

    glutMainLoop();
    return 0;
}