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

// ---------------------------------------------------------------------------
// Particula
//   Unidade individual do sistema de partículas (explosões de morte etc).
//   Puramente visual: nunca participa de colisão ou lógica de jogo.
//
//   posicao            — posição 3D atual no mundo.
//   velocidade          — vetor de deslocamento por segundo (já contém a
//                         magnitude/direção combinadas).
//   tempoVida           — segundos restantes antes da partícula expirar.
//   tempoVidaMaximo     — duração total original, usada para interpolar
//                         o fade de transparência (tempoVida / tempoVidaMaximo).
//   corR, corG, corB    — cor RGB (0.0–1.0) da partícula.
//   tamanho             — tamanho do ponto/quad ao desenhar.
//   transparencia       — alfa atual (0.0–1.0), decai com o tempo.
//   ativa               — false = pronta para remoção.
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
// FloatingDamage
//   Número de dano flutuante exibido sobre um inimigo no instante em que
//   ele é atingido. Puramente visual — não interfere em hp, colisão ou
//   qualquer outro sistema de jogo.
//
//   posicao            — posição 3D do impacto (convertida para tela via
//                         gluProject no momento do desenho).
//   valorDano           — quantidade de dano a ser exibida como texto.
//   tempoRestante       — segundos restantes antes de desaparecer.
//   tempoTotal          — duração original, usada para interpolar a subida
//                         e o fade (1.0 no início, 0.0 no fim).
//   deslocamentoVertical— quanto o número já subiu (unidades de mundo),
//                         cresce a cada frame para o efeito de flutuação.
//   corR, corG, corB    — cor RGB do texto.
//   transparencia       — alfa atual (0.0–1.0), decai com o tempo.
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

    // NOVO: sistemas de feedback visual (puramente cosméticos)
    std::vector<Particula> particulas;
    std::vector<FloatingDamage> numerosFlutuantes;

    float tempoSobrevivido;

    float tempoUltimoSpawn;
    float cooldownAtual;
    int quantidadeSpawnAtual;

    // Controle do loop principal (Cuidado para não duplicar esta parte!)
    bool pausadoParaUpgrade;
    int nivelAntesDaEscolha;
    bool jogoPausado; // Controle de pausa manual

    // Sinaliza se o jogador disparou algum tiro neste frame
    bool atirandoAgora;

    // Controle de evolução de disparo
    int contagemAtributosNivel2;    // Quantos atributos atingiram exatamente nível 2
    bool disparoEvoluido;           // true após primeira evolução
    TipoUpgrade primeiroAtributoNivel2;  
    TipoUpgrade segundoAtributoNivel2;   

    // Opções sorteadas para o menu de upgrade atual
    TipoUpgrade opcoesUpgrade[3];
    int quantidadeOpcoes;           
};

#endif