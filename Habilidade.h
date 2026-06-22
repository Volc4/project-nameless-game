#ifndef HABILIDADE_H
#define HABILIDADE_H

// ===========================================================================
//  ATENÇÃO — ARQUIVO LEGADO (NÃO INCLUIR)
//
//  Habilidade.h pertence à arquitetura pré-Fase 5 (polimorfismo com vtable).
//  Foi substituído pelo motor data-driven: SkillTypes.h + SkillExecutor.h +
//  SkillManager.h. Este arquivo referencia 'struct Projetil' e
//  'jogo.tirosNaTela' que foram REMOVIDOS de Entities.h.
//
//  Se este arquivo for incluído, o código NÃO compila.
//  Mantenha-o aqui apenas como referência histórica, sem incluí-lo.
//
//  O sistema atual usa:
//    RuntimeSkill  (em RuntimeSkill.h)  — substitui Projetil
//    SkillManager  (em SkillManager.h)  — substitui HabilidadeBase/subclasses
// ===========================================================================

// --- Bloco desabilitado para evitar erros de compilação ---
#if 0
//
//  FILOSOFIA DE DESIGN
//  -------------------
//  Cada "arma" ou "disparo especial" é uma Habilidade concreta que herda da
//  interface HabilidadeBase. O jogo nunca sabe qual tipo de habilidade está
//  ativa; ele só chama habilidade->executar(jogo, alvo).
//
//  COMPATIBILIDADE C++98
//  ----------------------
//  - Sem auto, sem lambda, sem nullptr, sem enum class, sem smart pointers.
//  - Polimorfismo via vtable (herança + virtual puro) — disponível em C++98.
//  - Ponteiros crus gerenciados pelo SkillManager (dono único).
//
//  EXTENSÃO FUTURA
//  ---------------
//  Para adicionar uma nova arma: crie uma struct que herde HabilidadeBase,
//  implemente executar() e clone() — zero alterações no restante do código.
// ===========================================================================

#include "Entities.h"
#include "MathUtils.h"
#include <cstdlib>
#include <cmath>

// ---------------------------------------------------------------------------
// AtributosHabilidade
//   Contêiner de dados puros que descreve uma habilidade configurada.
//   Preenchido pelo SkillManager com base nos upgrades do jogador.
//   Separa DADOS de COMPORTAMENTO: a Habilidade sabe O QUE fazer,
//   os atributos dizem COM QUE PARÂMETROS fazer.
// ---------------------------------------------------------------------------
struct AtributosHabilidade {
    int   dano;
    float velocidadeProjetil;
    float raioColisao;
    int   perfuracao;
    int   quantidadeTiros;
    float spreadAngulo;        // ângulo entre tiros em radianos
    bool  tiroRadial;          // true = distribui em 360°, false = em cone

    // Flags de efeito especial (para evitar enums de disparo)
    bool efeitoCuraVida;       // acertar tem chance de curar HP
    bool efeitoDrenarTensao;   // acertar reduz tensão do Stand

    AtributosHabilidade()
        : dano(1), velocidadeProjetil(20.0f), raioColisao(0.20f)
        , perfuracao(0), quantidadeTiros(1), spreadAngulo(0.15f)
        , tiroRadial(false), efeitoCuraVida(false), efeitoDrenarTensao(false)
    {}
};

// Declaração antecipada — HabilidadeBase precisa de EstadoDoJogo
// mas Entities.h não inclui GameLogic.h.
// A implementação das funções de morte/partícula fica em GameLogic.h,
// então as Habilidades concretas incluem GameLogic.h indiretamente via
// SkillManager.h. Aqui só precisamos do struct completo de EstadoDoJogo.
struct EstadoDoJogo; // já definido em Entities.h (incluído acima)

// ---------------------------------------------------------------------------
// HabilidadeBase — Interface polimórfica (C++98: herança + virtual puro)
//
//  Contrato mínimo que o SkillManager e o jogo precisam conhecer:
//    executar()  — dispara/ativa a habilidade no mundo
//    clone()     — cópia profunda para o SkillManager alocar cópias
//    destrutor virtual — obrigatório para delete via ponteiro base
// ---------------------------------------------------------------------------
struct HabilidadeBase {
    AtributosHabilidade atributos;

    // Construtor: recebe os atributos já calculados
    explicit HabilidadeBase(const AtributosHabilidade& a) : atributos(a) {}

    // Destrutor virtual obrigatório para delete via ponteiro base (C++98)
    virtual ~HabilidadeBase() {}

    // Executa a habilidade: gera projéteis em jogo.tirosNaTela
    // Entrada: jogo (estado mutável), posicaoAlvo (mundo)
    virtual void executar(EstadoDoJogo& jogo, Vetor3D posicaoAlvo) = 0;

    // Cria uma cópia alocada em heap — SkillManager é dono
    virtual HabilidadeBase* clone() const = 0;

protected:
    // -----------------------------------------------------------------
    // Auxiliar compartilhado: monta um Projetil a partir dos atributos
    // e da direção calculada. Reutilizado por todas as subclasses.
    // -----------------------------------------------------------------
    Projetil _fazerProjetil(const EstadoDoJogo& jogo,
                             Vetor3D posicaoOrigem,
                             Vetor3D direcao) const {
        Projetil p;
        p.posicao            = posicaoOrigem;
        p.direcao            = direcao;
        p.ativo              = true;
        p.dano               = atributos.dano;
        p.velocidade         = atributos.velocidadeProjetil;
        p.raioColisao        = atributos.raioColisao;
        p.perfuracaoRestante = atributos.perfuracao;
        return p;
    }

    // -----------------------------------------------------------------
    // Auxiliar: gira um vetor 2D (x,z) por `angulo` radianos.
    // Usado para distribuir tiros em cone ou radial.
    // -----------------------------------------------------------------
    static void _rotacionar(float& dx, float& dz, float angulo) {
        float cosA = cosf(angulo);
        float sinA = sinf(angulo);
        float nx   = dx * cosA - dz * sinA;
        float nz   = dx * sinA + dz * cosA;
        dx = nx;
        dz = nz;
    }
};

// ===========================================================================
//  HABILIDADES CONCRETAS
//
//  Convenção de nomenclatura:
//    Habilidade<NomeDescritivo>
//
//  Cada struct:
//    1. Herda HabilidadeBase
//    2. Implementa executar()  — toda a lógica de criação de projéteis
//    3. Implementa clone()     — return new HabilidadeXxx(*this)
//
//  A lógica de EFEITOS ESPECIAIS (cura, drenar tensão) é lida via flags
//  em AtributosHabilidade e processada em processarColisoesTiros_Grade()
//  no GameLogic.h/SpatialGrid.h — sem mudança naquelas funções.
// ===========================================================================

// ---------------------------------------------------------------------------
// HabilidadeTiroSimples
//   Um único projétil disparado em direção ao alvo.
//   Caso base: DISPARO_NORMAL, qualquer build sem evolução.
// ---------------------------------------------------------------------------
struct HabilidadeTiroSimples : HabilidadeBase {
    explicit HabilidadeTiroSimples(const AtributosHabilidade& a)
        : HabilidadeBase(a) {}

    virtual void executar(EstadoDoJogo& jogo, Vetor3D posicaoAlvo) {
        Vetor3D dir = obterDirecaoNormalizada(jogo.stand.posicao, posicaoAlvo);
        Projetil p  = _fazerProjetil(jogo, jogo.stand.posicao, dir);
        jogo.tirosNaTela.push_back(p);
    }

    virtual HabilidadeBase* clone() const {
        return new HabilidadeTiroSimples(*this);
    }
};

// ---------------------------------------------------------------------------
// HabilidadeCone
//   N tiros distribuídos em cone (spread simétrico ao redor da direção base).
//   Cobre: tiro duplo (N=2), espingarda (N=5), e builds intermediários.
//   Parametrizado por atributos.quantidadeTiros e atributos.spreadAngulo.
// ---------------------------------------------------------------------------
struct HabilidadeCone : HabilidadeBase {
    explicit HabilidadeCone(const AtributosHabilidade& a)
        : HabilidadeBase(a) {}

    virtual void executar(EstadoDoJogo& jogo, Vetor3D posicaoAlvo) {
        Vetor3D dirBase = obterDirecaoNormalizada(jogo.stand.posicao, posicaoAlvo);
        int     n       = atributos.quantidadeTiros;

        // Ângulo do tiro mais à esquerda do cone
        float anguloInicio = -((n - 1) * 0.5f) * atributos.spreadAngulo;

        for (int i = 0; i < n; ++i) {
            float dx = dirBase.x;
            float dz = dirBase.z;
            _rotacionar(dx, dz, anguloInicio + i * atributos.spreadAngulo);

            Vetor3D dir = {dx, 0.0f, dz};
            Projetil p  = _fazerProjetil(jogo, jogo.stand.posicao, dir);
            jogo.tirosNaTela.push_back(p);
        }
    }

    virtual HabilidadeBase* clone() const {
        return new HabilidadeCone(*this);
    }
};

// ---------------------------------------------------------------------------
// HabilidadeRadial
//   N tiros distribuídos em 360° (explosão estelar).
//   A direção base é ignorada — todos os ângulos são calculados
//   a partir de 0, divididos em N fatias iguais.
// ---------------------------------------------------------------------------
struct HabilidadeRadial : HabilidadeBase {
    explicit HabilidadeRadial(const AtributosHabilidade& a)
        : HabilidadeBase(a) {}

    virtual void executar(EstadoDoJogo& jogo, Vetor3D posicaoAlvo) {
        int   n    = atributos.quantidadeTiros;
        float step = (2.0f * 3.14159265f) / (float)n;

        for (int i = 0; i < n; ++i) {
            float ang = i * step;
            Vetor3D dir;
            dir.x = cosf(ang);
            dir.y = 0.0f;
            dir.z = sinf(ang);
            Projetil p = _fazerProjetil(jogo, jogo.stand.posicao, dir);
            jogo.tirosNaTela.push_back(p);
        }
    }

    virtual HabilidadeBase* clone() const {
        return new HabilidadeRadial(*this);
    }
};

// ---------------------------------------------------------------------------
// HabilidadePerfurante
//   Tiro único com altíssima perfuração — atravessa colunas de inimigos.
//   Derivado de TiroSimples, só ajusta os atributos durante a construção.
//   Separado como tipo próprio para clareza semântica.
// ---------------------------------------------------------------------------
struct HabilidadePerfurante : HabilidadeBase {
    explicit HabilidadePerfurante(const AtributosHabilidade& a)
        : HabilidadeBase(a) {}

    virtual void executar(EstadoDoJogo& jogo, Vetor3D posicaoAlvo) {
        Vetor3D dir = obterDirecaoNormalizada(jogo.stand.posicao, posicaoAlvo);
        Projetil p  = _fazerProjetil(jogo, jogo.stand.posicao, dir);
        jogo.tirosNaTela.push_back(p);
    }

    virtual HabilidadeBase* clone() const {
        return new HabilidadePerfurante(*this);
    }
};

// ---------------------------------------------------------------------------
// HabilidadePesada
//   Projétil único, lento e massivo — alta hitbox, alto dano.
//   Para builds DANO+VIDA ou PERFURACAO+VIDA.
// ---------------------------------------------------------------------------
struct HabilidadePesada : HabilidadeBase {
    explicit HabilidadePesada(const AtributosHabilidade& a)
        : HabilidadeBase(a) {}

    virtual void executar(EstadoDoJogo& jogo, Vetor3D posicaoAlvo) {
        Vetor3D dir = obterDirecaoNormalizada(jogo.stand.posicao, posicaoAlvo);
        Projetil p  = _fazerProjetil(jogo, jogo.stand.posicao, dir);
        jogo.tirosNaTela.push_back(p);
    }

    virtual HabilidadeBase* clone() const {
        return new HabilidadePesada(*this);
    }
};

// ---------------------------------------------------------------------------
// HabilidadeRapida
//   Projétil único ultrarrápido — alta velocidade, hitbox pequena.
//   Para builds VELOCIDADE+TENSAO ou DANO+VELOCIDADE.
// ---------------------------------------------------------------------------
struct HabilidadeRapida : HabilidadeBase {
    explicit HabilidadeRapida(const AtributosHabilidade& a)
        : HabilidadeBase(a) {}

    virtual void executar(EstadoDoJogo& jogo, Vetor3D posicaoAlvo) {
        Vetor3D dir = obterDirecaoNormalizada(jogo.stand.posicao, posicaoAlvo);
        Projetil p  = _fazerProjetil(jogo, jogo.stand.posicao, dir);
        jogo.tirosNaTela.push_back(p);
    }

    virtual HabilidadeBase* clone() const {
        return new HabilidadeRapida(*this);
    }
};

#endif // bloco desabilitado

#endif // HABILIDADE_H