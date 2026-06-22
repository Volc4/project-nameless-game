#ifndef SKILL_TYPES_H
#define SKILL_TYPES_H

// ===========================================================================
//  SkillTypes.h — Tipos de dados do motor Data-Driven de habilidades
//
//  CAMADA 0 da nova arquitetura. Define APENAS dados (POD): enums de tipo e
//  as structs de configuração (*Data + SkillData) e de instância (RuntimeSkill).
//
//  REGRAS (ver documento de arquitetura, Partes 4 e 8):
//   - Tudo aqui é Plain Old Data: sem vtable, sem std::string, sem ponteiros
//     donos. Referências entre dados são por índice inteiro. Isso garante
//     memcpy, serialização binária trivial e armazenamento em std::vector sem
//     custo de construtor.
//   - Config (SkillData) é IMUTÁVEL em runtime. Estado vive em RuntimeSkill.
//   - Nenhuma camada (Forma/Movimento/Origem/Efeito) conhece a implementação
//     concreta de outra. Combináveis livremente.
//
//  COMPATIBILIDADE C++98:
//   - enum simples (sem enum class), prefixo por convenção (FORMA_, MOV_…).
//   - O valor 0 de cada enum é o caso mais simples e seguro, de modo que uma
//     SkillData zerada por memset já é o tiro básico válido.
// ===========================================================================

#include "Entities.h"   // Vetor3D

// ---------------------------------------------------------------------------
// FORMA — geometria/volume de colisão. "Que espaço a skill ocupa?"
// ---------------------------------------------------------------------------
enum FormaType {
    FORMA_PROJECTILE = 0,
    FORMA_BEAM,
    FORMA_CONE,
    FORMA_ARC,
    FORMA_WAVE,
    FORMA_AREA,
    FORMA_AURA,
    FORMA_PRISM,
    FORMA_RING,
    FORMA_EXPLOSION,
    FORMA_CHAIN,
    FORMA_WALL,
    FORMA_TOTAL          // sentinela — dimensiona matrizes; nunca é forma real
};

// ---------------------------------------------------------------------------
// MOVIMENTO — integração vetorial por frame. "Como anda?"
// ---------------------------------------------------------------------------
enum MovimentoType {
    MOV_LINEAR = 0,
    MOV_ORBIT,
    MOV_BOOMERANG,
    MOV_HOMING,
    MOV_SPIRAL,
    MOV_BOUNCE,
    MOV_FALL,
    MOV_TELEPORT,
    MOV_RANDOMWALK,
    MOV_STATIONARY,
    MOV_TOTAL
};

// ---------------------------------------------------------------------------
// ORIGEM — resolução de posição inicial e/ou alvo. "De onde nasce, mira em quê?"
// ---------------------------------------------------------------------------
enum OrigemType {
    ORIG_PLAYER = 0,
    ORIG_STAND,
    ORIG_CURSOR,
    ORIG_RANDOMMAP,
    ORIG_KILLEDENEMY,
    ORIG_NEARESTENEMY,
    ORIG_ALLENEMIES,
    ORIG_ORBITPOINT,
    ORIG_TOTAL
};

// ---------------------------------------------------------------------------
// EFEITO — consequência on-hit/on-tick no alvo. "O que faz?"
// ---------------------------------------------------------------------------
enum EfeitoType {
    EFE_DAMAGE = 0,
    EFE_DRAIN,         // reduz tensão do Stand (antigo efeitoDrenarTensao)
    EFE_HEAL,          // cura HP do jogador (antigo efeitoCuraVida)
    EFE_BURN,
    EFE_FREEZE,
    EFE_SHOCK,
    EFE_KNOCKBACK,
    EFE_EXPLOSION,
    EFE_CHAINEXPLOSION,
    EFE_SPAWNSKILL,
    EFE_TOTAL
};

// ---------------------------------------------------------------------------
// Política de empilhamento de um efeito repetido sobre o mesmo alvo.
// ---------------------------------------------------------------------------
enum StackPolicy {
    STACK_SOMA = 0,    // valores somam (dano)
    STACK_REFRESCA,    // renova duração, não soma (freeze)
    STACK_LIMITADO     // soma até maxStacks (burn)
};

// ===========================================================================
//  STRUCTS DE CAMADA (POD)
//
//  Campos genéricos nomeados (não union): cada *Type interpreta o subconjunto
//  que lhe interessa. O overhead de memória é irrelevante para um catálogo de
//  dezenas de skills, e a clareza/auto-documentação compensa.
// ===========================================================================

// ---------------------------------------------------------------------------
// FormaData — geometria e volume.
// ---------------------------------------------------------------------------
struct FormaData {
    FormaType tipo;            // qual forma
    float raioColisao;         // Projectile/Ring/Area/Explosion/Aura
    float largura;             // Beam/Wave/Wall (espessura)
    float comprimento;         // Beam/Wall
    float alcanceMax;          // Projectile range, Wave, Boomerang
    int   quantidade;          // Cone/Ring/Projectile múltiplo
    float spreadAngulo;        // Cone/Ring distribuição angular
    int   perfuracao;          // Projectile: nº de atravessamentos
    float duracao;             // Beam/Area/Aura/Wall/Arc persistência
    float tickIntervalo;       // formas contínuas: intervalo entre danos
    int   numDivisoes;         // Prism: filhos por split
    int   geracoesMax;         // Prism/recursão: limite de profundidade
    int   saltosMax;           // Chain
    float raioInterno;         // Arc
    float raioExterno;         // Arc
    float anguloAbertura;      // Arc/Cone
};

// ---------------------------------------------------------------------------
// MovimentoData — integração vetorial.
// ---------------------------------------------------------------------------
struct MovimentoData {
    MovimentoType tipo;
    float velocidade;          // Linear/Boomerang/Homing/Bounce/RandomWalk
    float velocidadeAngular;   // Orbit/Spiral/Ring
    float velocidadeRadial;    // Spiral
    float raioOrbita;          // Orbit/Ring
    float taxaCorrecao;        // Homing (0..1 por segundo)
    int   ricochetesMax;       // Bounce
    float alturaInicial;       // Fall
    float velocidadeQueda;     // Fall
    float tempoAviso;          // Fall (telegrafo antes de ativar)
    float intervalo;           // Teleport/RandomWalk
    bool  seguirOrigem;        // Stationary/Aura: cola na origem
};

// ---------------------------------------------------------------------------
// OrigemData — resolução de posição/alvo.
// ---------------------------------------------------------------------------
struct OrigemData {
    OrigemType tipo;
    Vetor3D    offset;             // deslocamento fixo sobre a posição resolvida
    bool       reavaliarPorFrame;  // true: re-resolve a cada frame (Player/Aura)
    float      intervaloReaquisicao; // Homing/Nearest: re-mira a cada X s
};

// ---------------------------------------------------------------------------
// EfeitoData — uma consequência. SkillData tem um array fixo (POD).
// ---------------------------------------------------------------------------
struct EfeitoData {
    EfeitoType  tipo;
    int         valor;         // Damage/Shock dano, Heal cura, Drain tensão
    float       chance;        // 0..1 (Heal proc, crit)
    float       duracao;       // Burn/Freeze/Shock status
    float       magnitude;     // dps (Burn), slow% (Freeze), força (Knockback)
    StackPolicy empilhamento;  // como acumula
    int         prioridade;    // ordem de resolução (maior primeiro)
    int         idSkillFilha;  // SpawnSkill/Explosion: índice no catálogo (-1=nenhum)
    int         maxStacks;     // STACK_LIMITADO
    int         propagacaoMax; // ChainExplosion
};

#define MAX_EFEITOS_POR_SKILL 4

// ---------------------------------------------------------------------------
// SkillData — a build completa. IMUTÁVEL após montada. POD puro.
//   É o "documento" que descreve uma habilidade inteira por composição das
//   quatro camadas. Mil instâncias podem compartilhar uma SkillData via índice.
// ---------------------------------------------------------------------------
struct SkillData {
    int           id;          // índice próprio no catálogo
    FormaData     forma;
    MovimentoData movimento;
    OrigemData    origem;
    EfeitoData    efeitos[MAX_EFEITOS_POR_SKILL];
    int           numEfeitos;  // efeitos válidos em uso (<= MAX_EFEITOS_POR_SKILL)
    float         cooldown;    // tempo entre disparos automáticos (0 = manual)
    float         custoTensao; // custo de tensão por disparo (antigo +5.0f)
    bool          ativa;       // build habilitada para o jogador
};

// ===========================================================================
//  ESTRUTURAS RUNTIME — definidas em RuntimeSkill.h
//
//  RuntimeSkill, RuntimeEmitter e RuntimeEffect foram movidas para
//  RuntimeSkill.h (versão completa com jaAcertados[], timerMovimento,
//  timerReaquisicao, etc.). Incluímos aqui para que qualquer header que
//  inclua SkillTypes.h continue vendo as structs runtime sem alteração.
// ===========================================================================
#include "RuntimeSkill.h"

#endif // SKILL_TYPES_H