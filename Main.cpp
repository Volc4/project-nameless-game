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
//#include "GameLogic.h"

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

#include "GameConfig.h"

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
static bool g_emMenuInicial = true;   // true enquanto o menu principal estiver ativo
static int  g_menuOpcao     = 0;      // 0 = Iniciar Jogo, 1 = Sair
static int  g_kills         = 0;

// Falas da protagonista — volume e flags de disparo único (reset em inicializarEstadoJogo)
static const int VOLUME_FALA       = 200;
static bool g_faladaMagrelas       = false;
static bool g_faladaCarecas        = false;
static bool g_faladaFestaZumbi     = false;

// --- Modelo da protagonista Sofia -------------------------------------------
static ModeloAnimado g_sofia;
static bool          g_sofiaCarregada = false;

// --- Pistola (prop estático, bone socket na mão da Sofia) -------------------
static ModeloAnimado g_pistola;
static bool          g_pistolaCarregada = false;

// --- Modelo do Boss (Cabeça.glb) -------------------------------------------
static ModeloAnimado g_cabeca;
static bool          g_cabecaCarregada = false;

// --- Modelo dos Zumbis (Zumbi.glb) -----------------------------------------
static ModeloAnimado g_zumbi;
static bool          g_zumbiCarregado  = false;
static std::string   g_zumbiNomeAnim;

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
static void desenharPause();
static void desenharHitboxDevorar();
static void desenharProjeteisZumbi();
static Vetor3D projetarMouseNoMundo(int mx, int my);
// --- Boss -------------------------------------------------------------------
static void invocarBoss();
static void atualizarBoss(float dt);
static void desenharBoss(const Zumbi& z);

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

    // =========================================================================
    // BOSS MORREU — tratamento especial antes de qualquer lógica de zumbi comum
    // =========================================================================
    if (z.ehBoss) {
        tocarEfeito("Sons/BossMorte.mp3");
        // Explosão visual em duas ondas de partículas (roxo + dourado)
        criarParticulasMorte(jogo, z.posicao, 0.55f, 0.0f, 1.0f);
        criarParticulasMorte(jogo, z.posicao, 1.0f,  0.8f, 0.0f);

        // Grande recompensa de XP
        jogo.protagonista.xpAtual += BOSS_XP_RECOMPENSA;

        // Transição de fase: Boss derrotado → Vitória
        jogo.fasePartida      = FASE_VITORIA;
        jogo.tempMensagemBoss = BOSS_MSG_DURACAO;

        // Verificar level-up acumulado pela XP do Boss
        while (jogo.protagonista.xpAtual >= jogo.protagonista.xpParaProximoNivel) {
            jogo.protagonista.xpAtual -= jogo.protagonista.xpParaProximoNivel;
            jogo.protagonista.xpParaProximoNivel =
                (int)(jogo.protagonista.xpParaProximoNivel * 1.4f);
            jogo.protagonista.nivel++;
            sortearRecompensas(jogo.menuAtual, jogo.inventario,
                               jogo.arquetipoArma, jogo.protagonista.upgrades);
            jogo.pausadoParaUpgrade = true;
            jogo.jogoPausado        = true;
        }
        return; // não executa a lógica de zumbi comum abaixo
    }
    // =========================================================================

    tocarMorteZumbi();

    // XP por tipo (×10 temporário para teste de arquétipos)
    int xp = 1;
    switch (z.tipo) {
        case RAPIDO:    xp = 10; break;
        case TANK:      xp = 50; break;
        case ATIRADOR:  xp = 300; break;
        case EXPLOSIVO: xp = 500; break;
        default:        xp = 2; break;
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
                tocarEfeitoComVolume("Sons/Ai.mp3", VOLUME_FALA);
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

#include "DrawPrimitives3D.h"

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
    g_jogo.protagonista.duracaoIframe      = 0.80f;
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
    g_jogo.pausaManual            = false;
    g_jogo.houveMorteRecente      = false;
    g_faladaMagrelas   = false;
    g_faladaCarecas    = false;
    g_faladaFestaZumbi = false;

    // --- Sistema de Boss ---
    g_jogo.fasePartida        = FASE_NORMAL;
    g_jogo.tempMensagemBoss   = 0.0f;
    g_jogo.bossJaFoiInvocado  = false;
    g_jogo.tempoEntradaBoss   = 0.0f;
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

#include "GameUpdate.h"

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
            tocarEfeitoComVolume("Sons/Ai.mp3", VOLUME_FALA);
            p.temporizadorIframe = p.duracaoIframe;
            if (p.hp <= 0) {
                p.hp   = 0;
                p.vivo = false;
                g_jogoTerminado = true;
                pausarMusicaFundo();
                tocarGameOver();
            }
            break;
        }
    }
    (void)dt;
}


#include "RenderCena.h"


// =============================================================================
//  HUD (ortográfico 2D)
// =============================================================================

#include "TextoGL.h"

// ---------------------------------------------------------------------------
// Primitivas 2D reutilizáveis no HUD
// ---------------------------------------------------------------------------
#include "Hud.h"


#include "MenuUI.h"
#include "InputGLUT.h"


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
        // Toca a primeira animação encontrada no modelo
        const std::map<std::string,int>& anims = g_sofia.animacoesDisponiveis();
        if (!anims.empty()) {
            g_sofia.tocarAnimacao(anims.begin()->first, true);
            g_sofia.congelarNoFrame(14); // Começa parada no frame 1
        }
    }
    g_pistolaCarregada = g_pistola.carregar("pistola.glb");

    // Boss — Cabeça.glb com root motion removido; toca 1ª animação em loop.
    // O nó raiz animado é detectado automaticamente pelo heurístico
    // "primeiro osso com canal" (_rootBoneEncontrado), então não é preciso
    // saber o nome exato do bone raiz do arquivo.
    g_cabecaCarregada = g_cabeca.carregar("Cabeça.glb");
    if (g_cabecaCarregada) {
        g_cabeca.configurarRootMotion(true); // zera translação do root bone (in-place)
        const std::map<std::string,int>& animsBoss = g_cabeca.animacoesDisponiveis();
        if (!animsBoss.empty())
            g_cabeca.tocarAnimacao(animsBoss.begin()->first, true);
    }

    g_zumbiCarregado = g_zumbi.carregar("Zumbi.glb");
    if (g_zumbiCarregado) {
        g_zumbi.configurarRootMotion(true);
        const std::map<std::string,int>& animsZ = g_zumbi.animacoesDisponiveis();
        if (!animsZ.empty()) {
            g_zumbiNomeAnim = animsZ.begin()->first;
            g_zumbi.tocarAnimacao(g_zumbiNomeAnim, true);
        }
    }

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
    glutSpecialFunc(cbSpecial);
    glutReshapeFunc(cbReshape);

    // 5. OpenGL
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
    glLineWidth(1.5f);

    g_ultimoTempo = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

    // 6. Áudio — começa com a música do menu
    tocarMusicaFundo("Sons/Menu.mp3");

    g_ultimoTempo = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

    glutMainLoop();
    return 0;
}
