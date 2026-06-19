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

    float anguloGraus    = (float)(rand() % 360);
    float anguloRadianos = anguloGraus * (3.14159f / 180.0f);

    Vetor3D novaPosicao;
    novaPosicao.x = posJogador.x + (raioSpawn * cos(anguloRadianos));
    novaPosicao.y = 0.0f;
    novaPosicao.z = posJogador.z + (raioSpawn * sin(anguloRadianos));

    return novaPosicao;
}

// Função "Fábrica": Cria e configura um zumbi dependendo do tipo escolhido.
// raioColisao de cada tipo corresponde exatamente à meia-largura (lx) do cubo visual.
//   NORMAL    → lx = 0.75  → raioColisao = 0.75
//   RAPIDO    → lx = 0.55  → raioColisao = 0.55
//   TANK      → lx = 1.10  → raioColisao = 1.10
//   ATIRADOR  → lx = 0.75  → raioColisao = 0.75
//   EXPLOSIVO → lx = 0.85  → raioColisao = 0.85
inline Zumbi invocarZumbi(TipoZumbi tipoDesejado, Vetor3D posicaoInicial) {
    Zumbi z;
    z.posicao    = posicaoInicial;
    z.tipo       = tipoDesejado;
    z.estadoAtual = WANDER;
    z.vivo       = true;

    switch (tipoDesejado) {
        case NORMAL:
            z.velocidade = 2.0f; z.raioColisao = 0.75f; z.vida = 1; break;
        case RAPIDO:
            z.velocidade = 4.5f; z.raioColisao = 0.55f; z.vida = 1; break;
        case TANK:
            z.velocidade = 1.2f; z.raioColisao = 1.10f; z.vida = 10; break;
        case ATIRADOR:
            z.velocidade = 1.8f; z.raioColisao = 0.75f; z.vida = 2; break;
        case EXPLOSIVO:
            z.velocidade = 2.5f; z.raioColisao = 0.85f; z.vida = 1; break;
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
    }
    else if (tempoSegundos >= 240.0f) {
        if (chance < 60) return NORMAL;
        if (chance < 80) return RAPIDO;
        if (chance < 90) return TANK;
        return ATIRADOR;
    }
    else if (tempoSegundos >= 150.0f) {
        if (chance < 70) return NORMAL;
        if (chance < 90) return RAPIDO;
        return TANK;
    }
    else if (tempoSegundos >= 60.0f) {
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

    if (jogo.cooldownAtual < 0.2f)
        jogo.cooldownAtual = 0.2f;

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

// ---------------------------------------------------------------------------
// SISTEMA DE TENSÃO — NOVO COMPORTAMENTO
//
//  Estado normal (emSobrecarga == false):
//    • Tensão SOBE somente enquanto o jogador está atirando (atirandoAgora).
//      Taxa: TAXA_TENSAO_TIRO unidades/segundo enquanto houver tiros ativos.
//    • Tensão DESCE lentamente quando o jogador NÃO está atirando.
//      Taxa: TAXA_TENSAO_REPOUSO unidades/segundo (negativa = queda).
//    • Ao chegar em 100%: entra em SOBRECARGA.
//
//  Estado de sobrecarga (emSobrecarga == true):
//    • Tensão DRENA automaticamente à taxa TAXA_DRENAGEM_SOBRECARGA/segundo.
//    • Tiros NÃO aumentam nem interrompem a drenagem.
//    • Parry está bloqueado.
//    • Dano dos tiros fica reduzido (FATOR_DANO_SOBRECARGA aplicado em processarColisoesTiros).
//    • Quando tensão chega a 0%: emSobrecarga = false, comportamento normal retorna.
// ---------------------------------------------------------------------------
const float TAXA_TENSAO_TIRO          = 18.0f;  // +tensão/segundo enquanto atira
const float TAXA_TENSAO_REPOUSO       =  5.0f;  // -tensão/segundo enquanto NÃO atira
const float TAXA_DRENAGEM_SOBRECARGA  = 12.0f;  // -tensão/segundo durante sobrecarga
const float FATOR_DANO_SOBRECARGA     = 0.25f;  // dano real = floor(dano * fator) mínimo 0

// Constantes do sistema de Parry (mantidas idênticas)
const float JANELA_PARRY_SEGUNDOS   = 0.25f;
const float COOLDOWN_PARRY_SEGUNDOS = 1.5f;
const float RAIO_EXPULSAO_PARRY     = 15.0f;

// Instancia um novo projétil disparado pela Entidade em direção ao alvo.
// Durante sobrecarga o disparo é BLOQUEADO — a função retorna sem criar projétil.
inline void dispararProjetil(EstadoDoJogo& jogo, Vetor3D posicaoAlvo)
{
    // Bloqueia disparo completamente durante sobrecarga
    if (jogo.stand.emSobrecarga)
        return;

    Projetil novoTiro;
    novoTiro.posicao     = jogo.stand.posicao;
    novoTiro.direcao     = obterDirecaoNormalizada(jogo.stand.posicao, posicaoAlvo);
    novoTiro.velocidade  = 20.0f;
    novoTiro.raioColisao = 0.20f;
    novoTiro.ativo       = true;
    novoTiro.dano        = 1;

    jogo.tirosNaTela.push_back(novoTiro);

    // Incremento flat por clique (fora da sobrecarga, garantido pelo return acima)
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

// ============================================================
// SEÇÃO 1: MATRIZ DE COLISÃO E DANO
// ============================================================

// Cruza tiros com a horda.
// Durante sobrecarga o dano é reduzido pelo FATOR_DANO_SOBRECARGA.
// Tiros durante sobrecarga NÃO interrompem a drenagem da tensão.
inline void processarColisoesTiros(EstadoDoJogo& jogo) {
    for (size_t i = 0; i < jogo.tirosNaTela.size(); ++i) {
        Projetil& tiro = jogo.tirosNaTela[i];
        if (!tiro.ativo) continue;

        for (size_t j = 0; j < jogo.horda.size(); ++j) {
            Zumbi& z = jogo.horda[j];
            if (!z.vivo) continue;

            if (verificarColisao(tiro.posicao, tiro.raioColisao, z.posicao, z.raioColisao)) {
                // Dano efetivo: reduzido em sobrecarga (mínimo 0, ou seja pode não causar dano)
                // Como dano é int, usamos: dano_efetivo = max(0, floor(dano * fator))
                // FATOR_DANO_SOBRECARGA = 0.25 → 1 * 0.25 = 0.25 → floor = 0 → dano zero
                // Isso significa que durante sobrecarga os tiros não causam nenhum dano.
                int danoEfetivo = tiro.dano;
                if (jogo.stand.emSobrecarga) {
                    danoEfetivo = (int)(tiro.dano * FATOR_DANO_SOBRECARGA);
                    // garante que dano 1 * 0.25 → 0 (ineficaz)
                }

                z.vida -= danoEfetivo;
                tiro.ativo = false;

                if (z.vida <= 0) {
                    z.vivo = false;
                    if ((rand() % 100) < 70) {
                        GemaXP gema;
                        gema.posicao  = z.posicao;
                        gema.coletada = false;
                        switch (z.tipo) {
                            case TANK:     gema.valorXP = 10; break;
                            case ATIRADOR: gema.valorXP = 5;  break;
                            case EXPLOSIVO:gema.valorXP = 7;  break;
                            case RAPIDO:   gema.valorXP = 3;  break;
                            default:       gema.valorXP = 2;  break;
                        }
                        jogo.gemas.push_back(gema);
                    }
                }
                break;
            }
        }
    }
}

// Verifica colisão de cada zumbi vivo contra o jogador.
// Respeita i-frames: só aplica dano se temporizadorIframe == 0.
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

                if (jog.hp <= 0) {
                    jog.vivo = false;
                }
            }
        }
    }
}

// ============================================================
// SEÇÃO 2: SISTEMA DE TENSÃO E PARRY
// ============================================================

// Atualiza a tensão a cada frame segundo as novas regras:
//
//  Estado NORMAL (emSobrecarga == false):
//    • Se atirandoAgora == true  → tensão += TAXA_TENSAO_TIRO  * deltaTime
//    • Se atirandoAgora == false → tensão -= TAXA_TENSAO_REPOUSO * deltaTime
//    • tensão clampeada em [0, 100]
//    • Se tensão >= 100: entra em sobrecarga
//
//  Estado SOBRECARGA (emSobrecarga == true):
//    • tensão -= TAXA_DRENAGEM_SOBRECARGA * deltaTime (ignora atirandoAgora)
//    • Se tensão <= 0: sai da sobrecarga, tensão = 0

// Atualiza a tensão a cada frame
inline void atualizarTensao(EstadoDoJogo& jogo, float deltaTime)
{
    Entidade& stand = jogo.stand;

    if (!stand.emSobrecarga)
    {
        // Sobe APENAS quando o jogador atirou neste frame (flag setada em Main.cpp)
        if (jogo.atirandoAgora)
            stand.tensaoAtual += TAXA_TENSAO_TIRO * deltaTime;
        else
            stand.tensaoAtual -= TAXA_TENSAO_REPOUSO * deltaTime;

        // Clamp [0, 100]
        if (stand.tensaoAtual < 0.0f)
            stand.tensaoAtual = 0.0f;

        if (stand.tensaoAtual >= 100.0f)
        {
            stand.tensaoAtual  = 100.0f;
            stand.emSobrecarga = true;
            stand.parryAtivo   = false;
        }
    }
    else
    {
        // Sobrecarga drena automaticamente usando a constante correta
        stand.tensaoAtual -= TAXA_DRENAGEM_SOBRECARGA * deltaTime;

        if (stand.tensaoAtual <= 0.0f)
        {
            stand.tensaoAtual  = 0.0f;
            stand.emSobrecarga = false;
        }
    }
}

// Aplica knockback nos zumbis dentro do raio do Parry.
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

// Chamado pelo input do jogador (tecla Espaço em Main.cpp).
// Bloqueia durante sobrecarga (além do cooldown e janela já ativa).
inline void tentarAtivarParry(Entidade& stand) {
    if (stand.emSobrecarga) return;
    if (stand.temporizadorCooldown > 0.0f) return;
    if (stand.parryAtivo) return;

    stand.parryAtivo        = true;
    stand.temporizadorParry = JANELA_PARRY_SEGUNDOS;
}

// Tenta consumir a janela ativa de Parry ao detectar dano iminente.
// Deve ser chamada ANTES de processarColisaoZumbiJogador.
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
        stand.temporizadorCooldown = COOLDOWN_PARRY_SEGUNDOS;
        stand.parryBemSucedido     = true;
        stand.temporizadorFeedback = 0.4f;
        return true;
    }

    return false;
}

// Atualiza os temporizadores internos do Stand a cada frame.
inline void atualizarTimersStand(EstadoDoJogo& jogo, float deltaTime) {
    Entidade& stand = jogo.stand;

    if (stand.parryAtivo) {
        stand.temporizadorParry -= deltaTime;
        if (stand.temporizadorParry <= 0.0f) {
            stand.parryAtivo           = false;
            stand.temporizadorParry    = 0.0f;
            stand.temporizadorCooldown = COOLDOWN_PARRY_SEGUNDOS * 0.5f;
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

// ============================================================
// SEÇÃO 3: CICLO DE RECOMPENSA — GEMAS E LEVEL UP
// ============================================================

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
                jog.xpAtual          -= jog.xpParaProximoNivel;
                jog.nivel            += 1;
                jog.xpParaProximoNivel = calcularXpParaNivel(jog.nivel);

                jogo.pausadoParaUpgrade  = true;
                jogo.nivelAntesDaEscolha = jog.nivel;
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