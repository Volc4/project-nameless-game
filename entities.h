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
    QUANTIDADE = 3,
    TENSAO_UP     = 4,
    VIDA          = 5,
    VELOCIDADE_UP = 6,
    TOTAL_UPGRADES = 7
};

// Atributos QUE COMPÕEM A ARMA BASE "Disparo" (subconjunto de TipoUpgrade).
// São os 4 que participam do sistema de Arquétipos (spec Fase de Evolução).
// Mantidos como lista para o menu e a lógica de especialização.
enum AtributoArma {
    ATR_CADENCIA   = 0,
    ATR_DANO       = 1,
    ATR_PERFURACAO = 2,
    ATR_QUANTIDADE = 3,
    TOTAL_ATRIBUTOS_ARMA = 4
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

// ===========================================================================
//  SISTEMA DE ARQUÉTIPOS — especialização irreversível da arma base
//
//  Quando DOIS atributos diferentes da arma atingem o nível 2, o jogador
//  desbloqueia um Arquétipo. A partir daí a build é permanentemente
//  especializada: só esses dois atributos podem evoluir; os outros dois
//  ficam bloqueados pelo resto da partida. Ao levar os dois a nível 3,
//  a arma atinge sua Evolução Final.
// ===========================================================================
enum Arquetipo {
    ARQ_NENHUM = 0,           // ainda não especializado
    ARQ_METRALHADORA_PESADA,  // Cadência + Dano
    ARQ_METRALHADORA_LEVE,    // Cadência + Quantidade
    ARQ_CANHAO_ROTATIVO,      // Cadência + Perfuração
    ARQ_RIFLE_LASER,          // Dano + Perfuração
    ARQ_ESPINGARDA_TATICA,    // Dano + Quantidade
    ARQ_CANHAO_FRAGMENTACAO,  // Perfuração + Quantidade
    TOTAL_ARQUETIPOS
};

// Estado da especialização. Vive em EstadoDoJogo. POD, serializável.
struct EstadoArquetipo {
    Arquetipo arquetipo;       // ARQ_NENHUM enquanto não especializado
    bool      especializado;   // true após travar os dois atributos
    int       atributoA;       // AtributoArma principal 1 (-1 se nenhum)
    int       atributoB;       // AtributoArma principal 2 (-1 se nenhum)
    bool      evolucaoFinal;   // true quando ambos chegam a nível 3
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

    bool  devorarPedido;
    float temporizadorDevorar;
    float temporizadorCooldownDevorar;
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

// ===========================================================================
// SISTEMA DE FASES DA PARTIDA
//   Controla em qual fase o jogo se encontra usando uma máquina de estados
//   simples. Evita múltiplos booleanos espalhados pelo código.
//
//   FASE_NORMAL        — spawn normal de zumbis.
//   FASE_AGUARDANDO    — 10 min atingidos; spawn bloqueado; arena sendo limpa.
//   FASE_BOSS          — Boss ativo perseguindo o jogador.
//   FASE_VITORIA       — Boss derrotado; spawn retoma infinitamente.
// ===========================================================================
enum FasePartida {
    FASE_NORMAL,
    FASE_AGUARDANDO_BOSS,
    FASE_BOSS,
    FASE_VITORIA
};

// Dados do Zumbi
struct Zumbi {
    Vetor3D posicao;
    TipoZumbi tipo;
    EstadoIA estadoAtual;
    float velocidade;
    float raioColisao;
    bool vivo;
    int vida;
    int dano;        // dano de contato (aplicado via p.hp -= z.dano)
    float tiroTimer; // timer de cooldown do ATIRADOR entre disparos
    bool ehBoss;     // true somente para o Boss (tratamento especial em toda a pipeline)
};

// Projétil disparado pelo Atirador (devorável, move-se em linha reta)
struct ProjetilZumbi {
    Vetor3D posicao;
    Vetor3D direcao;    // normalizada, rumo ao jogador no momento do disparo
    float   velocidade;
    int     dano;
    bool    ativo;
    bool    ehDoBoss;   // true = projétil da rajada do Boss (visual diferente)
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
    std::vector<ProjetilZumbi> projeteisZumbi;

    float tempoSobrevivido;

    float tempoUltimoSpawn;
    float cooldownAtual;
    int quantidadeSpawnAtual;

    bool pausadoParaUpgrade;
    int nivelAntesDaEscolha;
    bool jogoPausado;

    bool atirandoAgora;
    bool pausaManual;             // pause manual via ESC/P (distinto de pausadoParaUpgrade)

    // FASE 6 — Menu de progressão
    MenuLevelUp menuAtual;

    // FASE 6 — Inventário de skills do jogador
    InventarioSkills inventario;

    // Desbloqueios de capacidade (bitmasks)
    EstadoDesbloqueio desbloqueios;

    // Hook para ORIG_KILLEDENEMY
    Vetor3D ultimaPosicaoMorte;
    bool houveMorteRecente;

    // Sistema de Arquétipos — especialização irreversível da arma base.
    EstadoArquetipo arquetipoArma;

    // Posição da pistola no espaço do mundo (calculada a cada frame em Main.cpp
    // a partir da transform da Sofia + bone socket). Usada como origem dos
    // projéteis manuais do Disparo.
    Vetor3D posicaoPistola;

    // ===========================================================================
    // SISTEMA DE BOSS — campos de controle da máquina de estados
    //   fasePartida      : fase atual da partida (ver enum FasePartida)
    //   tempMensagemBoss : timer decrescente das mensagens "BOSS APARECEU!" /
    //                      "BOSS DERROTADO!" exibidas na tela
    //   bossJaFoiInvocado: garante que o Boss apareça apenas UMA vez por partida
    //   tempoEntradaBoss : timer da animação de entrada visual do Boss (pulso)
    // ===========================================================================
    FasePartida fasePartida;
    float       tempMensagemBoss;
    bool        bossJaFoiInvocado;
    float       tempoEntradaBoss;
};

// ---------------------------------------------------------------------------
// Tensão por nível de TENSAO_UP — fonte única dos parâmetros da barra.
//   maxTensaoDoNivel      : limite máximo antes da sobrecarga
//   taxaDecaimentoDoNivel : velocidade de esvaziamento quando não está atirando
// ---------------------------------------------------------------------------
inline float maxTensaoDoNivel(int nivel) {
    if (nivel == 1) return 130.0f;
    if (nivel == 2) return 160.0f;
    if (nivel >= 3) return 200.0f;
    return 100.0f; // nivel 0
}

// fatorVelocidadeDoNivel — multiplicador de velocidade do jogador.
//   Nivel 0->1.0x  Nivel 1->1.25x  Nivel 2->1.5x  Nivel 3+->2.0x
inline float fatorVelocidadeDoNivel(int nivel) {
    if (nivel == 1) return 1.25f;
    if (nivel == 2) return 1.50f;
    if (nivel >= 3) return 2.00f;
    return 1.0f;
}

inline float taxaDecaimentoTensaoDoNivel(int nivel) {
    if (nivel == 1) return 22.0f;
    if (nivel == 2) return 35.0f;
    if (nivel >= 3) return 55.0f;
    return 15.0f; // nivel 0
}

// ---------------------------------------------------------------------------
// Fontes únicas dos atributos de upgrade — devem ficar aqui para que tanto
// SkillCatalog.h quanto GameLogic.h (e ArmaInteligente.h) as enxerguem,
// já que entities.h é o header base incluído por todos.
// ---------------------------------------------------------------------------

inline float fatorDanoDoNivel(int nivel) {
    if (nivel == 1) return 2.0f;
    if (nivel == 2) return 4.0f;
    if (nivel >= 3) return 8.0f;
    return 1.0f; // nivel 0
}

inline int quantidadeDoNivel(int nivel) {
    if (nivel == 1) return 3;  // centro + 2 (±0.20 rad)
    if (nivel == 2) return 5;  // centro + 4 (±0.20, ±0.40 rad)
    if (nivel >= 3) return 7;  // centro + 6 (±0.20, ±0.40, ±0.60 rad)
    return 1; // nivel 0
}

// Entrada: nível do atributo CADENCIA (0–3)
// Saída:   multiplicador do cooldown (< 1.0 = mais rápido)
//   Nivel 0 -> 1.00  Nivel 1 -> 0.75  Nivel 2 -> 0.50  Nivel 3 -> 0.25
inline float fatorCadenciaDoNivel(int nivel) {
    if (nivel == 1) return 0.75f;
    if (nivel == 2) return 0.50f;
    if (nivel >= 3) return 0.25f;
    return 1.0f; // nivel 0
}

// ---------------------------------------------------------------------------
// perfuracaoDoNivel — FONTE ÚNICA do atributo PERFURAÇÃO.
//   Nivel 0->0  Nivel 1->2  Nivel 2->5  Nivel 3->"tudo"
//   PERFURACAO_INFINITA funciona direto no loop de colisão do SkillManager
//   (if perfuracaoRestante <= 0 -> morre), sem sentinela especial.
// ---------------------------------------------------------------------------
const int PERFURACAO_INFINITA = 1000000;

inline int perfuracaoDoNivel(int nivel) {
    if (nivel <= 0) return 0;
    if (nivel == 1) return 2;
    if (nivel == 2) return 5;
    return PERFURACAO_INFINITA; // nivel 3+
}

#endif // ENTITIES_H
