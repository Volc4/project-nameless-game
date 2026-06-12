#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include "Entities.h"
#include <cmath> // Necessário para a função sqrt()

// Retorna a distância ao quadrado entre dois pontos
inline float calcularDistanciaQuadrada(Vetor3D p1, Vetor3D p2) {
    return (p2.x - p1.x) * (p2.x - p1.x) +
           (p2.y - p1.y) * (p2.y - p1.y) +
           (p2.z - p1.z) * (p2.z - p1.z);
}

// Verifica a colisão entre duas entidades usando Esferas Envolventes
inline bool verificarColisao(Vetor3D pos1, float raio1, Vetor3D pos2, float raio2) {
    float distQuadrada = calcularDistanciaQuadrada(pos1, pos2);
    float somaRaios = raio1 + raio2;
    return distQuadrada <= (somaRaios * somaRaios);
}

// Calcula o vetor de direção normalizado (tamanho 1) de uma origem para um alvo
inline Vetor3D obterDirecaoNormalizada(Vetor3D origem, Vetor3D alvo) {
    Vetor3D direcao = {0.0f, 0.0f, 0.0f};
    
    float dx = alvo.x - origem.x;
    float dz = alvo.z - origem.z;
    
    // Calcula a magnitude exata (tamanho do vetor) usando Pitágoras
    float magnitude = sqrt((dx * dx) + (dz * dz));
    
    // Evita divisão por zero caso estejam exatamente na mesma coordenada
    if (magnitude > 0.0001f) {
        direcao.x = dx / magnitude;
        direcao.z = dz / magnitude;
    }
    
    return direcao;
}

#endif