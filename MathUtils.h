#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include "Entities.h"
#include <cmath> // Necessário para sqrt() e fabs()

// Retorna a distância ao quadrado entre dois pontos
inline float calcularDistanciaQuadrada(Vetor3D p1, Vetor3D p2) {
    return (p2.x - p1.x) * (p2.x - p1.x) +
           (p2.y - p1.y) * (p2.y - p1.y) +
           (p2.z - p1.z) * (p2.z - p1.z);
}

// Verifica a colisão entre duas entidades usando AABB (Caixas Quadradas)
// Garante que tiros nas quinas e bordas dos cubos sejam registrados perfeitamente
inline bool verificarColisao(Vetor3D pos1, float raio1, Vetor3D pos2, float raio2) {
    bool colisaoX = std::fabs(pos1.x - pos2.x) <= (raio1 + raio2);
    bool colisaoZ = std::fabs(pos1.z - pos2.z) <= (raio1 + raio2);
    return colisaoX && colisaoZ;
}

// Calcula o vetor de direção normalizado (tamanho 1) de uma origem para um alvo
inline Vetor3D obterDirecaoNormalizada(Vetor3D origem, Vetor3D alvo) {
    Vetor3D direcao = {0.0f, 0.0f, 0.0f};
    
    float dx = alvo.x - origem.x;
    float dz = alvo.z - origem.z;
    
    float length = std::sqrt(dx * dx + dz * dz);
    
    if (length > 0.0001f) {
        direcao.x = dx / length;
        direcao.z = dz / length;
    }
    
    return direcao;
}

#endif