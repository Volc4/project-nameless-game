#ifndef SKILL_TYPES_H
#define SKILL_TYPES_H

// ===========================================================================
//  SkillTypes.h — Enums e structs POD imutáveis que descrevem uma skill.
//
//  Nenhuma lógica de execução — apenas estrutura de dados. Inclui
//  RuntimeSkill.h no final (estado mutável que depende dos enums daqui).
// ===========================================================================

#include "Entities.h"   // Vetor3D, EstadoDoJogo, etc.
#include <cstring>      // memset

enum FormaType {
    FORMA_PROJECTILE = 0,   // projétil pontual (o disparo atual)
    FORMA_CONE,             // leque de projéteis
    FORMA_RING,             // anel de projéteis
    FORMA_AREA,             // círculo de área (AoE periódico)
    FORMA_AURA,             // círculo que segue o jogador
    FORMA_BEAM,             // feixe reto (segmento)
    FORMA_WALL,             // parede (segmento fixo)
    FORMA_WAVE,             // frente de onda
    FORMA_ARC,              // arco de anel (setor)
    FORMA_EXPLOSION,        // explosão instantânea (1 tick)
    FORMA_CHAIN,            // salto encadeado entre inimigos
    FORMA_PRISM,            // projétil que se divide ao colidir
    FORMA_MAX
};

// Alias para SkillValidator que usa _TOTAL como sentinela.
const int FORMA_TOTAL = FORMA_MAX;

enum MovimentoType {
    MOV_LINEAR = 0,         // voa em linha reta
    MOV_HOMING,             // busca o inimigo mais próximo
    MOV_ORBIT,              // orbita o jogador
    MOV_BOOMERANG,          // vai e volta
    MOV_BOUNCE,             // ricocheteia nas bordas
    MOV_SPIRAL,             // espiral crescente
    MOV_FALL,               // cai do alto (telegrafado)
    MOV_TELEPORT,           // teletransporta a cada intervalo
    MOV_RANDOMWALK,         // anda em direção aleatória
    MOV_STATIONARY,         // parado (Aura/Beam fixo)
    MOV_MAX
};
const int MOV_TOTAL = MOV_MAX;

enum OrigemType {
    ORIG_STAND = 0,         // posição do Stand
    ORIG_PLAYER,            // posição do Jogador
    ORIG_CURSOR,            // posição do cursor do mouse
    ORIG_RANDOMMAP,         // posição aleatória no mapa
    ORIG_NEARESTENEMY,      // inimigo mais próximo do Stand
    ORIG_KILLEDENEMY,       // posição do último inimigo morto
    ORIG_ALLENEMIES,        // spawnado em CADA inimigo vivo
    ORIG_ORBITPOINT,        // ponto orbital ao redor do jogador
    ORIG_MAX
};

enum EfeitoType {
    EFE_NONE = 0,
    EFE_DAMAGE,             // dano direto
    EFE_DRAIN,              // drena tensão (vampírico)
    EFE_HEAL,               // cura o jogador
    EFE_BURN,               // queimadura (DoT de fogo)
    EFE_FREEZE,             // congelamento (slow)
    EFE_SHOCK,              // choque (stun + dano)
    EFE_KNOCKBACK,          // empurrão
    EFE_EXPLOSION,          // explosão ao acertar
    EFE_CHAINEXPLOSION,     // explosão em cadeia
    EFE_SPAWNSKILL,         // spawna outra skill ao acertar
    EFE_MAX
};

enum StackType {
    STACK_REFRESCA = 0,     // reinicia a duração (não acumula)
    STACK_ACUMULA,          // soma duração e magnitude
    STACK_LIMITADO,         // até maxStacks cópias simultâneas
    STACK_IGNORAR           // ignora se já aplicado
};

struct FormaData {
    FormaType tipo;

    float raioColisao;      // raio de colisão para formas circulares
    float comprimento;      // Beam/Wall: comprimento do segmento
    float largura;          // Beam/Wall/Wave: meia-largura de impacto
    float raioInterno;      // Arc: raio interno do setor
    float raioExterno;      // Arc: raio externo do setor
    float anguloAbertura;   // Arc/Cone: abertura total em radianos
    float spreadAngulo;     // Cone/Ring: spread entre sub-projéteis (rad)
    float alcanceMax;       // Projectile/Boomerang: distância máxima
    float duracao;          // Beam/Area/Aura: tempo de vida em segundos (0 = infinito)
    float tickIntervalo;    // Beam/Area/Aura: intervalo entre ticks de dano
    int   quantidade;       // Cone/Ring: número de sub-projéteis
    int   perfuracao;       // Projectile: quantos inimigos atravessa (0 = 1 alvo)
    int   saltosMax;        // Chain: número máximo de saltos
    int   geracoesMax;      // Prism: profundidade máxima de divisões (recursão segura)

    float corR, corG, corB;  // (0,0,0) = padrão amarelo em SkillRender.h
};

struct MovimentoData {
    MovimentoType tipo;

    float velocidade;       // Linear/Homing/Boomerang/Bounce/RandomWalk: px/s
    float taxaCorrecao;     // Homing: fator de correção de rota [0,1]/s
    float raioOrbita;       // Orbit: distância do centro
    float velocidadeAngular;// Orbit/Spiral: rad/s
    float velocidadeRadial; // Spiral: crescimento de raio px/s
    float ricochetesMax;    // Bounce: máximo de ricochetes
    float alturaInicial;    // Fall: altura de onde cai
    float velocidadeQueda;  // Fall: velocidade de descida px/s
    float tempoAviso;       // Fall: tempo de telegrafar antes de cair
    float intervalo;        // Teleport/RandomWalk: tempo entre ações
    bool  seguirOrigem;     // Stationary/Aura: segue a origem a cada frame
    float taxaEmissao;      // re-emissão periódica (separado do cooldown do slot)
};

struct OrigemData {
    OrigemType tipo;
    Vetor3D    offset;          // deslocamento adicional sobre a origem
    bool       reavaliarPorFrame; // true = reavalia centro a cada frame (Orbit/Aura)
};

struct EfeitoData {
    EfeitoType  tipo;
    int         valor;          // dano/cura/etc (inteiro para paridade com legado)
    float       magnitude;      // DoT dps / slow% / knockback distância
    float       duracao;        // duração do status em segundos
    StackType   empilhamento;
    int         maxStacks;      // STACK_LIMITADO: máximo de pilhas
    int         idSkillSpawn;   // EFE_SPAWNSKILL: qual skill spawnar
    int         propagacaoMax;  // EFE_CHAINEXPLOSION: máximo de propagações em cadeia
};

#define MAX_EFEITOS_SKILL      4
#define MAX_EFEITOS_POR_SKILL  MAX_EFEITOS_SKILL  // alias para SkillValidator

struct SkillData {
    int         id;             // índice na factory (preenchido por RegistrarSkill)
    bool        ativa;          // false = slot vazio no catálogo

    FormaData    forma;
    MovimentoData movimento;
    OrigemData    origem;

    EfeitoData  efeitos[MAX_EFEITOS_SKILL];
    int         numEfeitos;

    float       custoTensao;    // tensão gasta por disparo
    float       cooldown;       // segundos entre disparos automáticos (0 = manual)
    float       cooldownManual; // se > 0, sobrescreve baseCooldown do disparo manual em Main.cpp
};

// Ponto de partida de todas as builds: Projétil Linear, ORIG_STAND, 5 de dano, 10 de tensão.
inline SkillData buildBase() {
    SkillData s;
    char* p = (char*)&s;
    int i;
    for (i = 0; i < (int)sizeof(SkillData); ++i) p[i] = 0;

    s.id    = -1;
    s.ativa = true;

    s.forma.tipo          = FORMA_PROJECTILE;
    s.forma.raioColisao   = 0.25f;
    s.forma.alcanceMax    = 50.0f;
    s.forma.duracao       = 0.0f;  // sem expiração (some pela borda da arena)
    s.forma.perfuracao    = 0;
    s.forma.quantidade    = 1;
    s.forma.tickIntervalo = 0.25f;

    s.movimento.tipo       = MOV_LINEAR;
    s.movimento.velocidade = 52.0f;

    s.origem.tipo              = ORIG_STAND;
    s.origem.reavaliarPorFrame = false;

    s.efeitos[0].tipo  = EFE_DAMAGE;
    s.efeitos[0].valor = 5;
    s.numEfeitos       = 1;

    s.custoTensao = 10.0f;
    s.cooldown    = 0.0f;

    return s;
}

// RuntimeSkill depende dos enums acima — incluído após eles.
#include "RuntimeSkill.h"

#endif // SKILL_TYPES_H
