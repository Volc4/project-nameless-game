#ifndef ENTITIES_H
#define ENTITIES_H
#include <vector>
// Estrutura matemática básica para posições no mundo 3D
struct Vetor3D {
    float x, y, z;
};

struct Projetil {
    Vetor3D posicao;
    Vetor3D direcao;
    float velocidade;
    float raioColisao;
    bool ativo;
    int dano;
};

// Dados da Protagonista (Sobrevivente)
struct Jogador {
    Vetor3D posicao;
    float velocidade;
    float raioColisao;
    bool emDash;
    bool vivo;

    // Sistema de HP e invencibilidade temporária
    int hp;
    float temporizadorIframe;   // Contador regressivo em segundos
    float duracaoIframe;        // Duração total do i-frame após levar dano

    // Sistema de XP e Nível
    int xpAtual;
    int xpParaProximoNivel;
    int nivel;
};

// Dados da Entidade (O "Stand")
struct Entidade {
    Vetor3D posicao;
    float anguloMira;
    float tensaoAtual;          // 0.0f a 100.0f

    // NOVO: emSobrecarga agora é true quando tensão chega a 100%
    // e permanece assim até a tensão drenar de volta para 0%.
    bool emSobrecarga;

    // Controle da janela de Parry
    bool parryAtivo;            // true durante a janela de execução válida
    float temporizadorParry;    // Conta a duração da janela ativa
    float cooldownParry;        // Impede spam do Parry
    float temporizadorCooldown;

    // Efeito visual de feedback do Parry
    bool parryBemSucedido;
    float temporizadorFeedback;
};

struct GemaXP {
    Vetor3D posicao;
    int valorXP;
    bool coletada;
};

// Tipos de inimigos baseados no seu GDD
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
    bool pausadoParaUpgrade;    // Congela o timer quando true
    int nivelAntesDaEscolha;    // Para saber quantos upgrades mostrar

    // NOVO: sinaliza se o jogador disparou algum tiro neste frame.
    // Setado em Main.cpp (cliqueMouse) e lido em atualizarTensao().
    bool atirandoAgora;
};
#endif