#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include "Entities.h"
#include "MathUtils.h"
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
    return DANO_BASE + nivelDano * GANHO_DANO_POR_NIVEL;
}

// Entrada: nível do atributo PERFURACAO (0–3)
// Saída:   número de inimigos extras que o projétil pode atravessar
inline int calcularPerfuracaoBase(int nivelPerfuracao) {
    return PERFURACAO_BASE + nivelPerfuracao * GANHO_PERFURACAO_POR_NIVEL;
}

// Entrada: nível do atributo VELOCIDADE (0–3)
// Saída:   velocidade de movimento do jogador
inline float calcularVelocidadeJogador(int nivelVelocidade) {
    return VELOCIDADE_BASE + nivelVelocidade * GANHO_VELOCIDADE_POR_NIVEL;
}

// Entrada: nível do atributo TENSAO_UP (0–3)
// Saída:   cooldown efetivo do Parry (limitado a 0.5s mínimo)
inline float calcularCooldownParry(int nivelTensao) {
    float cooldown = COOLDOWN_PARRY_SEGUNDOS - nivelTensao * GANHO_TENSAO_REDUCAO;
    if (cooldown < 0.5f) cooldown = 0.5f;
    return cooldown;
}

// Entrada: nível do atributo TENSAO_UP (0–3)
// Saída:   fator de redução da taxa de acúmulo de tensão por tiro (0.0 = sem redução)
inline float calcularReducaoTensaoPorTiro(int nivelTensao) {
    // Cada nível reduz em 20% a tensão gerada por tiro
    return nivelTensao * 0.20f;
}

// Entrada: nível do atributo VIDA (0–3)
// Saída:   HP máximo do jogador
inline int calcularHPMaximo(int nivelVida) {
    return HP_BASE + nivelVida * GANHO_HP_POR_NIVEL;
}

// ===========================================================================
// SEÇÃO: DETERMINAÇÃO DO TIPO DE DISPARO
// ===========================================================================

// Dado dois atributos (em qualquer ordem), retorna o TipoDisparo correspondente.
// A tabela foi construída garantindo que a + b seja sempre o mesmo
// independente da ordem: normalizamos para a < b antes de consultar.
// Entrada: dois TipoUpgrade distintos que chegaram ao nível 2
// Saída:   TipoDisparo correspondente à combinação
inline TipoDisparo determinarTipoDisparo(TipoUpgrade a, TipoUpgrade b) {
    // Normaliza: garante a < b para lookup simples
    if ((int)a > (int)b) {
        TipoUpgrade tmp = a;
        a = b;
        b = tmp;
    }

    // Tabela de 15 combinações (C(6,2))
    if (a == DANO       && b == CADENCIA)   return DISPARO_DANO_CADENCIA;
    if (a == DANO       && b == PERFURACAO) return DISPARO_DANO_PERFURACAO;
    if (a == DANO       && b == TENSAO_UP)  return DISPARO_DANO_TENSAO;
    if (a == DANO       && b == VELOCIDADE) return DISPARO_DANO_VELOCIDADE;
    if (a == DANO       && b == VIDA)       return DISPARO_DANO_VIDA;
    if (a == CADENCIA   && b == PERFURACAO) return DISPARO_CADENCIA_PERFURACAO;
    if (a == CADENCIA   && b == TENSAO_UP)  return DISPARO_CADENCIA_TENSAO;
    if (a == CADENCIA   && b == VELOCIDADE) return DISPARO_CADENCIA_VELOCIDADE;
    if (a == CADENCIA   && b == VIDA)       return DISPARO_CADENCIA_VIDA;
    if (a == PERFURACAO && b == TENSAO_UP)  return DISPARO_PERFURACAO_TENSAO;
    if (a == PERFURACAO && b == VELOCIDADE) return DISPARO_PERFURACAO_VELOCIDADE;
    if (a == PERFURACAO && b == VIDA)       return DISPARO_PERFURACAO_VIDA;
    if (a == TENSAO_UP  && b == VELOCIDADE) return DISPARO_TENSAO_VELOCIDADE;
    if (a == TENSAO_UP  && b == VIDA)       return DISPARO_TENSAO_VIDA;
    if (a == VELOCIDADE && b == VIDA)       return DISPARO_VELOCIDADE_VIDA;

    return DISPARO_NORMAL; // Fallback de segurança
}

// ===========================================================================
// SEÇÃO: INSTANCIAÇÃO DE PROJÉTEIS
//
// instanciarProjetil() configura cada campo do projétil de acordo com o
// TipoDisparo ativo na partida.  Cada combinação altera ao menos dois
// parâmetros que afetam a jogabilidade de forma distinta.
// ===========================================================================

// Entrada: jogo (para ler upgrades e tipo de disparo), posicaoAlvo (alvo do clique)
// Saída:   Projetil completamente configurado, pronto para push_back
inline Projetil instanciarProjetil(const EstadoDoJogo& jogo, Vetor3D posicaoAlvo) {
    Projetil p;
    p.posicao    = jogo.stand.posicao;
    p.direcao    = obterDirecaoNormalizada(jogo.stand.posicao, posicaoAlvo);
    p.ativo      = true;

    // Valores base derivados dos upgrades individuais
    int   danoBase   = calcularDanoBase(jogo.protagonista.upgrades.niveis[DANO]);
    int   perfBase   = calcularPerfuracaoBase(jogo.protagonista.upgrades.niveis[PERFURACAO]);

    // Defaults antes de aplicar comportamento do tipo de disparo
    p.dano              = danoBase;
    p.velocidade        = 20.0f;
    p.raioColisao       = 0.20f;
    p.perfuracaoRestante = perfBase;

    switch (jogo.stand.tipoDisparoAtual) {

        // --------------------------------------------------------------
        // DANO + CADENCIA: projéteis maiores, mais dano, alta velocidade
        // --------------------------------------------------------------
        case DISPARO_DANO_CADENCIA:
            p.dano        = danoBase + 2;
            p.velocidade  = 28.0f;
            p.raioColisao = 0.38f;
            break;

        // --------------------------------------------------------------
        // DANO + PERFURAÇÃO: muito dano, atravessa vários inimigos
        // --------------------------------------------------------------
        case DISPARO_DANO_PERFURACAO:
            p.dano               = danoBase + 3;
            p.velocidade         = 22.0f;
            p.raioColisao        = 0.25f;
            p.perfuracaoRestante = perfBase + 3;
            break;

        // --------------------------------------------------------------
        // DANO + TENSÃO: tiros que drenam menos tensão (redução 50%)
        //   → jogador pode atirar mais antes de entrar em sobrecarga
        //   → representa o atributo TENSAO_UP beneficiando o disparo
        // --------------------------------------------------------------
        case DISPARO_DANO_TENSAO:
            p.dano       = danoBase + 2;
            p.velocidade = 20.0f;
            // raioColisao menor = tiro mais preciso, menos tensão por acerto
            p.raioColisao = 0.15f;
            break;

        // --------------------------------------------------------------
        // DANO + VELOCIDADE: projéteis extremamente rápidos, muito dano
        // --------------------------------------------------------------
        case DISPARO_DANO_VELOCIDADE:
            p.dano       = danoBase + 3;
            p.velocidade = 36.0f;
            p.raioColisao = 0.18f;
            break;

        // --------------------------------------------------------------
        // DANO + VIDA: tiros pesados e lentos, mas com alto dano
        //   Compensa baixa velocidade com hitbox maior
        // --------------------------------------------------------------
        case DISPARO_DANO_VIDA:
            p.dano        = danoBase + 4;
            p.velocidade  = 14.0f;
            p.raioColisao = 0.42f;
            break;

        // --------------------------------------------------------------
        // CADÊNCIA + PERFURAÇÃO: rajadas que atravessam vários inimigos
        // --------------------------------------------------------------
        case DISPARO_CADENCIA_PERFURACAO:
            p.dano               = danoBase;
            p.velocidade         = 26.0f;
            p.raioColisao        = 0.18f;
            p.perfuracaoRestante = perfBase + 4;
            break;

        // --------------------------------------------------------------
        // CADÊNCIA + TENSÃO: tiros rápidos que acumulam pouca tensão
        //   → cadência alta + duração do tiroteio prolongada
        // --------------------------------------------------------------
        case DISPARO_CADENCIA_TENSAO:
            p.dano        = danoBase;
            p.velocidade  = 24.0f;
            p.raioColisao = 0.16f;
            break;

        // --------------------------------------------------------------
        // CADÊNCIA + VELOCIDADE: projéteis ultrarrápidos em sequência
        // --------------------------------------------------------------
        case DISPARO_CADENCIA_VELOCIDADE:
            p.dano        = danoBase;
            p.velocidade  = 40.0f;
            p.raioColisao = 0.15f;
            break;

        // --------------------------------------------------------------
        // CADÊNCIA + VIDA: tiros médios que curam lentamente ao acertar
        //   Mecânica de cura implementada em processarColisoesTiros()
        // --------------------------------------------------------------
        case DISPARO_CADENCIA_VIDA:
            p.dano        = danoBase;
            p.velocidade  = 22.0f;
            p.raioColisao = 0.22f;
            break;

        // --------------------------------------------------------------
        // PERFURAÇÃO + TENSÃO: atravessa muitos inimigos, menor tensão
        // --------------------------------------------------------------
        case DISPARO_PERFURACAO_TENSAO:
            p.dano               = danoBase;
            p.velocidade         = 20.0f;
            p.raioColisao        = 0.20f;
            p.perfuracaoRestante = perfBase + 5;
            break;

        // --------------------------------------------------------------
        // PERFURAÇÃO + VELOCIDADE: tiro rápido que atravessa hordas
        // --------------------------------------------------------------
        case DISPARO_PERFURACAO_VELOCIDADE:
            p.dano               = danoBase;
            p.velocidade         = 32.0f;
            p.raioColisao        = 0.20f;
            p.perfuracaoRestante = perfBase + 3;
            break;

        // --------------------------------------------------------------
        // PERFURAÇÃO + VIDA: projétil lento e pesado, atravessa tudo
        // --------------------------------------------------------------
        case DISPARO_PERFURACAO_VIDA:
            p.dano               = danoBase + 2;
            p.velocidade         = 12.0f;
            p.raioColisao        = 0.35f;
            p.perfuracaoRestante = perfBase + 6;
            break;

        // --------------------------------------------------------------
        // TENSÃO + VELOCIDADE: rápido, acumula pouca tensão
        // --------------------------------------------------------------
        case DISPARO_TENSAO_VELOCIDADE:
            p.dano        = danoBase;
            p.velocidade  = 34.0f;
            p.raioColisao = 0.16f;
            break;

        // --------------------------------------------------------------
        // TENSÃO + VIDA: tiro que restaura uma fração de tensão ao acertar
        //   Mecânica especial em processarColisoesTiros()
        // --------------------------------------------------------------
        case DISPARO_TENSAO_VIDA:
            p.dano        = danoBase + 1;
            p.velocidade  = 18.0f;
            p.raioColisao = 0.25f;
            break;

        // --------------------------------------------------------------
        // VELOCIDADE + VIDA: tiro rápido com hitbox grande
        // --------------------------------------------------------------
        case DISPARO_VELOCIDADE_VIDA:
            p.dano        = danoBase + 1;
            p.velocidade  = 30.0f;
            p.raioColisao = 0.30f;
            break;

        // DISPARO_NORMAL ou qualquer fallback: usa valores padrão já setados
        default:
            break;
    }

    return p;
}

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
    // Monta pool de candidatos válidos
    TipoUpgrade candidatos[TOTAL_UPGRADES];
    int numCandidatos = 0;

    for (int i = 0; i < TOTAL_UPGRADES; ++i) {
        if (jogo.protagonista.upgrades.niveis[i] < NIVEL_MAXIMO_UPGRADE) {
            candidatos[numCandidatos] = (TipoUpgrade)i;
            numCandidatos++;
        }
    }

    // Fisher-Yates parcial: embaralha apenas os primeiros 3 do pool
    int limite = (numCandidatos < 3) ? numCandidatos : 3;
    for (int i = 0; i < limite; ++i) {
        int j = i + rand() % (numCandidatos - i);
        TipoUpgrade tmp  = candidatos[i];
        candidatos[i]    = candidatos[j];
        candidatos[j]    = tmp;
    }

    jogo.quantidadeOpcoes = limite;
    for (int i = 0; i < limite; ++i)
        jogo.opcoesUpgrade[i] = candidatos[i];
}

// Aplica o upgrade escolhido ao estado do jogo e atualiza todos os atributos.
// Cuida também da contagem para evolução do disparo.
// Entrada: jogo (estado mutável), tipo (atributo a subir de nível)
// Saída:   modifica jogo in-place; não retorna nada
inline void aplicarUpgrade(EstadoDoJogo& jogo, TipoUpgrade tipo) {
    SistemaUpgrades& upg = jogo.protagonista.upgrades;

    // Guarda o nível atual antes de incrementar
    int nivelAnterior = upg.niveis[tipo];

    // Garante que não ultrapasse o máximo
    if (nivelAnterior >= NIVEL_MAXIMO_UPGRADE)
        return;

    upg.niveis[tipo] = nivelAnterior + 1;
    int novoNivel    = upg.niveis[tipo];

    // --- Efeitos imediatos por tipo ---
    switch (tipo) {
        case DANO:
            // O dano é calculado dinamicamente em instanciarProjetil(); nenhuma
            // variável extra precisa ser atualizada aqui.
            break;

        case CADENCIA:
            // O sistema de disparo atual é baseado em clique manual.
            // O upgrade de cadência beneficia os TipoDisparo que aumentam
            // velocidade/tamanho do projétil — sem cooldown automático a reduzir.
            break;

        case PERFURACAO:
            // Perfuração é aplicada dinamicamente em instanciarProjetil().
            break;

        case TENSAO_UP:
            // Atualiza cooldown do parry imediatamente
            jogo.stand.cooldownParry = calcularCooldownParry(novoNivel);
            // Se o parry estiver em cooldown e o novo valor for menor, ajusta
            if (jogo.stand.temporizadorCooldown > jogo.stand.cooldownParry)
                jogo.stand.temporizadorCooldown = jogo.stand.cooldownParry;
            break;

        case VELOCIDADE:
            jogo.protagonista.velocidade = calcularVelocidadeJogador(novoNivel);
            break;

        case VIDA: {
            int hpMaxNovo = calcularHPMaximo(novoNivel);
            jogo.protagonista.hpMaximo = hpMaxNovo;
            // Recupera 1 HP ao subir, sem ultrapassar o máximo
            jogo.protagonista.hp += 1;
            if (jogo.protagonista.hp > jogo.protagonista.hpMaximo)
                jogo.protagonista.hp = jogo.protagonista.hpMaximo;
            break;
        }

        default:
            break;
    }

    // --- Controle da evolução de disparo ---
    // Só registramos quando o atributo atingiu exatamente o nível 2
    if (nivelAnterior == 1 && novoNivel == 2 && !jogo.disparoEvoluido) {
        jogo.contagemAtributosNivel2++;

        if (jogo.contagemAtributosNivel2 == 1) {
            jogo.primeiroAtributoNivel2 = tipo;
        } else if (jogo.contagemAtributosNivel2 == 2) {
            jogo.segundoAtributoNivel2  = tipo;

            // Evolução! Determina o tipo de disparo e trava para sempre.
            jogo.stand.tipoDisparoAtual = determinarTipoDisparo(
                jogo.primeiroAtributoNivel2,
                jogo.segundoAtributoNivel2
            );
            jogo.disparoEvoluido = true;
        }
        // Se contagemAtributosNivel2 > 2, disparoEvoluido já é true → ignorado
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
// SEÇÃO: DISPARO
// ===========================================================================

// Instancia um projétil configurado para o TipoDisparo ativo.
// Bloqueia completamente durante sobrecarga.
// Entrada: jogo (estado global), posicaoAlvo (coordenada mundo do clique)
// Saída:   adiciona um Projetil a jogo.tirosNaTela (se não em sobrecarga)
inline void dispararProjetil(EstadoDoJogo& jogo, Vetor3D posicaoAlvo) {
    if (jogo.stand.emSobrecarga) return;

    Projetil novoTiro = instanciarProjetil(jogo, posicaoAlvo);
    jogo.tirosNaTela.push_back(novoTiro);

    // Tensão base por clique (independente do tipo de disparo)
    jogo.stand.tensaoAtual += 5.0f;
    if (jogo.stand.tensaoAtual > 100.0f)
        jogo.stand.tensaoAtual = 100.0f;
}

// Atualiza a trajetória dos projéteis e descarta os que saíram da arena
inline void atualizarProjeteis(EstadoDoJogo& jogo, float deltaTime) {
    const float LIMITE_ARENA = 150.0f;

    for (size_t i = 0; i < jogo.tirosNaTela.size(); ++i) {
        Projetil& tiro = jogo.tirosNaTela[i];
        if (!tiro.ativo) continue;

        tiro.posicao.x += tiro.direcao.x * tiro.velocidade * deltaTime;
        tiro.posicao.z += tiro.direcao.z * tiro.velocidade * deltaTime;

        if (tiro.posicao.x >  LIMITE_ARENA || tiro.posicao.x < -LIMITE_ARENA ||
            tiro.posicao.z >  LIMITE_ARENA || tiro.posicao.z < -LIMITE_ARENA) {
            tiro.ativo = false;
        }
    }
}

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

// Cruza tiros com a horda, respeitando perfuração e evitando acertar o mesmo
// zumbi duas vezes no mesmo frame com o mesmo projétil.
// Comportamentos especiais por TipoDisparo:
//   DISPARO_CADENCIA_VIDA  → 30% de chance de recuperar 1 HP ao acertar
//   DISPARO_TENSAO_VIDA    → restaura 3 unidades de tensão ao acertar
inline void processarColisoesTiros(EstadoDoJogo& jogo) {
    const int MAX_ZUMBIS_POR_TIRO = 32; // Limite de segurança para o array local

    for (size_t i = 0; i < jogo.tirosNaTela.size(); ++i) {
        Projetil& tiro = jogo.tirosNaTela[i];
        if (!tiro.ativo) continue;

        // Array local de índices de zumbis já acertados por ESTE tiro NESTE frame
        size_t jaAcertados[MAX_ZUMBIS_POR_TIRO];
        int    qtdAcertados = 0;

        for (size_t j = 0; j < jogo.horda.size(); ++j) {
            Zumbi& z = jogo.horda[j];
            if (!z.vivo) continue;

            // Não acertar o mesmo zumbi duas vezes
            if (jaAcertouEsteZumbi(jaAcertados, qtdAcertados, j)) continue;

            if (verificarColisao(tiro.posicao, tiro.raioColisao, z.posicao, z.raioColisao)) {
                // Registra acerto para evitar colisão dupla
                if (qtdAcertados < MAX_ZUMBIS_POR_TIRO)
                    jaAcertados[qtdAcertados++] = j;

                // Dano efetivo
                int danoEfetivo = tiro.dano;
                if (jogo.stand.emSobrecarga) {
                    danoEfetivo = (int)(tiro.dano * FATOR_DANO_SOBRECARGA);
                }

                z.vida -= danoEfetivo;

                // Efeitos especiais por tipo de disparo
                if (jogo.stand.tipoDisparoAtual == DISPARO_CADENCIA_VIDA) {
                    // 30% de chance de recuperar 1 HP
                    if ((rand() % 100) < 30) {
                        if (jogo.protagonista.hp < jogo.protagonista.hpMaximo)
                            jogo.protagonista.hp++;
                    }
                } else if (jogo.stand.tipoDisparoAtual == DISPARO_TENSAO_VIDA) {
                    // Restaura 3 unidades de tensão ao acertar
                    if (!jogo.stand.emSobrecarga) {
                        jogo.stand.tensaoAtual -= 3.0f;
                        if (jogo.stand.tensaoAtual < 0.0f) jogo.stand.tensaoAtual = 0.0f;
                    }
                }

                // Morte do zumbi
                if (z.vida <= 0) {
                    processarMorteZumbi(jogo, z);
                }

                // Controle de perfuração
                if (tiro.perfuracaoRestante <= 0) {
                    tiro.ativo = false;
                    break; // Sem perfuração restante → para de verificar zumbis
                } else {
                    tiro.perfuracaoRestante--;
                    // Continua ativo, verifica próximo zumbi
                }
            }
        }
    }
}

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

inline int calcularXpParaNivel(int nivel) {
    return 10 + (nivel * 15);
}

inline void processarColetaDeGemas(EstadoDoJogo& jogo) {
    const float RAIO_COLETA_QUAD = 3.0f * 3.0f;
    Jogador& jog = jogo.protagonista;

    for (size_t i = 0; i < jogo.gemas.size(); ++i) {
        GemaXP& gema = jogo.gemas[i];
        if (gema.coletada) continue;

        float dist = calcularDistanciaQuadrada(jog.posicao, gema.posicao);
        if (dist <= RAIO_COLETA_QUAD) {
            gema.coletada  = true;
            jog.xpAtual   += gema.valorXP;

            while (jog.xpAtual >= jog.xpParaProximoNivel) {
                jog.xpAtual           -= jog.xpParaProximoNivel;
                jog.nivel             += 1;
                jog.xpParaProximoNivel = calcularXpParaNivel(jog.nivel);

                jogo.pausadoParaUpgrade  = true;
                jogo.nivelAntesDaEscolha = jog.nivel;

                // Sorteia as opções de upgrade no momento da pausa
                sortearUpgrades(jogo);
            }
        }
    }
}

inline void limparEntidadesInativas(EstadoDoJogo& jogo) {
    std::vector<Zumbi> hordaAtiva;
    for (size_t i = 0; i < jogo.horda.size(); ++i) {
        if (jogo.horda[i].vivo) hordaAtiva.push_back(jogo.horda[i]);
    }
    jogo.horda = hordaAtiva;

    std::vector<Projetil> tirosAtivos;
    for (size_t i = 0; i < jogo.tirosNaTela.size(); ++i) {
        if (jogo.tirosNaTela[i].ativo) tirosAtivos.push_back(jogo.tirosNaTela[i]);
    }
    jogo.tirosNaTela = tirosAtivos;

    std::vector<GemaXP> gemasVisiveis;
    for (size_t i = 0; i < jogo.gemas.size(); ++i) {
        if (!jogo.gemas[i].coletada) gemasVisiveis.push_back(jogo.gemas[i]);
    }
    jogo.gemas = gemasVisiveis;
}

#endif