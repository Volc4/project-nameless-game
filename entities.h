#ifndef ENTITIES_H
#define ENTITIES_H
#include <vector>

struct Vetor3D {
    float x, y, z;
};

// Valor == índice em SistemaUpgrades::niveis — aplicação O(1) sem switch.
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

// Os 4 atributos da arma base que participam do sistema de Arquétipos.
enum AtributoArma {
    ATR_CADENCIA   = 0,
    ATR_DANO       = 1,
    ATR_PERFURACAO = 2,
    ATR_QUANTIDADE = 3,
    TOTAL_ATRIBUTOS_ARMA = 4
};

struct SistemaUpgrades {
    int niveis[TOTAL_UPGRADES];
};

// Dois atributos em nível 2 desbloqueiam um Arquétipo; os outros dois ficam bloqueados.
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

struct EstadoArquetipo {
    Arquetipo arquetipo;       // ARQ_NENHUM enquanto não especializado
    bool      especializado;   // true após travar os dois atributos
    int       atributoA;       // AtributoArma principal 1 (-1 se nenhum)
    int       atributoB;       // AtributoArma principal 2 (-1 se nenhum)
    bool      evolucaoFinal;   // true quando ambos chegam a nível 3
};

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

struct Entidade {  // "Stand"
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

struct FloatingDamage {
    Vetor3D posicao;
    int valorDano;
    float tempoRestante;
    float tempoTotal;
    float deslocamentoVertical;
    float corR, corG, corB;
    float transparencia;
};

enum TipoZumbi { NORMAL, RAPIDO, TANK, ATIRADOR, EXPLOSIVO };
enum EstadoIA  { WANDER, CHASE };

// AGUARDANDO_BOSS: 10 min atingidos, spawn bloqueado até limpar a arena.
enum FasePartida {
    FASE_NORMAL,
    FASE_AGUARDANDO_BOSS,
    FASE_BOSS,
    FASE_VITORIA
};

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

struct ProjetilZumbi {
    Vetor3D posicao;
    Vetor3D direcao;    // normalizada, rumo ao jogador no momento do disparo
    float   velocidade;
    int     dano;
    bool    ativo;
    bool    ehDoBoss;   // true = projétil da rajada do Boss (visual diferente)
};

struct EstadoDesbloqueio {
    unsigned long formasLiberadas;
    unsigned long movimentosLiberados;
    unsigned long origensLiberadas;
    unsigned long efeitosLiberados;
};

// Limites aqui para que EstadoDoJogo seja autossuficiente sem incluir SkillTypes.h.
#define _INV_MAX_EQUIPADAS     6
#define _INV_MAX_CATALOGO    128
#define _INV_NIVEL_MAX         5
#define _MENU_MAX_ESCOLHAS     3
#define _ESCOLHA_DESC_MAX     96
#define _ESCOLHA_SUB_MAX      64

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

struct InventarioSkills {
    SkillEstado estado[_INV_MAX_CATALOGO];
    int         nivelSkill[_INV_MAX_CATALOGO];
    int         equipadas[_INV_MAX_EQUIPADAS];
    int         numEquipadas;
    AtributosGlobais atributosGlobais;
};

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

struct LevelUpChoice {
    EscolhaTipo     tipo;
    EscolhaRaridade raridade;
    int             referencia;
    int             valorExtra;
    char            descricao[_ESCOLHA_DESC_MAX];
    char            subtitulo[_ESCOLHA_SUB_MAX];
};

struct MenuLevelUp {
    LevelUpChoice escolhas[_MENU_MAX_ESCOLHAS];
    int           quantidade;
};

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
    bool pausaManual;           // ESC/P — distinto de pausadoParaUpgrade

    MenuLevelUp      menuAtual;
    InventarioSkills inventario;
    EstadoDesbloqueio desbloqueios;

    Vetor3D ultimaPosicaoMorte;  // origem para ORIG_KILLEDENEMY
    bool    houveMorteRecente;

    EstadoArquetipo arquetipoArma;

    // Calculada a cada frame a partir da transform da Sofia; origem dos projéteis manuais.
    Vetor3D posicaoPistola;

    FasePartida fasePartida;
    float       tempMensagemBoss;   // timer para "BOSS APARECEU!" / "BOSS DERROTADO!"
    bool        bossJaFoiInvocado;  // garante spawn único por partida
    float       tempoEntradaBoss;   // animação de entrada (pulso visual)
};

// Funções de tabela: fontes únicas de todos os atributos numéricos por nível.

// Limite de tensão antes da sobrecarga.
inline float maxTensaoDoNivel(int nivel) {
    if (nivel == 1) return 130.0f;
    if (nivel == 2) return 160.0f;
    if (nivel >= 3) return 200.0f;
    return 100.0f; // nivel 0
}

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

// Multiplicador do cooldown (< 1.0 = mais rápido): nível 0→1.00  1→0.75  2→0.50  3→0.25.
inline float fatorCadenciaDoNivel(int nivel) {
    if (nivel == 1) return 0.75f;
    if (nivel == 2) return 0.50f;
    if (nivel >= 3) return 0.25f;
    return 1.0f; // nivel 0
}

// Nível 3+ retorna PERFURACAO_INFINITA; o executor trata como "não morre por perfuração".
const int PERFURACAO_INFINITA = 1000000;

inline int perfuracaoDoNivel(int nivel) {
    if (nivel <= 0) return 0;
    if (nivel == 1) return 2;
    if (nivel == 2) return 5;
    return PERFURACAO_INFINITA; // nivel 3+
}

#endif // ENTITIES_H
