#ifndef GAMEUPDATE_H
#define GAMEUPDATE_H

// =============================================================================
//  GameUpdate.h — Lógica de atualização do jogo (extraído de Main.cpp)
// =============================================================================
// =============================================================================
//  SPAWN DE ZUMBIS
// =============================================================================

static Zumbi criarZumbi(TipoZumbi tipo) {
    Zumbi z;
    z.tipo        = tipo;
    z.estadoAtual = WANDER;
    z.vivo        = true;
    z.ehBoss      = false; // zumbis comuns nunca são o Boss

    // Spawn em anel ao redor do jogador — fora da tela, dentro da arena
    {
        const float PI = 3.14159265f;
        float ang = (rand() % 360) * (PI / 180.0f);
        float r   = SPAWN_RAIO_MIN + (rand() % 1000) * (SPAWN_RAIO_MAX - SPAWN_RAIO_MIN) / 1000.0f;
        Jogador& jp = g_jogo.protagonista;
        float sx = jp.posicao.x + std::cos(ang) * r;
        float sz = jp.posicao.z + std::sin(ang) * r;
        float lim = ARENA_HALF - 1.0f;
        if (sx >  lim) sx =  lim;
        if (sx < -lim) sx = -lim;
        if (sz >  lim) sz =  lim;
        if (sz < -lim) sz = -lim;
        z.posicao.x = sx;
        z.posicao.y = 0.0f;
        z.posicao.z = sz;
    }

    switch (tipo) {
        case RAPIDO:
            z.velocidade = 10.0f; z.raioColisao = 0.6f; z.vida = 1;  z.dano = 1; break;
        case TANK:
            z.velocidade =  5.0f; z.raioColisao = 1.3f; z.vida = 10; z.dano = 3; break;
        case ATIRADOR:
            z.velocidade =  6.0f; z.raioColisao = 0.7f; z.vida = 2;  z.dano = 3; break;
        case EXPLOSIVO:
            z.velocidade = 10.0f; z.raioColisao = 0.8f; z.vida = 2;  z.dano = 3; break;
        default: // NORMAL
            z.velocidade = 8.0f; z.raioColisao = 0.7f; z.vida = 2;  z.dano = 1; break;
    }
    z.tiroTimer = ATIRADOR_COOLDOWN * ((rand() % 100) / 100.0f); // offset inicial aleatório
    return z;
}

// =============================================================================
//  ATUALIZAÇÃO: JOGADOR
// =============================================================================

static void atualizarJogador(float dt) {
    Jogador& p = g_jogo.protagonista;
    if (!p.vivo) return;

    if (p.temporizadorIframe > 0.0f) {
        p.temporizadorIframe -= dt;
    }

    // Movimentação WASD
    float dx = 0.0f, dz = 0.0f;
    if (g_teclaW) dz -= 1.0f;
    if (g_teclaS) dz += 1.0f;
    if (g_teclaA) dx -= 1.0f;
    if (g_teclaD) dx += 1.0f;

    float len = std::sqrt(dx * dx + dz * dz);
    if (len > 0.0001f) {
        dx /= len; dz /= len;
        float velFinal = p.velocidade * fatorVelocidadeDoNivel(p.upgrades.niveis[VELOCIDADE_UP]);
        p.posicao.x += dx * velFinal * dt;
        p.posicao.z += dz * velFinal * dt;

        float lim = ARENA_HALF - p.raioColisao;
        if (p.posicao.x >  lim) p.posicao.x =  lim;
        if (p.posicao.x < -lim) p.posicao.x = -lim;
        if (p.posicao.z >  lim) p.posicao.z =  lim;
        if (p.posicao.z < -lim) p.posicao.z = -lim;
    }

    // Mira e orientação seguem o mouse; stand fica nas costas (oposto ao cursor)
    Vetor3D dirMira = obterDirecaoNormalizada(p.posicao, g_posicaoCursor);
    const float distanciaStand = 1.5f;
    g_jogo.stand.posicao.x = p.posicao.x - dirMira.x * distanciaStand;
    g_jogo.stand.posicao.z = p.posicao.z - dirMira.z * distanciaStand;
    g_jogo.stand.anguloMira = std::atan2(dirMira.z, dirMira.x);

    // Mantém o Y do stand na mesma altura do fantasma (bob incluso)
    g_jogo.stand.posicao.y = 3.5f + std::sin(g_jogo.tempoSobrevivido * 2.0f) * 0.12f;

    // Posição da pistola no mundo — replica a cadeia de transforms de desenharJogador()
    // mais o bone socket, para que projéteis manuais saiam do cano da pistola.
    if (g_sofiaCarregada && g_sofia.temMao()) {
        float angRad = -g_jogo.stand.anguloMira + glm::radians(SOFIA_ROT_OFFSET);
        glm::mat4 m = glm::translate(glm::mat4(1.0f),
                                     glm::vec3(p.posicao.x, SOFIA_Y_OFFSET, p.posicao.z));
        m = glm::rotate(m, angRad, glm::vec3(0.0f, 1.0f, 0.0f));
        m = glm::rotate(m, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        m = glm::scale(m, glm::vec3(SOFIA_ESCALA, SOFIA_ESCALA, SOFIA_ESCALA));
        m = m * g_sofia.obterMatrizSocketMao();
        m = glm::translate(m, glm::vec3(g_pistolaOffX, g_pistolaOffY, g_pistolaOffZ));
        g_jogo.posicaoPistola.x = m[3].x;
        g_jogo.posicaoPistola.y = m[3].y;
        g_jogo.posicaoPistola.z = m[3].z;
    } else {
        g_jogo.posicaoPistola.x = p.posicao.x;
        g_jogo.posicaoPistola.y = 1.5f;
        g_jogo.posicaoPistola.z = p.posicao.z;
    }

// --- Controle de Sobrecarga e Resfriamento de Tensão ---
    float maxTensao      = maxTensaoDoNivel(g_jogo.protagonista.upgrades.niveis[TENSAO_UP]);
    float taxaDecaimento = taxaDecaimentoTensaoDoNivel(g_jogo.protagonista.upgrades.niveis[TENSAO_UP]);

    if (g_jogo.stand.emSobrecarga) {
        g_jogo.stand.tensaoAtual -= (taxaDecaimento * (4.0f / 3.0f)) * dt;
        if (g_jogo.stand.tensaoAtual <= 0.0f) {
            g_jogo.stand.tensaoAtual  = 0.0f;
            g_jogo.stand.emSobrecarga = false;
        }
    } else {
        if (g_jogo.stand.tensaoAtual >= maxTensao) {
            g_jogo.stand.emSobrecarga = true;
            g_jogo.stand.tensaoAtual  = maxTensao;
        }
        else if (!g_cliqueMouse) {
            // Esvazia a barra continuamente quando o botão não está pressionado
            g_jogo.stand.tensaoAtual -= taxaDecaimento * dt;
            if (g_jogo.stand.tensaoAtual < 0.0f) {
                g_jogo.stand.tensaoAtual = 0.0f;
            }
        }
    }

    // --- Disparo Contínuo por Clique ---
    static float s_cooldownDisparo = 0.0f;
    if (s_cooldownDisparo > 0.0f) {
        s_cooldownDisparo -= dt;
    }

    if (g_cliqueMouse && !g_jogo.stand.emSobrecarga && !g_jogo.jogoPausado) {
        g_jogo.atirandoAgora = true;
        if (s_cooldownDisparo <= 0.0f) {
            g_skills.executar(g_jogo, g_posicaoCursor);
            
            // Reproduz o efeito sonoro de disparo usando o arquivo renomeado
            tocarEfeito("Sons/disparo.mp3");

            // Cooldown do clique manual afetado pela velocidade da cadência.
            // ARQ_ESPINGARDA_TATICA seta cooldownManual > 0 para impor base ×4 mais lento.
            float baseCooldown = 0.50f;
            {
                const SlotSkill& slot0 = g_skills.slot(0);
                if (slot0.ativo && slot0.build.cooldownManual > 0.0f)
                    baseCooldown = slot0.build.cooldownManual;
            }
            int nivelCadencia  = g_jogo.protagonista.upgrades.niveis[CADENCIA];
            s_cooldownDisparo  = baseCooldown * fatorCadenciaDoNivel(nivelCadencia);  
        }
    } else {
        // A flag atirandoAgora só deve ser falsa quando o jogador não estiver clicando
        // ou estiver em sobrecarga/pausa.
        g_jogo.atirandoAgora = false;
    }

    // --- Animação da Sofia ---------------------------------------------------
    if (g_sofiaCarregada) {
        bool movendo = (g_teclaW || g_teclaS || g_teclaA || g_teclaD);
        if (movendo) {
            // Direção normalizada do input
            float idx = 0.0f, idz = 0.0f;
            if (g_teclaW) idz -= 1.0f;
            if (g_teclaS) idz += 1.0f;
            if (g_teclaA) idx -= 1.0f;
            if (g_teclaD) idx += 1.0f;
            float ilen = std::sqrt(idx*idx + idz*idz);
            if (ilen > 0.0001f) { idx /= ilen; idz /= ilen; }
            // Componente na direção de mira: positivo=frente, negativo=costas
            float ang  = g_jogo.stand.anguloMira;
            float dotFwd = idx * std::cos(ang) + idz * std::sin(ang);
            float fatorVel = fatorVelocidadeDoNivel(g_jogo.protagonista.upgrades.niveis[VELOCIDADE_UP]);
            g_sofia.definirVelocidade(dotFwd >= 0.0f ? fatorVel : -fatorVel);
        } else {
            // CORREÇÃO: Congela exatamente no frame 1 quando o jogador parar
            g_sofia.congelarNoFrame(14); 
        }
        g_sofia.atualizar(dt);
    }
}

// =============================================================================
//  ATUALIZAÇÃO: ZUMBIS (IA simples WANDER→CHASE)
// =============================================================================

static void atualizarZumbis(float dt) {
    Jogador& p = g_jogo.protagonista;
    int zumbisAtivos = 0; // Contador para o gerenciador de áudio

    for (size_t i = 0; i < g_jogo.horda.size(); ++i) {
        Zumbi& z = g_jogo.horda[i];
        if (!z.vivo) continue;

        zumbisAtivos++; // Registra que há zumbis vivos na tela

        float dx   = p.posicao.x - z.posicao.x;
        float dz   = p.posicao.z - z.posicao.z;
        float dist = std::sqrt(dx * dx + dz * dz);
        float nx   = (dist > 0.001f) ? dx / dist : 0.0f;
        float nz   = (dist > 0.001f) ? dz / dist : 0.0f;

        if (z.tipo == ATIRADOR) {
            // IA do Atirador
            if (dist > ATIRADOR_DIST * 1.3f) {
                z.posicao.x += nx * z.velocidade * dt;
                z.posicao.z += nz * z.velocidade * dt;
            } else if (dist < ATIRADOR_DIST * 0.7f) {
                z.posicao.x -= nx * z.velocidade * dt;
                z.posicao.z -= nz * z.velocidade * dt;
            }
            
            z.tiroTimer -= dt;
            if (z.tiroTimer <= 0.0f) {
                z.tiroTimer = ATIRADOR_COOLDOWN;
                ProjetilZumbi pz;
                pz.posicao    = z.posicao;
                pz.direcao.x  = (dist > 0.001f) ? nx : 1.0f;
                pz.direcao.y  = 0.0f;
                pz.direcao.z  = (dist > 0.001f) ? nz : 0.0f;
                pz.velocidade = PROJETIL_ZUMBI_VEL;
                pz.dano       = PROJETIL_ZUMBI_DANO;
                pz.ativo      = true;
                pz.ehDoBoss   = false;
                g_jogo.projeteisZumbi.push_back(pz);
            }
        } else {
            // IA de Perseguição Direta
            z.posicao.x += nx * z.velocidade * dt;
            z.posicao.z += nz * z.velocidade * dt;
        }

        // Limites da arena
        if (z.posicao.x >  ARENA_HALF) z.posicao.x =  ARENA_HALF;
        if (z.posicao.x < -ARENA_HALF) z.posicao.x = -ARENA_HALF;
        if (z.posicao.z >  ARENA_HALF) z.posicao.z =  ARENA_HALF;
        if (z.posicao.z < -ARENA_HALF) z.posicao.z = -ARENA_HALF;
    }

    // --- Gerenciador de Áudio Ambiente dos Zumbis ---
    static float timerSomZumbi = 2.0f;

    // Sons ambiente de zumbi: só disparam se a personagem não estiver falando.
    // Quando ela fala, acumulamos o timer normalmente para não gerar uma
    // "fila" reprimida — ao silenciar, o próximo som já está pronto.
    if (zumbisAtivos > 0) {
        timerSomZumbi -= dt;
        if (timerSomZumbi <= 0.0f) {
            if (!personagemEstaFalando()) {
                // Sorteia um dos três sons de zumbi apenas se a Sofia não estiver falando
                int sorteio = rand() % 3;
                if (sorteio == 0)      tocarEfeito("Sons/brains1.mp3");
                else if (sorteio == 1) tocarEfeito("Sons/brains2.mp3");
                else                   tocarEfeito("Sons/passos_zumbi.mp3");
            }
            // Recarrega o timer independente de ter tocado ou não
            timerSomZumbi = 1.5f + ((rand() % 200) / 100.0f);
        }
    }
}


// =============================================================================
//  ATUALIZAÇÃO: PARTÍCULAS E FLOATING DAMAGE
// =============================================================================

static void atualizarParticulas(float dt) {
    for (size_t i = 0; i < g_jogo.particulas.size(); ++i) {
        Particula& p = g_jogo.particulas[i];
        if (!p.ativa) continue;
        p.posicao.x    += p.velocidade.x * dt;
        p.posicao.y    += p.velocidade.y * dt;
        p.posicao.z    += p.velocidade.z * dt;
        p.velocidade.y -= 9.8f * dt;
        p.tempoVida    -= dt;
        p.transparencia = p.tempoVida / p.tempoVidaMaximo;
        if (p.tempoVida <= 0.0f) p.ativa = false;
    }
    // Compacta in-place — mantém vetor pequeno, iterações futuras baratas
    size_t w = 0;
    for (size_t i = 0; i < g_jogo.particulas.size(); ++i)
        if (g_jogo.particulas[i].ativa)
            g_jogo.particulas[w++] = g_jogo.particulas[i];
    g_jogo.particulas.resize(w);
}

static void atualizarFloatingDamage(float dt) {
    for (size_t i = 0; i < g_jogo.numerosFlutuantes.size(); ++i) {
        FloatingDamage& fd = g_jogo.numerosFlutuantes[i];
        if (fd.tempoRestante <= 0.0f) continue;  // não atualiza expirados
        fd.tempoRestante        -= dt;
        fd.deslocamentoVertical += 2.0f * dt;
        fd.transparencia = fd.tempoRestante / fd.tempoTotal;
    }
    // Compacta expirados
    size_t w = 0;
    for (size_t i = 0; i < g_jogo.numerosFlutuantes.size(); ++i)
        if (g_jogo.numerosFlutuantes[i].tempoRestante > 0.0f)
            g_jogo.numerosFlutuantes[w++] = g_jogo.numerosFlutuantes[i];
    g_jogo.numerosFlutuantes.resize(w);
}

static void atualizarProjeteisZumbi(float dt) {
    Jogador& p = g_jogo.protagonista;
    for (size_t i = 0; i < g_jogo.projeteisZumbi.size(); ++i) {
        ProjetilZumbi& pz = g_jogo.projeteisZumbi[i];
        if (!pz.ativo) continue;

        pz.posicao.x += pz.direcao.x * pz.velocidade * dt;
        pz.posicao.z += pz.direcao.z * pz.velocidade * dt;

        if (std::abs(pz.posicao.x) > ARENA_HALF || std::abs(pz.posicao.z) > ARENA_HALF) {
            pz.ativo = false;
            continue;
        }

        if (p.vivo && p.temporizadorIframe <= 0.0f) {
            float cdx = pz.posicao.x - p.posicao.x;
            float cdz = pz.posicao.z - p.posicao.z;
            float r   = p.raioColisao + 0.2f;
            if ((cdx*cdx + cdz*cdz) <= r * r) {
                pz.ativo = false;
                p.hp -= pz.dano;
                tocarEfeitoComVolume("Sons/Ai.mp3", VOLUME_FALA);
                p.temporizadorIframe = p.duracaoIframe;
                if (p.hp <= 0) {
                    p.hp   = 0;
                    p.vivo = false;
                    g_jogoTerminado = true;
                    pausarMusicaFundo();
                    tocarGameOver();
                }
                }
            }
        }
    // Compacta projéteis inativos — mantém vetor pequeno para iterações futuras
    size_t w = 0;
    for (size_t i = 0; i < g_jogo.projeteisZumbi.size(); ++i)
        if (g_jogo.projeteisZumbi[i].ativo)
            g_jogo.projeteisZumbi[w++] = g_jogo.projeteisZumbi[i];
    g_jogo.projeteisZumbi.resize(w);
    }


// =============================================================================
//  SISTEMA DE BOSS
// =============================================================================

// ---------------------------------------------------------------------------
// invocarBoss — cria o Boss como um Zumbi especial dentro da horda existente.
//   Reutiliza todo o sistema de colisão, dano e IA de perseguição dos zumbis.
//   O Boss é colocado a pelo menos BOSS_DIST_SPAWN unidades do jogador.
// ---------------------------------------------------------------------------
static void invocarBoss() {
    Zumbi boss;
    boss.ehBoss      = true;
    boss.tipo        = NORMAL;       // campo tipo não é usado no Boss (ehBoss prevalece)
    boss.estadoAtual = CHASE;
    boss.vivo        = true;
    boss.vida        = BOSS_VIDA_MAXIMA;
    boss.dano        = BOSS_DANO;
    boss.velocidade  = BOSS_VELOCIDADE;
    boss.raioColisao = BOSS_RAIO_COLISAO;
    boss.tiroTimer   = 0.0f;

    // Calcula posição de spawn: anel ao redor do jogador, dentro da arena
    const float PI = 3.14159265f;
    const float lim = ARENA_HALF - BOSS_RAIO_COLISAO - 2.0f;
    Jogador& jp = g_jogo.protagonista;

    float sx = 0.0f, sz = 0.0f;
    for (int tentativa = 0; tentativa < 16; ++tentativa) {
        float ang = (rand() % 360) * (PI / 180.0f);
        float r   = BOSS_DIST_SPAWN + (float)(rand() % 20);
        sx = jp.posicao.x + std::cos(ang) * r;
        sz = jp.posicao.z + std::sin(ang) * r;
        if (sx >  lim) sx =  lim;
        if (sx < -lim) sx = -lim;
        if (sz >  lim) sz =  lim;
        if (sz < -lim) sz = -lim;
        // Verifica distância mínima do jogador
        float dx = sx - jp.posicao.x, dz = sz - jp.posicao.z;
        if ((dx*dx + dz*dz) >= BOSS_DIST_SPAWN * BOSS_DIST_SPAWN * 0.5f) break;
    }

    boss.posicao.x = sx;
    boss.posicao.y = 0.0f;
    boss.posicao.z = sz;

    g_jogo.horda.push_back(boss);

    // Ativa fase e timers
    g_jogo.fasePartida        = FASE_BOSS;
    g_jogo.tempMensagemBoss   = BOSS_MSG_DURACAO;
    g_jogo.tempoEntradaBoss   = BOSS_ENTRADA_DURACAO;
    g_jogo.bossJaFoiInvocado  = true;
    tocarEfeito("Sons/BossEntrada.mp3");
}

// ---------------------------------------------------------------------------
// atualizarBoss — decrementa timers visuais do Boss (mensagem + entrada).
//   Chamado a cada frame em cbIdle quando o jogo não está pausado.
// ---------------------------------------------------------------------------
static void atualizarBoss(float dt) {
    if (g_jogo.tempMensagemBoss > 0.0f) {
        g_jogo.tempMensagemBoss -= dt;
        if (g_jogo.tempMensagemBoss < 0.0f) g_jogo.tempMensagemBoss = 0.0f;
    }
    if (g_jogo.tempoEntradaBoss > 0.0f) {
        g_jogo.tempoEntradaBoss -= dt;
        if (g_jogo.tempoEntradaBoss < 0.0f) g_jogo.tempoEntradaBoss = 0.0f;
    }

    // Avança a animação do modelo enquanto o Boss estiver na fase ativa.
    if (g_cabecaCarregada && g_jogo.fasePartida == FASE_BOSS)
        g_cabeca.atualizar(dt);

    // Disparo em rajada circular do Boss
    if (g_jogo.fasePartida == FASE_BOSS) {
        for (size_t i = 0; i < g_jogo.horda.size(); ++i) {
            Zumbi& z = g_jogo.horda[i];
            if (!z.ehBoss || !z.vivo) continue;

            z.tiroTimer -= dt;
            if (z.tiroTimer <= 0.0f) {
                z.tiroTimer = BOSS_TIRO_COOLDOWN;

                // Rajada em círculo completo; o offset angular roda com o tempo
                // criando um padrão espiral que força o jogador a se mover.
                const float PI2 = 6.28318530f;
                float offset = g_jogo.tempoSobrevivido * 0.8f;
                for (int k = 0; k < BOSS_TIRO_COUNT; ++k) {
                    float ang = offset + (float)k * PI2 / (float)BOSS_TIRO_COUNT;
                    ProjetilZumbi pz;
                    pz.posicao    = z.posicao;
                    pz.direcao.x  = std::cos(ang);
                    pz.direcao.y  = 0.0f;
                    pz.direcao.z  = std::sin(ang);
                    pz.velocidade = BOSS_PROJETIL_VEL;
                    pz.dano       = BOSS_PROJETIL_DANO;
                    pz.ativo      = true;
                    pz.ehDoBoss   = true;
                    g_jogo.projeteisZumbi.push_back(pz);
                }
            }
            break; // só existe um Boss
        }
    }
}

// =============================================================================
//  SPAWN PERIÓDICO DE ZUMBIS
// =============================================================================

// Multiplicador de spawn para um tipo de zumbi no tempo t.
// Dobra a cada minuto desde o desbloqueio; 0 = ainda não aparece.
static int multiplicadorSpawn(TipoZumbi tipo, float t) {
    float desbloqueio = 0.0f;
    switch (tipo) {
        case NORMAL:    desbloqueio =   0.0f; break;
        case RAPIDO:    desbloqueio =  60.0f; break;
        case TANK:      desbloqueio = 120.0f; break;
        case ATIRADOR:  desbloqueio = 180.0f; break;
        case EXPLOSIVO: desbloqueio = 180.0f; break;
        default:        desbloqueio =   0.0f; break;
    }
    if (t < desbloqueio) return 0;
    int minutos = (int)((t - desbloqueio) / 60.0f);
    int mult = 1;
    for (int i = 0; i < minutos; ++i) {
        mult *= 2;
        if (mult >= SPAWN_MULT_MAX) { mult = SPAWN_MULT_MAX; break; }
    }
    return mult;
}

static void atualizarSpawn(float dt) {
    if (g_jogo.jogoPausado) return;

    g_jogo.tempoSobrevivido += dt;
    if (!g_faladaMagrelas && g_jogo.tempoSobrevivido >= 60.0f) {
        g_faladaMagrelas = true;
        tocarFalaPersonagem("Sons/Magrelas.mp3", VOLUME_FALA);   // canal dedicado
    }
    if (!g_faladaCarecas && g_jogo.tempoSobrevivido >= 120.0f) {
        g_faladaCarecas = true;
        tocarFalaPersonagem("Sons/Carecas.mp3", VOLUME_FALA);    // canal dedicado
    }
    if (!g_faladaFestaZumbi && g_jogo.tempoSobrevivido >= 180.0f) {
        g_faladaFestaZumbi = true;
        tocarFalaPersonagem("Sons/FestaZumbi.mp3", VOLUME_FALA); // canal dedicado
    }

    // =========================================================================
    // MÁQUINA DE ESTADOS DA PARTIDA
    //   Transições avaliadas a cada frame, independente do tick de spawn.
    // =========================================================================

    // Transição FASE_NORMAL → FASE_AGUARDANDO_BOSS ao atingir 4 minutos
#ifdef DEBUG_BOSS_SPAWN
    // DEBUG: invoca o Boss direto após 2 s (remove #define DEBUG_BOSS_SPAWN para desligar)
    if (g_jogo.fasePartida == FASE_NORMAL && g_jogo.tempoSobrevivido >= 2.0f &&
        !g_jogo.bossJaFoiInvocado) {
        g_jogo.horda.clear();
        invocarBoss();
    }
#else
    if (g_jogo.fasePartida == FASE_NORMAL &&
        g_jogo.tempoSobrevivido >= BOSS_TEMPO_TRIGGER) {
        g_jogo.fasePartida = FASE_AGUARDANDO_BOSS;
        tocarEfeito("Sons/Cuidado.mp3");
    }
#endif

    // FASE_AGUARDANDO_BOSS: assim que a arena estiver limpa, invoca o Boss.
    // Spawn já está bloqueado, então "vivos == 0" significa arena realmente vazia.
    if (g_jogo.fasePartida == FASE_AGUARDANDO_BOSS &&
        !g_jogo.bossJaFoiInvocado) {
        int vivos = 0;
        for (size_t s = 0; s < g_jogo.horda.size(); ++s)
            if (g_jogo.horda[s].vivo) vivos++;
        if (vivos == 0) invocarBoss();
    }

    // Bloqueia spawn durante espera e durante batalha do Boss
    if (g_jogo.fasePartida == FASE_AGUARDANDO_BOSS ||
        g_jogo.fasePartida == FASE_BOSS) {
        return; // nenhum zumbi novo enquanto Boss não for derrotado
    }
    // =========================================================================

    g_jogo.tempoUltimoSpawn += dt;
    if (g_jogo.tempoUltimoSpawn < SPAWN_INTERVALO) return;
    g_jogo.tempoUltimoSpawn = 0.0f;

    // Conta apenas zumbis VIVOS — mortos ficam no vetor como slots reaproveitáveis.
    // horda.size() cresce sem parar se usarmos push_back sempre; por isso contamos
    // vivos para o cap e reaproveitamos slots mortos antes de alocar novos.
    int ativosNaHorda = 0;
    for (size_t s = 0; s < g_jogo.horda.size(); ++s)
        if (g_jogo.horda[s].vivo) ativosNaHorda++;

    // Especiais primeiro: garantem slots antes dos normais (que têm mult altíssimo)
    const TipoZumbi tipos[5] = { EXPLOSIVO, ATIRADOR, TANK, RAPIDO, NORMAL };
    float t = g_jogo.tempoSobrevivido;

    for (int ti = 0; ti < 5; ++ti) {
        int mult = multiplicadorSpawn(tipos[ti], t);
        if (mult == 0) continue;
        int n = SPAWN_BASE * mult;
        for (int i = 0; i < n; ++i) {
            if (ativosNaHorda >= HORDA_MAX) break;

            // Tenta reaproveitar um slot morto para não crescer o vetor ao infinito.
            bool reutilizou = false;
            for (size_t s = 0; s < g_jogo.horda.size(); ++s) {
                if (!g_jogo.horda[s].vivo) {
                    g_jogo.horda[s] = criarZumbi(tipos[ti]);
                    reutilizou = true;
                    break;
                }
            }
            if (!reutilizou)
                g_jogo.horda.push_back(criarZumbi(tipos[ti]));

            ativosNaHorda++;
        }
        if (ativosNaHorda >= HORDA_MAX) break;
    }
}

// =============================================================================
//  ATUALIZAÇÃO: DEVORAR (corpo a corpo alto-risco/alta-recompensa, ESPAÇO)
// =============================================================================

static void atualizarDevorar(EstadoDoJogo& jogo, float dt) {
    Entidade& stand = jogo.stand;
    Jogador&  p     = jogo.protagonista;

    // -- Cooldown entre usos
    if (stand.temporizadorCooldownDevorar > 0.0f) {
        stand.temporizadorCooldownDevorar -= dt;
        if (stand.temporizadorCooldownDevorar < 0.0f)
            stand.temporizadorCooldownDevorar = 0.0f;
    }

    // -- Ativação pelo teclado (pedido registrado em cbKeyboard)
    if (stand.devorarPedido) {
        stand.devorarPedido = false;
        if (stand.temporizadorDevorar <= 0.0f &&
            stand.temporizadorCooldownDevorar <= 0.0f &&
            !stand.emSobrecarga) {
            stand.temporizadorDevorar         = DEVORAR_DURACAO_JANELA;
            stand.temporizadorCooldownDevorar = DEVORAR_COOLDOWN;
            if (DEVORAR_LUNGE > 0.0f) {
                p.posicao.x += std::cos(stand.anguloMira) * DEVORAR_LUNGE;
                p.posicao.z += std::sin(stand.anguloMira) * DEVORAR_LUNGE;
            }
        }
    }

    // -- Janela inativa: nada a fazer
    if (stand.temporizadorDevorar <= 0.0f) return;

    stand.temporizadorDevorar -= dt;
    bool janelaExpirou = (stand.temporizadorDevorar <= 0.0f);
    if (janelaExpirou) stand.temporizadorDevorar = 0.0f;

    // -- Procura o alvo mais próximo dentro do setor frontal (zumbi ou projétil)
    const float PI         = 3.14159265f;
    float meiaAb           = DEVORAR_ANGULO * 0.5f;
    Zumbi*        alvoZ    = NULL;
    ProjetilZumbi* alvoP   = NULL;
    float  menorDist       = DEVORAR_ALCANCE * 2.0f;

    for (size_t i = 0; i < jogo.horda.size(); ++i) {
        Zumbi& z = jogo.horda[i];
        if (!z.vivo) continue;
        float dx   = z.posicao.x - p.posicao.x;
        float dz   = z.posicao.z - p.posicao.z;
        float dist = std::sqrt(dx * dx + dz * dz);
        if (dist > DEVORAR_ALCANCE) continue;
        float angZ = std::atan2(dz, dx);
        float diff = angZ - stand.anguloMira;
        while (diff >  PI) diff -= 2.0f * PI;
        while (diff < -PI) diff += 2.0f * PI;
        if (diff < -meiaAb || diff > meiaAb) continue;
        if (dist < menorDist) { menorDist = dist; alvoZ = &z; alvoP = NULL; }
    }

    // Projéteis do Atirador também são devoráveis
    for (size_t i = 0; i < jogo.projeteisZumbi.size(); ++i) {
        ProjetilZumbi& pz = jogo.projeteisZumbi[i];
        if (!pz.ativo) continue;
        float dx   = pz.posicao.x - p.posicao.x;
        float dz   = pz.posicao.z - p.posicao.z;
        float dist = std::sqrt(dx * dx + dz * dz);
        if (dist > DEVORAR_ALCANCE) continue;
        float angZ = std::atan2(dz, dx);
        float diff = angZ - stand.anguloMira;
        while (diff >  PI) diff -= 2.0f * PI;
        while (diff < -PI) diff += 2.0f * PI;
        if (diff < -meiaAb || diff > meiaAb) continue;
        if (dist < menorDist) { menorDist = dist; alvoP = &pz; alvoZ = NULL; }
    }

    if (alvoZ != NULL) {
        // SUCESSO (zumbi): morte instantânea + purga tensão
        processarMorteZumbi(jogo, *alvoZ);
        stand.tensaoAtual = 0.0f;
        if (DEVORAR_SUCESSO_LIMPA_SOBRECARGA) stand.emSobrecarga = false;
        stand.temporizadorDevorar = 0.0f;
    } else if (alvoP != NULL) {
        // SUCESSO (projétil): destrói o projétil + purga tensão
        alvoP->ativo = false;
        stand.tensaoAtual = 0.0f;
        if (DEVORAR_SUCESSO_LIMPA_SOBRECARGA) stand.emSobrecarga = false;
        stand.temporizadorDevorar = 0.0f;
    } else if (janelaExpirou) {
        // ERRO: janela expirou sem acerto → sobrecarga imediata
        float maxT = maxTensaoDoNivel(jogo.protagonista.upgrades.niveis[TENSAO_UP]);
        stand.tensaoAtual  = maxT;
        stand.emSobrecarga = true;
    }
}

#endif // GAMEUPDATE_H
