#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include "Entities.h"
#include "MathUtils.h"
#include "SkillManager.h"
#include "ProgressionSystem.h"
#include <cstdlib>
#include <cmath>

// ===========================================================================
// CONSTANTES DO SISTEMA DE TENSÃO E PARRY
// ===========================================================================
const float TAXA_TENSAO_TIRO          = 18.0f;
const float TAXA_TENSAO_REPOUSO       =  5.0f;
const float TAXA_DRENAGEM_SOBRECARGA  = 12.0f;
const float FATOR_DANO_SOBRECARGA     = 0.25f;

const float JANELA_PARRY_SEGUNDOS   = 0.25f;
const float COOLDOWN_PARRY_SEGUNDOS = 1.5f;
const float RAIO_EXPULSAO_PARRY     = 15.0f;

// ===========================================================================
// CONSTANTES DO SISTEMA DE UPGRADES
// ===========================================================================
const int NIVEL_MAXIMO_UPGRADE = 3;

// Valores base antes de qualquer upgrade
const int   DANO_BASE            = 1;
const float CADENCIA_COOLDOWN_BASE = 0.0f;  // Sem cooldown automático no sistema atual (clique manual)
const int   PERFURACAO_BASE      = 0;
const float VELOCIDADE_BASE      = 10.0f;
const int   HP_BASE              = 3;

// Ganhos por nível de upgrade
const int   GANHO_DANO_POR_NIVEL       = 1;    // +1 de dano por nível
const float GANHO_TENSAO_REDUCAO       = 2.0f; // -2s de cooldown do parry por nível
const int   GANHO_PERFURACAO_POR_NIVEL = 1;    // +1 de perfuração por nível
const float GANHO_VELOCIDADE_POR_NIVEL = 2.0f; // +2 de velocidade por nível
const int   GANHO_HP_POR_NIVEL         = 1;    // +1 de HP máximo por nível

// ===========================================================================
// CÁLCULO DINÂMICO DOS ATRIBUTOS
// Cada função transforma o nível em valor prático, centralizando a lógica.
// ===========================================================================

// Entrada: nível do atributo DANO (0–3)
// Saída:   dano base dos projéteis
inline int calcularDanoBase(int nivelDano) {
    if (nivelDano == 1) return 3;  // Mata zumbis normais (HP 1) e atiradores (HP 2) num hit
    if (nivelDano == 2) return 7;  // Quase mata o Tank (HP 10) num hit
    if (nivelDano >= 3) return 20; // Hit-Kill em quase tudo, poder absoluto
    
    return 1; // Nível 0 (Base)
}

// Entrada: nível do atributo CADENCIA (0–3)
// Saída:   quantidade de projéteis disparados por clique
inline int calcularQuantidadeTiros(int nivelCadencia) {
    if (nivelCadencia == 1) return 2; // Tiro duplo em "V"
    if (nivelCadencia == 2) return 5; // Espingarda (Cone de 5 tiros)
    if (nivelCadencia >= 3) return 8; // Explosão Estelar (8 tiros em todas as direções)
    
    return 1; // Nível 0 (Base - 1 tiro reto)
}

// Entrada: nível do atributo DANO (0–3)
// Saída:   multiplicador de área/tamanho do projétil
inline float calcularMultiplicadorTamanho(int nivelDano) {
    if (nivelDano == 1) return 1.5f; // 50% maior
    if (nivelDano == 2) return 2.2f; // Mais que o dobro do tamanho
    if (nivelDano >= 3) return 3.5f; // Projéteis massivos
    
    return 1.0f; // Nível 0 (Base)
}

// Entrada: nível do atributo PERFURACAO (0–3)
// Saída:   número de inimigos extras que o projétil pode atravessar
// Entrada: nível do atributo PERFURACAO (0–3)
// Saída:   número de inimigos extras que o projétil pode atravessar
inline int calcularPerfuracaoBase(int nivelPerfuracao) {
    if (nivelPerfuracao == 1) return 1;    // Atravessa 1 inimigo (acerta 2 no total)
    if (nivelPerfuracao == 2) return 5;    // Atravessa 5 inimigos
    if (nivelPerfuracao >= 3) return 9999; // Perfuração Infinita
    
    return 0; // Nível 0 (não atravessa ninguém)
}

// Entrada: nível do atributo VELOCIDADE (0–3)
// Saída:   velocidade de movimento do jogador
inline float calcularVelocidadeJogador(int nivelVelocidade) {
    if (nivelVelocidade == 1) return 13.0f; // Confortável para fugir (+30%)
    if (nivelVelocidade == 2) return 18.0f; // Muito ágil, escapa facilmente de encurralamentos
    if (nivelVelocidade >= 3) return 28.0f; // Velocidade extrema, cruza o mapa instantaneamente
    
    return 10.0f; // Nível 0 (Base)
}

// Entrada: nível do atributo TENSAO_UP (0–3)
// Saída:   cooldown efetivo do Parry
inline float calcularCooldownParry(int nivelTensao) {
    if (nivelTensao == 1) return 1.0f;  // Reduz 0.5s (Uso mais tático)
    if (nivelTensao == 2) return 0.7f;  // Quase sem recarga
    if (nivelTensao >= 3) return 0.4f; // Spam infinito (Parry metralhadora)
    
    return 1.5f; // Nível 0 (Base)
}

// Entrada: nível do atributo TENSAO_UP (0–3)
// Saída:   fator de redução da taxa de acúmulo de tensão por tiro (0.0 = sem redução)
// Entrada: nível do atributo TENSAO_UP (0–3)
// Saída:   fator de redução da taxa de acúmulo de tensão por tiro
inline float calcularReducaoTensaoPorTiro(int nivelTensao) {
    if (nivelTensao == 1) return 0.20f; // 30% menos tensão gerada ao atirar
    if (nivelTensao == 2) return 0.50f; // 70% menos tensão (Pode atirar à vontade)
    if (nivelTensao >= 3) return 0.70f;  // 100% de redução (Atirar não gera MAIS NENHUMA tensão, a arma esfria)
    
    return 0.0f; // Nível 0 (Base)
}

// Entrada: nível do atributo VIDA (0–3)
// Saída:   HP máximo do jogador
inline int calcularHPMaximo(int nivelVida) {
    if (nivelVida == 1) return 5;  // +2 HP (Uma pequena folga)
    if (nivelVida == 2) return 7;  // +6 HP (Resistente a grandes erros)
    if (nivelVida >= 3) return 10; // +17 HP (O verdadeiro "Survivor")
    
    return 3; // Nível 0 (Base)
}

// ===========================================================================
// SEÇÃO: DETERMINAÇÃO DO TIPO DE DISPARO
//   Removida — substituída pelo SkillManager (Habilidade.h + SkillManager.h).
//   O dispatch de projéteis agora é feito via skillManager.executar().
// ===========================================================================

// ===========================================================================
// SEÇÃO: SISTEMA DE UPGRADES
// ===========================================================================

// Verifica se ainda existem atributos disponíveis (nível < NIVEL_MAXIMO_UPGRADE)
// Entrada: upgrades do jogador
// Saída:   true se ao menos um atributo pode subir de nível
inline bool existemUpgradesDisponiveis(const SistemaUpgrades& upgrades) {
    for (int i = 0; i < TOTAL_UPGRADES; ++i) {
        if (upgrades.niveis[i] < NIVEL_MAXIMO_UPGRADE)
            return true;
    }
    return false;
}

// Sorteia até 3 opções de upgrade sem repetição, excluindo atributos já no máximo.
// Entrada: jogo (para ler níveis e preencher opcoesUpgrade / quantidadeOpcoes)
// Saída:   modifica jogo.opcoesUpgrade e jogo.quantidadeOpcoes in-place
inline void sortearUpgrades(EstadoDoJogo& jogo) {
    // FASE 6: delega ao ProgressionSystem o sorteio dos 3 tipos de recompensa.
    // O resultado fica em jogo.menuAtual (MenuLevelUp), não mais em opcoesUpgrade[].
    sortearRecompensas(jogo.menuAtual, jogo.inventario);
    jogo.quantidadeOpcoes = jogo.menuAtual.quantidade;   // compatibilidade legado
}

// ---------------------------------------------------------------------------
// SUBSTITUI aplicarUpgrade(EstadoDoJogo&, TipoUpgrade)
//
// A assinatura legada é mantida para retrocompatibilidade com qualquer código
// que ainda a chame. Internamente, ela delega ao ProgressionSystem caso a
// escolha venha do menuAtual. Se `tipo` for chamado diretamente (ex: testes),
// cria uma LevelUpChoice de atributo global e aplica.
// ---------------------------------------------------------------------------
inline void aplicarUpgrade(EstadoDoJogo& jogo, TipoUpgrade tipo) {
    LevelUpChoice escolha;
    escolha.tipo       = ESCOLHA_ATRIBUTO_GLOBAL;
    escolha.raridade   = RARIDADE_COMUM;
    escolha.referencia = (int)tipo;
    escolha.valorExtra = 1;
    escolha.descricao[0] = '\0';
    escolha.subtitulo[0] = '\0';

    // skillManager é global em Main.cpp — forward declaration evita include circular.
    extern SkillManager skillManager;
    aplicarEscolhaLevelUp(escolha,
                          jogo.inventario,
                          jogo.protagonista.upgrades,
                          skillManager,
                          jogo.protagonista);
}

// Aplica o upgrade escolhido ao estado do jogo e atualiza todos os atributos.
// Entrada: jogo (estado mutável), tipo (atributo a subir de nível)
// Saída:   modifica jogo in-place; não retorna nada
inline void aplicarEscolhaMenu(EstadoDoJogo& jogo, int indiceEscolha) {
    if (indiceEscolha < 0 || indiceEscolha >= jogo.menuAtual.quantidade) return;
    const LevelUpChoice& escolha = jogo.menuAtual.escolhas[indiceEscolha];

    extern SkillManager skillManager;
    aplicarEscolhaLevelUp(escolha,
                          jogo.inventario,
                          jogo.protagonista.upgrades,
                          skillManager,
                          jogo.protagonista);

    // Limpa o estado do Tensao_UP se foi atualizado (parry cooldown)
    if (escolha.tipo == ESCOLHA_ATRIBUTO_GLOBAL &&
        escolha.referencia == (int)TENSAO_UP) {
        jogo.stand.cooldownParry = calcularCooldownParry(
            jogo.protagonista.upgrades.niveis[TENSAO_UP]);
        if (jogo.stand.temporizadorCooldown > jogo.stand.cooldownParry)
            jogo.stand.temporizadorCooldown = jogo.stand.cooldownParry;
    }
}
// ===========================================================================
// SEÇÃO: MOVIMENTAÇÃO E ENTIDADES
// ===========================================================================

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
    const float raioSpawn    = 30.0f;
    float anguloGraus        = (float)(rand() % 360);
    float anguloRadianos     = anguloGraus * (3.14159f / 180.0f);

    Vetor3D novaPosicao;
    novaPosicao.x = posJogador.x + (raioSpawn * cos(anguloRadianos));
    novaPosicao.y = 0.0f;
    novaPosicao.z = posJogador.z + (raioSpawn * sin(anguloRadianos));
    return novaPosicao;
}

// Função "Fábrica": Cria e configura um zumbi dependendo do tipo escolhido.
inline Zumbi invocarZumbi(TipoZumbi tipoDesejado, Vetor3D posicaoInicial) {
    Zumbi z;
    z.posicao     = posicaoInicial;
    z.tipo        = tipoDesejado;
    z.estadoAtual = WANDER;
    z.vivo        = true;

    switch (tipoDesejado) {
        case NORMAL:
            z.velocidade = 2.0f; z.raioColisao = 0.75f; z.vida = 1;  break;
        case RAPIDO:
            z.velocidade = 4.5f; z.raioColisao = 0.55f; z.vida = 1;  break;
        case TANK:
            z.velocidade = 1.2f; z.raioColisao = 1.10f; z.vida = 10; break;
        case ATIRADOR:
            z.velocidade = 1.8f; z.raioColisao = 0.75f; z.vida = 2;  break;
        case EXPLOSIVO:
            z.velocidade = 2.5f; z.raioColisao = 0.85f; z.vida = 1;  break;
    }
    return z;
}

// Sorteia o tipo de zumbi baseado no tempo de sobrevivencia
inline TipoZumbi sortearTipoZumbi(float tempoSegundos) {
    int chance = rand() % 100;

    if (tempoSegundos >= 330.0f) {
        if (chance < 50) return NORMAL;
        if (chance < 70) return RAPIDO;
        if (chance < 80) return TANK;
        if (chance < 90) return ATIRADOR;
        return EXPLOSIVO;
    } else if (tempoSegundos >= 240.0f) {
        if (chance < 60) return NORMAL;
        if (chance < 80) return RAPIDO;
        if (chance < 90) return TANK;
        return ATIRADOR;
    } else if (tempoSegundos >= 150.0f) {
        if (chance < 70) return NORMAL;
        if (chance < 90) return RAPIDO;
        return TANK;
    } else if (tempoSegundos >= 60.0f) {
        if (chance < 80) return NORMAL;
        return RAPIDO;
    }
    return NORMAL;
}

// Gerencia a dificuldade progressiva e instancia novos zumbis na horda
inline void processarSpawn(EstadoDoJogo& jogo, float deltaTime) {
    const float COOLDOWN_BASE   = 3.0f;
    const int   QUANTIDADE_BASE = 1;

    float fatorDificuldade   = 1.0f + (jogo.tempoSobrevivido / 60.0f);
    jogo.cooldownAtual       = COOLDOWN_BASE / fatorDificuldade;
    if (jogo.cooldownAtual < 0.2f) jogo.cooldownAtual = 0.2f;

    jogo.quantidadeSpawnAtual = (int)(QUANTIDADE_BASE * fatorDificuldade);

    if (jogo.tempoSobrevivido - jogo.tempoUltimoSpawn >= jogo.cooldownAtual) {
        jogo.tempoUltimoSpawn = jogo.tempoSobrevivido;

        for (int i = 0; i < jogo.quantidadeSpawnAtual; i++) {
            TipoZumbi tipoSorteado = sortearTipoZumbi(jogo.tempoSobrevivido);
            Vetor3D   posicaoSpawn = calcularPosicaoSpawnOculta(jogo.protagonista.posicao);
            Zumbi     novoZumbi   = invocarZumbi(tipoSorteado, posicaoSpawn);
            jogo.horda.push_back(novoZumbi);
        }
    }
}

// Atualiza a mente e a posição de todos os zumbis da horda
inline void processarIA(EstadoDoJogo& jogo, float deltaTime) {
    for (size_t i = 0; i < jogo.horda.size(); ++i) {
        Zumbi& z = jogo.horda[i];
        if (!z.vivo) continue;

        z.estadoAtual = CHASE;
        Vetor3D direcao = obterDirecaoNormalizada(z.posicao, jogo.protagonista.posicao);
        z.posicao.x += direcao.x * z.velocidade * deltaTime;
        z.posicao.z += direcao.z * z.velocidade * deltaTime;
    }
}

// ===========================================================================
// SEÇÃO: SISTEMA DE TENSÃO E PARRY
// ===========================================================================

inline void atualizarTensao(EstadoDoJogo& jogo, float deltaTime) {
    Entidade& stand = jogo.stand;

    // Fator de redução de tensão baseado no upgrade de TENSAO_UP
    float reducaoFator = calcularReducaoTensaoPorTiro(
        jogo.protagonista.upgrades.niveis[TENSAO_UP]);
    // Taxa efetiva: se reducao = 0.4, taxa real = 18 * (1 - 0.4) = 10.8
    float taxaTiroEfetiva = TAXA_TENSAO_TIRO * (1.0f - reducaoFator);

    if (!stand.emSobrecarga) {
        if (jogo.atirandoAgora)
            stand.tensaoAtual += taxaTiroEfetiva * deltaTime;
        else
            stand.tensaoAtual -= TAXA_TENSAO_REPOUSO * deltaTime;

        if (stand.tensaoAtual < 0.0f) stand.tensaoAtual = 0.0f;

        if (stand.tensaoAtual >= 100.0f) {
            stand.tensaoAtual  = 100.0f;
            stand.emSobrecarga = true;
            stand.parryAtivo   = false;
        }
    } else {
        stand.tensaoAtual -= TAXA_DRENAGEM_SOBRECARGA * deltaTime;
        if (stand.tensaoAtual <= 0.0f) {
            stand.tensaoAtual  = 0.0f;
            stand.emSobrecarga = false;
        }
    }
}

inline void aplicarEfeitoAreaParry(EstadoDoJogo& jogo) {
    const float FORCA_KNOCKBACK = 20.0f;

    for (size_t i = 0; i < jogo.horda.size(); ++i) {
        Zumbi& z = jogo.horda[i];
        if (!z.vivo) continue;

        float distQuad = calcularDistanciaQuadrada(z.posicao, jogo.stand.posicao);
        float raioQuad = RAIO_EXPULSAO_PARRY * RAIO_EXPULSAO_PARRY;

        if (distQuad <= raioQuad) {
            Vetor3D direcaoFuga = obterDirecaoNormalizada(jogo.stand.posicao, z.posicao);
            z.posicao.x += direcaoFuga.x * FORCA_KNOCKBACK;
            z.posicao.z += direcaoFuga.z * FORCA_KNOCKBACK;
        }
    }
}

inline void tentarAtivarParry(Entidade& stand) {
    if (stand.emSobrecarga) return;
    if (stand.temporizadorCooldown > 0.0f) return;
    if (stand.parryAtivo) return;

    stand.parryAtivo        = true;
    stand.temporizadorParry = JANELA_PARRY_SEGUNDOS;
}

inline bool tentarExecutarParry(EstadoDoJogo& jogo) {
    Entidade& stand = jogo.stand;
    if (!stand.parryAtivo) return false;

    const float RAIO_IMINENCIA_QUAD = 6.0f * 6.0f;
    bool ameacaDetectada = false;

    for (size_t i = 0; i < jogo.horda.size(); ++i) {
        Zumbi& z = jogo.horda[i];
        if (!z.vivo) continue;
        float dist = calcularDistanciaQuadrada(z.posicao, jogo.protagonista.posicao);
        if (dist <= RAIO_IMINENCIA_QUAD) {
            ameacaDetectada = true;
            break;
        }
    }

    if (ameacaDetectada) {
        aplicarEfeitoAreaParry(jogo);
        stand.tensaoAtual          = 0.0f;
        stand.parryAtivo           = false;
        stand.temporizadorParry    = 0.0f;
        stand.temporizadorCooldown = jogo.stand.cooldownParry;
        stand.parryBemSucedido     = true;
        stand.temporizadorFeedback = 0.4f;
        return true;
    }
    return false;
}

inline void atualizarTimersStand(EstadoDoJogo& jogo, float deltaTime) {
    Entidade& stand = jogo.stand;

    if (stand.parryAtivo) {
        stand.temporizadorParry -= deltaTime;
        if (stand.temporizadorParry <= 0.0f) {
            stand.parryAtivo           = false;
            stand.temporizadorParry    = 0.0f;
            stand.temporizadorCooldown = stand.cooldownParry * 0.5f;
        }
    }

    if (stand.temporizadorCooldown > 0.0f) {
        stand.temporizadorCooldown -= deltaTime;
        if (stand.temporizadorCooldown < 0.0f) stand.temporizadorCooldown = 0.0f;
    }

    if (stand.temporizadorFeedback > 0.0f) {
        stand.temporizadorFeedback -= deltaTime;
        if (stand.temporizadorFeedback < 0.0f) {
            stand.temporizadorFeedback = 0.0f;
            stand.parryBemSucedido     = false;
        }
    }
}

// ===========================================================================
// SEÇÃO: DISPARO (Lógica de instância migrada para o SkillManager)
// ===========================================================================

// (atualizarProjeteis REMOVIDO — o movimento dos projéteis agora é produzido
//  pelo executor data-driven, MOV_LINEAR em SkillExecutor.h. A entidade ativa
//  passou a ser RuntimeSkill, não mais Projetil/tirosNaTela.)
// ===========================================================================
// SEÇÃO: COLISÕES
// ===========================================================================

// Verifica se um tiro já acertou um determinado zumbi neste frame.
// Usada para evitar que o mesmo tiro acerte o mesmo inimigo duas vezes.
// Entrada: indicesJaAcertados (array de índices), quantidade, indiceZumbi
// Saída:   true se indiceZumbi já está no array
inline bool jaAcertouEsteZumbi(const size_t* indicesJaAcertados, int quantidade, size_t indiceZumbi) {
    for (int i = 0; i < quantidade; ++i) {
        if (indicesJaAcertados[i] == indiceZumbi)
            return true;
    }
    return false;
}

// Dropa gema ao matar zumbi.
// Entrada: jogo, zumbi morto
// Saída:   pode adicionar GemaXP a jogo.gemas
inline void processarMorteZumbi(EstadoDoJogo& jogo, Zumbi& z) {
    z.vivo = false;

    // Fase 3 — registra posição da última morte para efeitos futuros (hotspot, loot, etc.)
    jogo.houveMorteRecente   = true;
    jogo.ultimaPosicaoMorte  = z.posicao;

    if ((rand() % 100) < 70) {
        GemaXP gema;
        gema.posicao  = z.posicao;
        gema.coletada = false;
        switch (z.tipo) {
            case TANK:      gema.valorXP = 10; break;
            case ATIRADOR:  gema.valorXP = 5;  break;
            case EXPLOSIVO: gema.valorXP = 7;  break;
            case RAPIDO:    gema.valorXP = 3;  break;
            default:        gema.valorXP = 2;  break;
        }
        jogo.gemas.push_back(gema);
    }
}

// ===========================================================================
// SEÇÃO: SISTEMA DE PARTÍCULAS
//
// Sistema simples de partículas cosméticas (sem textura, sem física complexa).
// As partículas nunca afetam colisão, dano ou qualquer outra regra de jogo;
// elas existem apenas para dar feedback visual de impacto/morte.
// ===========================================================================

// Gera uma pequena explosão de partículas na posição informada.
// Entrada: jogo (para empurrar em jogo.particulas), posicaoOrigem (centro da
//          explosão, tipicamente a posição do zumbi morto), corR/corG/corB
//          (cor-base das partículas, geralmente a cor do inimigo morto).
// Saída:   adiciona entre 10 e 30 partículas a jogo.particulas.
// Funcionamento: cada partícula recebe uma direção aleatória no plano XZ
//          (com um pequeno componente vertical Y para dar volume 3D), uma
//          velocidade aleatória, um tempo de vida aleatório, e uma leve
//          variação de cor em torno da cor-base para dar mais textura visual.
inline void criarParticulasMorte(EstadoDoJogo& jogo, Vetor3D posicaoOrigem,
                                  float corR, float corG, float corB) {
    const int MIN_PARTICULAS = 10;
    const int MAX_PARTICULAS = 30;
    int quantidade = MIN_PARTICULAS + (rand() % (MAX_PARTICULAS - MIN_PARTICULAS + 1));

    for (int i = 0; i < quantidade; ++i) {
        Particula p;
        p.posicao = posicaoOrigem;
        p.posicao.y += 0.4f; // Origem um pouco acima do chão (altura do "torso")

        // Direção aleatória no plano XZ (ângulo completo 0-360°)
        float anguloGraus = (float)(rand() % 360);
        float anguloRad   = anguloGraus * (3.14159f / 180.0f);

        // Velocidade aleatória entre 1.5 e 6.0 unidades/seg
        float velPlano = 1.5f + ((rand() % 100) / 100.0f) * 4.5f;

        // Pequeno componente vertical aleatório para dar volume à explosão
        float velY = 1.0f + ((rand() % 100) / 100.0f) * 3.0f;

        p.velocidade.x = cosf(anguloRad) * velPlano;
        p.velocidade.z = sinf(anguloRad) * velPlano;
        p.velocidade.y = velY;

        // Tempo de vida aleatório entre 0.4s e 1.0s
        p.tempoVidaMaximo = 0.4f + ((rand() % 100) / 100.0f) * 0.6f;
        p.tempoVida        = p.tempoVidaMaximo;

        // Variação leve de cor em torno da cor-base (mantém identidade do inimigo)
        float variacao = ((rand() % 100) / 100.0f) * 0.25f - 0.125f; // -0.125..+0.125
        p.corR = corR + variacao; if (p.corR < 0.0f) p.corR = 0.0f; if (p.corR > 1.0f) p.corR = 1.0f;
        p.corG = corG + variacao; if (p.corG < 0.0f) p.corG = 0.0f; if (p.corG > 1.0f) p.corG = 1.0f;
        p.corB = corB + variacao; if (p.corB < 0.0f) p.corB = 0.0f; if (p.corB > 1.0f) p.corB = 1.0f;

        p.tamanho       = 0.06f + ((rand() % 100) / 100.0f) * 0.10f; // 0.06–0.16
        p.transparencia = 1.0f;
        p.ativa         = true;

        jogo.particulas.push_back(p);
    }
}

// Atualiza posição, gravidade leve e tempo de vida de todas as partículas.
// Entrada: jogo (vetor jogo.particulas), deltaTime (segundos desde o último frame)
// Saída:   modifica cada Particula in-place; desativa as que expiraram.
//          Chamada exclusivamente em timer() — nunca desenha nada.
inline void atualizarParticulas(EstadoDoJogo& jogo, float deltaTime) {
    const float GRAVIDADE_LEVE = 4.0f; // unidades/seg^2, puramente estética

    for (size_t i = 0; i < jogo.particulas.size(); ++i) {
        Particula& p = jogo.particulas[i];
        if (!p.ativa) continue;

        // Gravidade opcional leve no eixo Y
        p.velocidade.y -= GRAVIDADE_LEVE * deltaTime;

        p.posicao.x += p.velocidade.x * deltaTime;
        p.posicao.y += p.velocidade.y * deltaTime;
        p.posicao.z += p.velocidade.z * deltaTime;

        // Nunca deixa a partícula afundar visualmente abaixo do chão
        if (p.posicao.y < 0.0f) p.posicao.y = 0.0f;

        p.tempoVida -= deltaTime;
        if (p.tempoVida <= 0.0f) {
            p.tempoVida   = 0.0f;
            p.ativa       = false;
            p.transparencia = 0.0f;
            continue;
        }

        // Transparência decai linearmente com o tempo de vida restante
        p.transparencia = p.tempoVida / p.tempoVidaMaximo;
    }
}

// Remove partículas inativas do vetor (mantém o vetor enxuto).
// Entrada: jogo
// Saída:   jogo.particulas passa a conter apenas partículas ativas.
inline void limparParticulasInativas(EstadoDoJogo& jogo) {
    std::vector<Particula> particulasAtivas;
    for (size_t i = 0; i < jogo.particulas.size(); ++i) {
        if (jogo.particulas[i].ativa) particulasAtivas.push_back(jogo.particulas[i]);
    }
    jogo.particulas = particulasAtivas;
}

// ===========================================================================
// SEÇÃO: FLOATING DAMAGE NUMBERS
//
// Números de dano flutuantes: criados sempre que um inimigo sofre dano.
// Puramente visuais — não alteram hp, vida ou qualquer regra de combate.
// ===========================================================================

// Cria um novo FloatingDamage no ponto de impacto.
// Entrada: jogo (para empurrar em jogo.numerosFlutuantes), posicaoImpacto
//          (posição 3D onde o dano ocorreu, tipicamente a posição do zumbi
//          atingido), dano (valor a ser exibido).
// Saída:   adiciona um FloatingDamage a jogo.numerosFlutuantes.
// Observação: cada chamada cria sua própria variável local 'fd' — nenhuma
//          variável temporária é reaproveitada entre chamadas.
inline void criarFloatingDamage(EstadoDoJogo& jogo, Vetor3D posicaoImpacto, int dano) {
    FloatingDamage fd;
    fd.posicao = posicaoImpacto;
    fd.posicao.y += 1.2f; // Surge próximo ao "topo" do inimigo, não nos pés

    fd.valorDano             = dano;
    fd.tempoTotal            = 0.9f;
    fd.tempoRestante         = fd.tempoTotal;
    fd.deslocamentoVertical  = 0.0f;
    fd.transparencia         = 1.0f;

    // Dano alto em destaque (amarelo/laranja), dano normal em branco
    if (dano >= 7) {
        fd.corR = 1.0f; fd.corG = 0.65f; fd.corB = 0.10f;
    } else {
        fd.corR = 1.0f; fd.corG = 1.0f; fd.corB = 1.0f;
    }

    jogo.numerosFlutuantes.push_back(fd);
}

// Atualiza a subida, o fade e o tempo de vida de todos os FloatingDamage.
// Entrada: jogo, deltaTime
// Saída:   modifica cada FloatingDamage in-place; remove os expirados.
//          Chamada exclusivamente em timer() — nunca desenha nada.
inline void atualizarFloatingDamage(EstadoDoJogo& jogo, float deltaTime) {
    const float VELOCIDADE_SUBIDA = 1.4f; // unidades de mundo por segundo

    std::vector<FloatingDamage> ativos;
    ativos.reserve(jogo.numerosFlutuantes.size());

    for (size_t i = 0; i < jogo.numerosFlutuantes.size(); ++i) {
        FloatingDamage& fd = jogo.numerosFlutuantes[i];

        fd.tempoRestante -= deltaTime;
        if (fd.tempoRestante <= 0.0f) continue; // descarta (não copia para 'ativos')

        fd.deslocamentoVertical += VELOCIDADE_SUBIDA * deltaTime;

        // Interpolação suave: fração de vida restante (1.0 → 0.0)
        float fracaoVida = fd.tempoRestante / fd.tempoTotal;
        // Easing simples (suaviza o final do fade, em vez de linear puro)
        fd.transparencia = fracaoVida * fracaoVida;

        ativos.push_back(fd);
    }

    jogo.numerosFlutuantes = ativos;
}

// Retorna a cor-base (RGB) associada a cada tipo de zumbi.
// Usada para colorir partículas de morte de forma consistente com o
// inimigo que as originou. Os valores espelham as cores usadas em
// desenharInimigos() (Main.cpp), mantendo identidade visual.
// Entrada: tipo do zumbi
// Saída:   corR, corG, corB preenchidos por referência
inline void obterCorBaseZumbi(TipoZumbi tipo, float& corR, float& corG, float& corB) {
    switch (tipo) {
        case NORMAL:    corR = 1.0f;  corG = 0.10f; corB = 0.10f; break;
        case RAPIDO:    corR = 1.0f;  corG = 0.45f; corB = 0.0f;  break;
        case TANK:      corR = 0.50f; corG = 0.0f;  corB = 0.85f; break;
        case ATIRADOR:  corR = 0.90f; corG = 0.0f;  corB = 0.75f; break;
        case EXPLOSIVO: corR = 1.0f;  corG = 0.80f; corB = 0.0f;  break;
        default:        corR = 1.0f;  corG = 0.10f; corB = 0.10f; break;
    }
}

// (processarColisoesTiros legado REMOVIDO — colisao agora no motor
//  data-driven via SkillManager::atualizarTodos.)


// Verifica colisão de cada zumbi vivo contra o jogador.
inline void processarColisaoZumbiJogador(EstadoDoJogo& jogo, float deltaTime) {
    Jogador& jog = jogo.protagonista;
    if (!jog.vivo) return;

    if (jog.temporizadorIframe > 0.0f) {
        jog.temporizadorIframe -= deltaTime;
        if (jog.temporizadorIframe < 0.0f) jog.temporizadorIframe = 0.0f;
    }

    for (size_t i = 0; i < jogo.horda.size(); ++i) {
        Zumbi& z = jogo.horda[i];
        if (!z.vivo) continue;

        if (verificarColisao(jog.posicao, jog.raioColisao, z.posicao, z.raioColisao)) {
            if (jog.temporizadorIframe <= 0.0f) {
                jog.hp -= 1;
                jog.temporizadorIframe = jog.duracaoIframe;
                if (jog.hp <= 0) jog.vivo = false;
            }
        }
    }
}

// ===========================================================================
// SEÇÃO: RECOMPENSAS E LEVEL UP
// ===========================================================================

// A inclusão do termo quadrático (nivel * nivel) força uma desaceleração,
// exigindo volumes massivamente maiores de gemas nos níveis avançados.
// Retorna para o modelo original (Rápido e linear)
inline int calcularXpParaNivel(int nivel) {
    return 10 + (nivel * 15);
}

// Apenas coleta a XP
inline void processarColetaDeGemas(EstadoDoJogo& jogo) {
    const float RAIO_COLETA_QUAD = 3.0f * 3.0f;
    Jogador& jog = jogo.protagonista;

    for (size_t i = 0; i < jogo.gemas.size(); ++i) {
        GemaXP& gema = jogo.gemas[i];
        if (gema.coletada) continue;

        float dist = calcularDistanciaQuadrada(jog.posicao, gema.posicao);
        if (dist <= RAIO_COLETA_QUAD) {
            gema.coletada = true;
            jog.xpAtual  += gema.valorXP;
        }
    }
}

// Nova função segura para o Level Up (Garante que o jogo não congele)
// Nova função segura para o Level Up (Garante que o jogo não congele)
inline void processarLevelUp(EstadoDoJogo& jogo) {
    if (jogo.pausadoParaUpgrade) return;

    if (jogo.protagonista.xpAtual >= jogo.protagonista.xpParaProximoNivel) {
        jogo.protagonista.xpAtual -= jogo.protagonista.xpParaProximoNivel;
        jogo.protagonista.nivel   += 1;
        jogo.protagonista.xpParaProximoNivel =
            calcularXpParaNivel(jogo.protagonista.nivel);

        // Sorteia as 3 recompensas do novo sistema
        sortearUpgrades(jogo);

        if (jogo.menuAtual.quantidade > 0) {
            jogo.pausadoParaUpgrade  = true;
            jogo.nivelAntesDaEscolha = jogo.protagonista.nivel;
        }
    }
}

inline void limparEntidadesInativas(EstadoDoJogo& jogo) {
    std::vector<Zumbi> hordaAtiva;
    for (size_t i = 0; i < jogo.horda.size(); ++i) {
        if (jogo.horda[i].vivo) hordaAtiva.push_back(jogo.horda[i]);
    }
    jogo.horda = hordaAtiva;

    // (compactação de tirosNaTela REMOVIDA — o pool de RuntimeSkill se compacta
    //  sozinho via compactarPool() ao fim de processarColisoesSkills_Grade.)

    std::vector<GemaXP> gemasVisiveis;
    for (size_t i = 0; i < jogo.gemas.size(); ++i) {
        if (!jogo.gemas[i].coletada) { // Apenas mantém as NÃO coletadas
            gemasVisiveis.push_back(jogo.gemas[i]);
        }
    }
    jogo.gemas = gemasVisiveis;
}

#endif // GAME_LOGIC_H