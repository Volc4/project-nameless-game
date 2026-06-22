#ifndef ENTITIES_H
#define ENTITIES_H
#include <vector>

// ---------------------------------------------------------------------------
// Estrutura matemática básica para posições no mundo 3D
// ---------------------------------------------------------------------------
struct Vetor3D {
    float x, y, z;
};

// ---------------------------------------------------------------------------
// Tipos de upgrade disponíveis ao jogador.
// Cada valor mapeia diretamente ao índice no array de níveis da struct
// SistemaUpgrades, tornando aplicação e leitura O(1) e sem switch.
// ---------------------------------------------------------------------------
enum TipoUpgrade {
    DANO       = 0,
    CADENCIA   = 1,
    PERFURACAO = 2,
    TENSAO_UP  = 3,
    VELOCIDADE = 4,
    VIDA       = 5,
    TOTAL_UPGRADES = 6
};

// (enum TipoDisparo REMOVIDO — sistema legado substituído pelo motor data-driven)

// ---------------------------------------------------------------------------
// (struct Projetil REMOVIDO — substituído por RuntimeSkill em SkillTypes.h,
//  a entidade ofensiva única do motor data-driven.)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Sistema de upgrades do jogador.
// ---------------------------------------------------------------------------
struct SistemaUpgrades {
    int niveis[TOTAL_UPGRADES];
};

// ---------------------------------------------------------------------------
// Dados da Protagonista (Sobrevivente)
// ---------------------------------------------------------------------------
struct Jogador {
    Vetor3D posicao;
    float velocidade;
    float raioColisao;
    bool emDash;
    bool vivo;

    int hp;
    int hpMaximo;
    float temporizadorIframe;
    float duracaoIframe;

    int xpAtual;
    int xpParaProximoNivel;
    int nivel;

    SistemaUpgrades upgrades;
};

// ---------------------------------------------------------------------------
// Dados da Entidade (O "Stand")
// ---------------------------------------------------------------------------
struct Entidade {
    Vetor3D posicao;
    float anguloMira;
    float tensaoAtual;

    bool emSobrecarga;

    bool parryAtivo;
    float temporizadorParry;
    float cooldownParry;
    float temporizadorCooldown;

    bool parryBemSucedido;
    float temporizadorFeedback;
};

struct GemaXP {
    Vetor3D posicao;
    int valorXP;
    bool coletada;
};

// ---------------------------------------------------------------------------
// Particula — puramente visual
// ---------------------------------------------------------------------------
struct Particula {
    Vetor3D posicao;
    Vetor3D velocidade;
    float tempoVida;
    float tempoVidaMaximo;
    float corR, corG, corB;
    float tamanho;
    float transparencia;
    bool ativa;
};

// ---------------------------------------------------------------------------
// FloatingDamage — puramente visual
// ---------------------------------------------------------------------------
struct FloatingDamage {
    Vetor3D posicao;
    int valorDano;
    float tempoRestante;
    float tempoTotal;
    float deslocamentoVertical;
    float corR, corG, corB;
    float transparencia;
};

// Tipos de inimigos
enum TipoZumbi { NORMAL, RAPIDO, TANK, ATIRADOR, EXPLOSIVO };

// Estados da IA (FSM)
enum EstadoIA { WANDER, CHASE };

// Dados do Zumbi
struct Zumbi {
    Vetor3D posicao;
    TipoZumbi tipo;
    EstadoIA estadoAtual;
    float velocidade;
    float raioColisao;
    bool vivo;
    int vida;
};

// ===========================================================================
// Sistema de Desbloqueio (Data-Driven) — bitmasks de capacidades
// ===========================================================================
struct EstadoDesbloqueio {
    unsigned long formasLiberadas;
    unsigned long movimentosLiberados;
    unsigned long origensLiberadas;
    unsigned long efeitosLiberados;
};

// ===========================================================================
// FASE 6 — Inventário de Skills (forward declaration)
//
// InventarioSkills e MenuLevelUp são definidos em SkillInventory.h e
// LevelUpChoice.h respectivamente. Declaramos aqui apenas os tamanhos
// necessários para que EstadoDoJogo possa incluí-los por valor sem
// incluir os headers completos (evita dependência circular com SkillTypes.h).
//
// Os includes reais ficam em GameLogic.h e Main.cpp.
// ===========================================================================

// Limites replicados aqui para que EstadoDoJogo seja autossuficiente
// (mesmo valor de SkillInventory.h e LevelUpChoice.h)
#define _INV_MAX_EQUIPADAS     6
#define _INV_MAX_CATALOGO    128
#define _INV_NIVEL_MAX         5
#define _MENU_MAX_ESCOLHAS     3
#define _ESCOLHA_DESC_MAX     96
#define _ESCOLHA_SUB_MAX      64

// AtributosGlobais — inline aqui para não depender de SkillInventory.h
struct AtributosGlobais {
    float bonusDano;
    float bonusVida;
    float bonusVelocidade;
    float bonusTensao;
    float bonusPerfuracao;
    int   bonusHPFlat;
};

enum SkillEstado {
    SKILL_BLOQUEADA   = 0,
    SKILL_DISPONIVEL  = 1,
    SKILL_EQUIPADA    = 2,
    SKILL_EVOLUIDA    = 3,
    SKILL_NIVEL_MAX   = 4
};

// InventarioSkills inline em Entities.h (POD, sem includes extras)
struct InventarioSkills {
    SkillEstado estado[_INV_MAX_CATALOGO];
    int         nivelSkill[_INV_MAX_CATALOGO];
    int         equipadas[_INV_MAX_EQUIPADAS];
    int         numEquipadas;
    AtributosGlobais atributosGlobais;
};

// Tipo e raridade da escolha de level-up
enum EscolhaTipo {
    ESCOLHA_NOVA_SKILL      = 0,
    ESCOLHA_UPGRADE_SKILL   = 1,
    ESCOLHA_ATRIBUTO_GLOBAL = 2
};

enum EscolhaRaridade {
    RARIDADE_COMUM    = 0,
    RARIDADE_INCOMUM  = 1,
    RARIDADE_RARA     = 2,
    RARIDADE_EPICA    = 3,
    RARIDADE_LENDARIA = 4
};

// LevelUpChoice — uma opção do menu de progressão
struct LevelUpChoice {
    EscolhaTipo     tipo;
    EscolhaRaridade raridade;
    int             referencia;
    int             valorExtra;
    char            descricao[_ESCOLHA_DESC_MAX];
    char            subtitulo[_ESCOLHA_SUB_MAX];
};

// MenuLevelUp — conjunto de opções sorteadas
struct MenuLevelUp {
    LevelUpChoice escolhas[_MENU_MAX_ESCOLHAS];
    int           quantidade;
};

// ---------------------------------------------------------------------------
// Estado global do jogo
// ---------------------------------------------------------------------------
struct EstadoDoJogo {
    Jogador protagonista;
    Entidade stand;
    std::vector<Zumbi> horda;
    std::vector<GemaXP> gemas;

    std::vector<Particula> particulas;
    std::vector<FloatingDamage> numerosFlutuantes;

    float tempoSobrevivido;

    float tempoUltimoSpawn;
    float cooldownAtual;
    int quantidadeSpawnAtual;

    bool pausadoParaUpgrade;
    int nivelAntesDaEscolha;
    bool jogoPausado;

    bool atirandoAgora;

    // FASE 6 — Menu de progressão (substitui opcoesUpgrade[]+quantidadeOpcoes)
    MenuLevelUp menuAtual;

    // RETROCOMPATIBILIDADE — mantidos para compilação de código legado que
    // ainda referencia esses campos. Deprecated: usar menuAtual.
    TipoUpgrade opcoesUpgrade[3];
    int quantidadeOpcoes;

    // FASE 6 — Inventário de skills do jogador
    InventarioSkills inventario;

    // Desbloqueios de capacidade (bitmasks)
    EstadoDesbloqueio desbloqueios;

    // Hook para ORIG_KILLEDENEMY
    Vetor3D ultimaPosicaoMorte;
    bool houveMorteRecente;
};

#endif // ENTITIES_H
