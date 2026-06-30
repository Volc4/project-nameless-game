#ifndef GAMECONFIG_H
#define GAMECONFIG_H

// =============================================================================
//  GameConfig.h — Constantes e tunables globais do jogo (extraído de Main.cpp)
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
static const int   HORDA_MAX          = 40;     // cap total de zumbis ativos
static const float SPAWN_RAIO_MIN     = 32.0f; // distância mínima do jogador no spawn
static const float SPAWN_RAIO_MAX     = 45.0f; // distância máxima do jogador no spawn (fora da tela)
static const float ATIRADOR_DIST      = 15.0f; // distância preferida do Atirador
static const float ATIRADOR_COOLDOWN  = 2.5f;  // s entre disparos do Atirador
static const float PROJETIL_ZUMBI_VEL = 12.0f; // velocidade dos projéteis
static const int   PROJETIL_ZUMBI_DANO = 1;    // dano dos projéteis
static const float EXPLOSAO_RAIO      = 3.0f;  // raio da explosão do Explosivo

// ===========================================================================
// CONSTANTES DO SISTEMA DE BOSS
//   Centralizadas aqui para facilitar balanceamento sem tocar na lógica.
//   Ajuste apenas estes valores para rebalancear o Boss.
// ===========================================================================
static const int   BOSS_VIDA_MAXIMA     = 5000;  // HP total do Boss
static const int   BOSS_DANO           = 5;      // dano por toque no jogador
static const float BOSS_VELOCIDADE     = 8.0f;   // 80 % da velocidade do RAPIDO (10.0f)
static const float BOSS_RAIO_COLISAO   = 2.5f;   // aprox. 5x maior que um zumbi NORMAL
static const float BOSS_DIST_SPAWN     = 40.0f;  // distância mínima do jogador no spawn
static const float BOSS_TEMPO_TRIGGER  = 240.0f; // 4 min: momento em que o spawn é bloqueado
static const int   BOSS_XP_RECOMPENSA  = 2000;   // XP concedido ao derrotar o Boss
static const float BOSS_MSG_DURACAO    = 5.0f;   // duração das mensagens na tela (segundos)
static const float BOSS_ENTRADA_DURACAO  = 2.5f;  // duração do efeito visual de entrada
static const float BOSS_TIRO_COOLDOWN   = 1.2f;  // s entre rajadas do Boss
static const int   BOSS_TIRO_COUNT      = 12;    // projéteis por rajada (círculo completo)
static const float BOSS_PROJETIL_VEL    = 18.0f; // velocidade dos projéteis do Boss
static const int   BOSS_PROJETIL_DANO   = 2;     // dano por projétil do Boss

static const float ARENA_HALF   = 150.0f;   // metade do lado da arena quadrada
static const float CAM_DIST     = 30.0f;    // distância da câmera ao ponto focal
static const float CAM_ANGLE_X  = 45.0f;   // inclinação da câmera (graus)
static const int   JANELA_W     = 1024;
static const int   JANELA_H     = 768;

// Calibrado para casar altura aparente com a Sofia (malha do Zumbi ~4.43x menor).
// Derivado de SOFIA_ESCALA * (alturaSofia/alturaZumbi) medidos via debugImprimirExtensao:
//   alturaSofia = 5.322 - 0.458 = 4.864  |  alturaZumbi = 0.244 - (-0.853) = 1.097
//   ZUMBI_ESCALA = 0.008 * (4.864 / 1.097) ≈ 0.0355
static float         ZUMBI_ESCALA      = 4.0f;
static const float   ZUMBI_ROT_OFFSET  = 90.0f;
// Pivot do Zumbi está no quadril (ymin=-0.853). Compensa para pés no chão:
//   Y_OFFSET = SOFIA_Y_OFFSET - (ymin_zumbi * ZUMBI_ESCALA) + (ymin_sofia * SOFIA_ESCALA)
//            = 2.0 - (-0.853*0.0355) + (0.458*0.008) ≈ 2.034 → arredondado para 2.03
static float         ZUMBI_Y_OFFSET    = 3.5f;
// Escala e offset Y do modelo — ajuste fino após ver o resultado em jogo.
static const float   BOSS_ESCALA    = 4.3f;
static const float   BOSS_MODELO_Y  = 3.87f;   // positivo → sobe; negativo → desce
static const float   BOSS_ROT_OFFSET = 90.0f;  // offset Y (Mixamo Z-up → OpenGL Y-up)

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

#endif // GAMECONFIG_H
