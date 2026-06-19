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
    TENSAO_UP  = 3,  // Chamado TENSAO_UP para não colidir com membros de Entidade
    VELOCIDADE = 4,
    VIDA       = 5,
    TOTAL_UPGRADES = 6   // Sentinela: nunca deve aparecer como opção real
};

// ---------------------------------------------------------------------------
// Tipos de disparo resultantes da evolução de dois atributos ao nível 2.
// A nomenclatura segue a ordem dos enumeradores de TipoUpgrade (menor índice
// primeiro), facilitando a determinação automática em determinarTipoDisparo().
// ---------------------------------------------------------------------------
enum TipoDisparo {
    DISPARO_NORMAL,              // Sem evolução
    DISPARO_DANO_CADENCIA,       // DANO(0) + CADENCIA(1)
    DISPARO_DANO_PERFURACAO,     // DANO(0) + PERFURACAO(2)
    DISPARO_DANO_TENSAO,         // DANO(0) + TENSAO_UP(3)
    DISPARO_DANO_VELOCIDADE,     // DANO(0) + VELOCIDADE(4)
    DISPARO_DANO_VIDA,           // DANO(0) + VIDA(5)
    DISPARO_CADENCIA_PERFURACAO, // CADENCIA(1) + PERFURACAO(2)
    DISPARO_CADENCIA_TENSAO,     // CADENCIA(1) + TENSAO_UP(3)
    DISPARO_CADENCIA_VELOCIDADE, // CADENCIA(1) + VELOCIDADE(4)
    DISPARO_CADENCIA_VIDA,       // CADENCIA(1) + VIDA(5)
    DISPARO_PERFURACAO_TENSAO,   // PERFURACAO(2) + TENSAO_UP(3)
    DISPARO_PERFURACAO_VELOCIDADE,// PERFURACAO(2) + VELOCIDADE(4)
    DISPARO_PERFURACAO_VIDA,     // PERFURACAO(2) + VIDA(5)
    DISPARO_TENSAO_VELOCIDADE,   // TENSAO_UP(3) + VELOCIDADE(4)
    DISPARO_TENSAO_VIDA,         // TENSAO_UP(3) + VIDA(5)
    DISPARO_VELOCIDADE_VIDA      // VELOCIDADE(4) + VIDA(5)
};

// ---------------------------------------------------------------------------
// Projétil
//   perfuracaoRestante: quantos inimigos adicionais o tiro ainda pode atingir.
//   0 = desaparece ao primeiro acerto.  >0 = atravessa inimigos.
// ---------------------------------------------------------------------------
struct Projetil {
    Vetor3D posicao;
    Vetor3D direcao;
    float velocidade;
    float raioColisao;
    bool ativo;
    int dano;
    int perfuracaoRestante;  // NOVO: controla quantos inimigos extras o tiro atravessa
};

// ---------------------------------------------------------------------------
// Sistema de upgrades do jogador.
// Armazena apenas o nível (0–3) de cada atributo.
// Os valores práticos são calculados dinamicamente pelas funções de GameLogic.
// ---------------------------------------------------------------------------
struct SistemaUpgrades {
    int niveis[TOTAL_UPGRADES];  // índice = TipoUpgrade
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

    // Sistema de HP e invencibilidade temporária
    int hp;
    int hpMaximo;               // NOVO: rastreado para que upgrades de VIDA funcionem
    float temporizadorIframe;
    float duracaoIframe;

    // Sistema de XP e Nível
    int xpAtual;
    int xpParaProximoNivel;
    int nivel;

    // NOVO: upgrades organizados em estrutura coesa
    SistemaUpgrades upgrades;
};

// ---------------------------------------------------------------------------
// Dados da Entidade (O "Stand")
// ---------------------------------------------------------------------------
struct Entidade {
    Vetor3D posicao;
    float anguloMira;
    float tensaoAtual;          // 0.0f a 100.0f

    bool emSobrecarga;

    // Controle da janela de Parry
    bool parryAtivo;
    float temporizadorParry;
    float cooldownParry;
    float temporizadorCooldown;

    // Efeito visual de feedback do Parry
    bool parryBemSucedido;
    float temporizadorFeedback;

    // NOVO: tipo de disparo evoluído desta partida
    TipoDisparo tipoDisparoAtual;
};

struct GemaXP {
    Vetor3D posicao;
    int valorXP;
    bool coletada;
};

// Tipos de inimigos baseados no GDD
enum TipoZumbi { NORMAL, RAPIDO, TANK, ATIRADOR, EXPLOSIVO };

// Estados da Inteligência Artificial (FSM)
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

// ---------------------------------------------------------------------------
// Estado global do jogo
// ---------------------------------------------------------------------------
struct EstadoDoJogo {
    Jogador protagonista;
    Entidade stand;
    std::vector<Zumbi> horda;
    std::vector<Projetil> tirosNaTela;
    std::vector<GemaXP> gemas;
    float tempoSobrevivido;

    float tempoUltimoSpawn;
    float cooldownAtual;
    int quantidadeSpawnAtual;

    // Controle do loop principal
    bool pausadoParaUpgrade;
    int nivelAntesDaEscolha;

    // Sinaliza se o jogador disparou algum tiro neste frame
    bool atirandoAgora;

    // NOVO: controle de evolução de disparo
    int contagemAtributosNivel2;    // Quantos atributos atingiram exatamente nível 2
    bool disparoEvoluido;           // true após primeira evolução (nunca muda de novo)
    TipoUpgrade primeiroAtributoNivel2;  // Primeiro atributo a atingir nível 2
    TipoUpgrade segundoAtributoNivel2;   // Segundo atributo a atingir nível 2

    // NOVO: opções sorteadas para o menu de upgrade atual
    TipoUpgrade opcoesUpgrade[3];
    int quantidadeOpcoes;           // Pode ser < 3 se poucos atributos disponíveis
};

#endif