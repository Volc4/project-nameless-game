#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include "Entities.h"
#include "MathUtils.h"
#include <cstdlib>
#include <cmath>

// Atualiza a posição da sobrevivente baseada nas teclas pressionadas (WASD)
inline void moverJogador(Jogador& jog, float dx, float dz, float deltaTime) {
    if (!jog.vivo) return;

    jog.posicao.x += dx * jog.velocidade * deltaTime;
    jog.posicao.z += dz * jog.velocidade * deltaTime;
}

// Atualiza a posição da Entidade Fantasmagórica para orbitar o jogador
inline void atualizarEntidade(Entidade& ent, Jogador& jog) {
    ent.posicao = jog.posicao; 
}

// Calcula uma coordenada aleatoria em um raio distante do jogador
inline Vetor3D calcularPosicaoSpawnOculta(Vetor3D posJogador) {
    float raioSpawn = 30.0f; 
    
    float anguloGraus = (float)(rand() % 360);
    float anguloRadianos = anguloGraus * (3.14159f / 180.0f);

    Vetor3D novaPosicao;
    novaPosicao.x = posJogador.x + (raioSpawn * cos(anguloRadianos));
    novaPosicao.y = 0.0f; 
    novaPosicao.z = posJogador.z + (raioSpawn * sin(anguloRadianos));

    return novaPosicao;
}

// Função "Fábrica": Cria e configura um zumbi dependendo do tipo escolhido
inline Zumbi invocarZumbi(TipoZumbi tipoDesejado, Vetor3D posicaoInicial) {
    Zumbi z;
    z.posicao = posicaoInicial;
    z.tipo = tipoDesejado;
    z.estadoAtual = WANDER; 
    z.vivo = true;

    switch (tipoDesejado) {
        case NORMAL:
            z.velocidade = 2.0f; z.raioColisao = 1.0f; z.vida = 1; break;
        case RAPIDO:
            z.velocidade = 4.5f; z.raioColisao = 0.7f; z.vida = 1; break;
        case TANK:
            z.velocidade = 1.2f; z.raioColisao = 1.5f; z.vida = 10; break;
        case ATIRADOR:
            z.velocidade = 1.8f; z.raioColisao = 1.0f; z.vida = 2; break;
        case EXPLOSIVO:
            z.velocidade = 2.5f; z.raioColisao = 1.2f; z.vida = 1; break;
    }
    return z;
}

// Sorteia o tipo de zumbi baseado no tempo de sobrevivencia
inline TipoZumbi sortearTipoZumbi(float tempoSegundos) {
    int chance = rand() % 100;

    if (tempoSegundos >= 330.0f) { // 5.5 min
        if (chance < 50) return NORMAL;
        if (chance < 70) return RAPIDO;
        if (chance < 80) return TANK;
        if (chance < 90) return ATIRADOR;
        return EXPLOSIVO;
    }
    else if (tempoSegundos >= 240.0f) { // 4.0 min
        if (chance < 60) return NORMAL;
        if (chance < 80) return RAPIDO;
        if (chance < 90) return TANK;
        return ATIRADOR;
    }
    else if (tempoSegundos >= 150.0f) { // 2.5 min
        if (chance < 70) return NORMAL;
        if (chance < 90) return RAPIDO;
        return TANK;
    }
    else if (tempoSegundos >= 60.0f) { // 1.0 min
        if (chance < 80) return NORMAL;
        return RAPIDO;
    }
    return NORMAL;
}

// Gerencia a dificuldade progressiva e instancia novos zumbis na horda
inline void processarSpawn(EstadoDoJogo& jogo, float deltaTime) {
    const float COOLDOWN_BASE = 3.0f; 
    const int QUANTIDADE_BASE = 1;    
    
    float fatorDificuldade = 1.0f + (jogo.tempoSobrevivido / 60.0f);
    jogo.cooldownAtual = COOLDOWN_BASE / fatorDificuldade;
    
    if (jogo.cooldownAtual < 0.2f) {
        jogo.cooldownAtual = 0.2f;
    }

    jogo.quantidadeSpawnAtual = (int)(QUANTIDADE_BASE * fatorDificuldade);

    if (jogo.tempoSobrevivido - jogo.tempoUltimoSpawn >= jogo.cooldownAtual) {
        jogo.tempoUltimoSpawn = jogo.tempoSobrevivido;

        for (int i = 0; i < jogo.quantidadeSpawnAtual; i++) {
            TipoZumbi tipoSorteado = sortearTipoZumbi(jogo.tempoSobrevivido);
            Vetor3D posicaoSpawn = calcularPosicaoSpawnOculta(jogo.protagonista.posicao);
            Zumbi novoZumbi = invocarZumbi(tipoSorteado, posicaoSpawn);
            jogo.horda.push_back(novoZumbi);
        }
    }
}

// Atualiza a mente e a posição de todos os zumbis da horda
inline void processarIA(EstadoDoJogo& jogo, float deltaTime) {
    float raioDeVisaoQuadrado = 100.0f; // Raio de visão para iniciar a caça

    for (Zumbi& z : jogo.horda) {
        if (!z.vivo) continue;

        float dist = calcularDistanciaQuadrada(z.posicao, jogo.protagonista.posicao);

        // Transição de Estados da FSM
        if (dist <= raioDeVisaoQuadrado) {
            z.estadoAtual = CHASE;
        } else {
            z.estadoAtual = WANDER;
        }

        // Execução do Movimento
        if (z.estadoAtual == CHASE) {
            // Obtém o vetor direção com tamanho 1 apontando para o jogador
            Vetor3D direcao = obterDirecaoNormalizada(z.posicao, jogo.protagonista.posicao);
            
            // Move o zumbi multiplicando a direção pela velocidade dele
            z.posicao.x += direcao.x * z.velocidade * deltaTime;
            z.posicao.z += direcao.z * z.velocidade * deltaTime;
        } 
        else if (z.estadoAtual == WANDER) {
            // Movimentação passiva: anda lentamente para o norte como comportamento padrão
            // Isso pode ser aprimorado posteriormente para gerar rotas aleatórias
            z.posicao.z += (z.velocidade * 0.5f) * deltaTime; 
        }
    }
}

#endif