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

// ---------------------------------------------------------------------------
// _rotacionarXZ — gira um vetor 2D no plano XZ por `angulo` radianos.
//   Movido de Habilidade.h (era HabilidadeBase::_rotacionar). Usado por
//   Cone/Ring/Spiral para distribuir/girar direções.
// ---------------------------------------------------------------------------
inline void _rotacionarXZ(float& dx, float& dz, float angulo) {
    float cosA = std::cos(angulo);
    float sinA = std::sin(angulo);
    float nx   = dx * cosA - dz * sinA;
    float nz   = dx * sinA + dz * cosA;
    dx = nx;
    dz = nz;
}

// ---------------------------------------------------------------------------
// distanciaPontoSegmento — menor distância (no plano XZ) de um ponto a um
//   segmento que parte de `base` na direção unitária `dir` com `comprimento`.
//   Usado por Beam e Wall para colisão de faixa.
//   Projeta o ponto sobre o segmento, satura t em [0, comprimento], e mede.
// ---------------------------------------------------------------------------
inline float distanciaPontoSegmento(Vetor3D ponto, Vetor3D base,
                                    Vetor3D dir, float comprimento) {
    float px = ponto.x - base.x;
    float pz = ponto.z - base.z;
    // projeção escalar sobre a direção (dir assumida unitária no plano XZ)
    float t = px * dir.x + pz * dir.z;
    if (t < 0.0f)            t = 0.0f;
    if (t > comprimento)     t = comprimento;
    float cx = base.x + dir.x * t;
    float cz = base.z + dir.z * t;
    float ex = ponto.x - cx;
    float ez = ponto.z - cz;
    return std::sqrt(ex * ex + ez * ez);
}

// ---------------------------------------------------------------------------
// dentroDoArco — true se `ponto` está dentro de um setor de anel centrado em
//   `centro`: entre raioInterno e raioExterno, e dentro de meia-abertura
//   `meiaAbertura` (radianos) ao redor de `anguloCentral`. Usado por Arc.
// ---------------------------------------------------------------------------
inline bool dentroDoArco(Vetor3D ponto, Vetor3D centro,
                         float raioInterno, float raioExterno,
                         float anguloCentral, float meiaAbertura) {
    float dx = ponto.x - centro.x;
    float dz = ponto.z - centro.z;
    float dist2 = dx * dx + dz * dz;
    if (dist2 < raioInterno * raioInterno) return false;
    if (dist2 > raioExterno * raioExterno) return false;

    float ang = std::atan2(dz, dx);            // [-pi, pi]
    float diff = ang - anguloCentral;
    // normaliza diff para [-pi, pi]
    const float PI  = 3.14159265358979323846f;
    const float TAU = 6.28318530717958647692f;
    while (diff >  PI) diff -= TAU;
    while (diff < -PI) diff += TAU;
    return (diff >= -meiaAbertura && diff <= meiaAbertura);
}

#endif