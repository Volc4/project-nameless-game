# Refatoração: Sistema de Habilidades por Componentes

## Visão Geral da Arquitetura

### Antes (acoplamento rígido)

```
dispararProjetil()
  └─ instanciarProjetil()
       └─ switch(tipoDisparoAtual)  ← 15 cases, cresce com cada novo tipo
            ├─ case DANO_CADENCIA: [configura p manualmente]
            ├─ case DANO_PERFURACAO: [configura p manualmente]
            └─ ... 13 outros cases

processarColisoesTiros()
  └─ if (tipoDisparoAtual == DISPARO_CADENCIA_VIDA)  ← enum vazando pelo código
  └─ if (tipoDisparoAtual == DISPARO_TENSAO_VIDA)
```

**Problema:** Adicionar uma nova arma exige editar 3 arquivos diferentes.

---

### Depois (componentes + composição)

```
skillManager.executar(jogo, alvo)
  └─ habilidadeAtiva->executar()    ← dispatch via vtable, O(1)
       └─ HabilidadeCone | HabilidadeRadial | HabilidadePerfurante | ...

processarColisoesTiros_Grade(jogo, grade, skills)
  └─ hab->efeitoCuraVida            ← flag de dado, não enum de tipo
  └─ hab->efeitoDrenarTensao
```

**Adicionar nova arma:** criar 1 struct em `Habilidade.h` + 1 condição em `fabricarHabilidade()`.

---

## Diagrama de Dependências

```
Entities.h
    │
    ├── MathUtils.h
    │
    ├── Habilidade.h          ← NOVO: interface + tipos concretos
    │       │
    │       └── SkillManager.h  ← NOVO: fábrica + composição + dispatch
    │               │
    │               ├── GameLogic.h    (alterado: remove switch, adiciona flags)
    │               └── SpatialGrid_updated.h  (alterado: +1 param)
    │
    └── Main.cpp              (alterado: +3 linhas)
```

---

## Fluxo de Dados: Upgrade → Projétil

```
Jogador escolhe upgrade "Cadência Nível 2"
    │
    ▼
aplicarUpgrade(jogo, CADENCIA)
    │
    ▼
skillManager.reconstruirHabilidade(upgrades)  ← único ponto de reconstrução
    │
    ▼
calcularAtributosBase()       → AtributosHabilidade base
    + aplicarModificadorDano(a, 0)            → sem alteração
    + aplicarModificadorCadencia(a, 2)        → quantidadeTiros=5, tiroRadial=false
    + aplicarModificadorPerfuracao(a, 0)      → sem alteração
    + aplicarModificadorTensao(a, 0)          → sem alteração
    + aplicarModificadorVelocidade(a, 0)      → sem alteração
    + aplicarModificadorVida(a, 0)            → sem alteração
    │
    ▼
fabricarHabilidade(a)
    └─ a.quantidadeTiros=5, !tiroRadial → new HabilidadeCone(a)
    │
    ▼
skillManager.habilidadeAtiva = HabilidadeCone{dano=1, tiros=5, spread=0.15}
```

---

## Como Adicionar uma Nova Arma (Exemplo: Aura de Gelo)

**1. Criar a struct em `Habilidade.h`:**

```cpp
struct HabilidadeAuraGelo : HabilidadeBase {
    float raioAura;

    explicit HabilidadeAuraGelo(const AtributosHabilidade& a)
        : HabilidadeBase(a), raioAura(5.0f) {}

    virtual void executar(EstadoDoJogo& jogo, Vetor3D /*alvo*/) {
        // Aplica slow em todos os zumbis dentro de raioAura
        for (size_t i = 0; i < jogo.horda.size(); ++i) {
            Zumbi& z = jogo.horda[i];
            if (!z.vivo) continue;
            float d2 = calcularDistanciaQuadrada(z.posicao, jogo.stand.posicao);
            if (d2 <= raioAura * raioAura)
                z.velocidade *= 0.5f; // slow temporário
        }
    }

    virtual HabilidadeBase* clone() const {
        return new HabilidadeAuraGelo(*this);
    }
};
```

**2. Adicionar flag em `AtributosHabilidade`:**

```cpp
bool efeitorAuraGelo;  // true quando os dois upgrades corretos se combinam
```

**3. Adicionar condição em `fabricarHabilidade()`:**

```cpp
if (a.efeitorAuraGelo)
    return new HabilidadeAuraGelo(a);
```

**4. Setar a flag no modificador correto em `SkillManager.h`:**

```cpp
// Exemplo: Velocidade Nível 3 + Tensão Nível 3 = Aura de Gelo
inline void aplicarModificadorCombinado(AtributosHabilidade& a,
                                         const SistemaUpgrades& upg) {
    if (upg.niveis[VELOCIDADE] >= 3 && upg.niveis[TENSAO_UP] >= 3)
        a.efeitorAuraGelo = true;
}
```

**Zero mudanças** em `Main.cpp`, `GameLogic.h` ou `SpatialGrid.h`.

---

## Compatibilidade com SpatialGrid

A `GradeEspacial` opera exclusivamente sobre `jogo.tirosNaTela` (vetor de `Projetil`).

As `Habilidade*` só fazem `jogo.tirosNaTela.push_back(p)` — a interface do vetor é inalterada.

A função `processarColisoesTiros_Grade()` recebe um parâmetro `const SkillManager& skills` adicional, mas internamente só acessa `skills.obterAtributos()` para ler dois bools — custo absoluto de leitura, zero overhead.

---

## Tabela de Arquivos Alterados

| Arquivo              | Tipo de mudança         | Linhas alteradas |
|----------------------|-------------------------|-----------------|
| `Habilidade.h`       | NOVO                    | —               |
| `SkillManager.h`     | NOVO                    | —               |
| `Entities.h`         | Remover enum/campos     | ~15 removidas   |
| `GameLogic.h`        | Remover switch/funções  | ~80 removidas, 5 adicionadas |
| `SpatialGrid.h`      | +1 parâmetro, 2 ifs     | ~8 alteradas    |
| `Main.cpp`           | +include, +global, +3 chamadas | ~6 adicionadas |
