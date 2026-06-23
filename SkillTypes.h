#ifndef SKILL_TYPES_H
#define SKILL_TYPES_H

// ===========================================================================
//  SkillTypes.h — Tipos de dados centrais do motor data-driven
//
//  Define todos os enums (FormaType, MovimentoType, OrigemType, EfeitoType,
//  StackType) e as structs de dados imutáveis (FormaData, MovimentoData,
//  OrigemData, EfeitoData, SkillData) que descrevem UMA skill como dados
//  puros. Nenhuma lógica de execução aqui — apenas a estrutura.
//
//  Também inclui RuntimeSkill.h no final (RuntimeSkill depende dos enums
//  definidos aqui, mas é uma struct separada de estado mutável).
//
//  C++98: POD puro, sem construtores, sem std::string, sem ponteiros donos.
// ===========================================================================

#include "Entities.h"   // Vetor3D, EstadoDoJogo, etc.
#include <cstring>      // memset

// ---------------------------------------------------------------------------
// FormaType — geometria/colisão da skill
// ---------------------------------------------------------------------------
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

// Aliases de compatibilidade: alguns módulos (SkillValidator) referenciam o
// sentinela como *_TOTAL. Mantemos um único valor canônico (FORMA_MAX) e
// expomos o alias como const, evitando duplicar o enum (sem risco de ODR:
// são constantes de integração com linkage interno por header).
const int FORMA_TOTAL = FORMA_MAX;

// ---------------------------------------------------------------------------
// MovimentoType — como a instância se move a cada frame
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// OrigemType — de onde a instância nasce no mundo
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// EfeitoType — o que acontece ao atingir um inimigo
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// StackType — como efeitos de status se acumulam
// ---------------------------------------------------------------------------
enum StackType {
    STACK_REFRESCA = 0,     // reinicia a duração (não acumula)
    STACK_ACUMULA,          // soma duração e magnitude
    STACK_LIMITADO,         // até maxStacks cópias simultâneas
    STACK_IGNORAR           // ignora se já aplicado
};

// ===========================================================================
//  FormaData — componente Forma (geometria + colisão)
// ===========================================================================
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
};

// ===========================================================================
//  MovimentoData — componente Movimento (como a instância se locomove)
// ===========================================================================
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
    bool  seguirOrigem;     // Stationary/Aura: se deve seguir a origem a cada frame

    // cooldown de re-emissão para emitters periódicos (separado do SlotSkill)
    float taxaEmissao;
};

// ===========================================================================
//  OrigemData — componente Origem (de onde a instância nasce)
// ===========================================================================
struct OrigemData {
    OrigemType tipo;
    Vetor3D    offset;          // deslocamento adicional sobre a origem
    bool       reavaliarPorFrame; // true = reavalia centro a cada frame (Orbit/Aura)
};

// ===========================================================================
//  EfeitoData — um efeito on-hit (array em SkillData)
// ===========================================================================
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

// ===========================================================================
//  SkillData — descrição COMPLETA e IMUTÁVEL de uma skill
//
//  POD puro: copiável por valor, sem construtores, sem destrutores,
//  serializável via memcpy. Todas as skills do catálogo são instâncias desta
//  struct. Uma SkillData nunca muda após ser registrada na factory.
// ===========================================================================

// Número máximo de efeitos on-hit por skill
#define MAX_EFEITOS_SKILL  4
// Alias de compatibilidade (SkillValidator usa este nome):
#define MAX_EFEITOS_POR_SKILL  MAX_EFEITOS_SKILL

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
};

// ---------------------------------------------------------------------------
// buildBase — SkillData zerada com valores-padrão seguros.
//   Equivale ao disparo básico (Projétil Linear, ORIG_STAND, 1 dano).
//   Todas as builds no SkillRegistry começam aqui.
// ---------------------------------------------------------------------------
inline SkillData buildBase() {
    SkillData s;
    // zera tudo (POD: ok com memset)
    char* p = (char*)&s;
    int i;
    for (i = 0; i < (int)sizeof(SkillData); ++i) p[i] = 0;

    s.id   = -1;
    s.ativa = true;

    // Forma: projétil padrão
    s.forma.tipo         = FORMA_PROJECTILE;
    s.forma.raioColisao  = 0.25f;
    s.forma.alcanceMax   = 50.0f;
    s.forma.duracao      = 0.0f;   // sem expiração por tempo (projétil some pela arena)
    s.forma.perfuracao   = 0;
    s.forma.quantidade   = 1;
    s.forma.tickIntervalo = 0.25f;

    // Movimento: linear padrão
    s.movimento.tipo      = MOV_LINEAR;
    s.movimento.velocidade = 20.0f;

    // Origem: Stand padrão
    s.origem.tipo              = ORIG_STAND;
    s.origem.offset.x          = 0.0f;
    s.origem.offset.y          = 0.0f;
    s.origem.offset.z          = 0.0f;
    s.origem.reavaliarPorFrame = false;

    // Efeito padrão: 1 de dano
    s.efeitos[0].tipo  = EFE_DAMAGE;
    s.efeitos[0].valor = 1;
    s.numEfeitos       = 1;

    // Custo e cooldown: modo manual, 1 de tensão
    s.custoTensao = 1.0f;
    s.cooldown    = 0.0f;

    return s;
}

// ---------------------------------------------------------------------------
// Inclui RuntimeSkill APÓS os enums/structs de SkillTypes estarem definidos.
// O guard de include em RuntimeSkill.h garante que não há inclusão dupla.
// ---------------------------------------------------------------------------
#include "RuntimeSkill.h"

#endif // SKILL_TYPES_H
