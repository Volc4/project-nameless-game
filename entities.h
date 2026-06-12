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
    bool vivo; // Se tomar 1 hit, vira false
};

// Dados da Entidade (O "Stand")
struct Entidade {
    Vetor3D posicao;
    float anguloMira; // Calculado com o mouse depois
    float tensaoAtual; // Vai de 0 a 100
    bool emSobrecarga; // Se a tensão chegar a 100%
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
    float tempoSobrevivido;
    
    // Variáveis de controle de Spawn progressivo
    float tempoUltimoSpawn;
    float cooldownAtual;
    int quantidadeSpawnAtual;
};

#endif

