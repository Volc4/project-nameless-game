#ifndef SKILL_FACTORY_H
#define SKILL_FACTORY_H

// ===========================================================================
//  SkillFactory.h — Registro central e fábrica de Skills por identificador
//                   (Fase 5 — Composable Skills, modelo data-driven)
//
//  PAPEL
//  -----
//  Desacopla NOMES de habilidade do motor. O SkillManager nunca conhece
//  "Laser" ou "Meteoro"; ele pede à factory uma SkillData por id:
//      int id = SkillFactory::id("Laser");
//      SkillData s = SkillFactory::criar(id);
//
//  Cada habilidade é registrada UMA vez como uma SkillData (composição de
//  componentes-dados: Forma + Movimento + Origem + Efeito). Adicionar uma
//  habilidade nova = uma chamada RegistrarSkill(...) — nada mais no motor.
//
//  POR QUE MELHORA ESCALABILIDADE
//  ------------------------------
//  Centenas de habilidades viram entradas numa tabela. O núcleo (SkillManager,
//  executor, colisão, render loop) permanece imutável. É o objetivo
//  arquitetural da Fase 5 cumprido sem classes de habilidade.
//
//  C++98: tabela = array fixo de structs POD + nome em char[]. Sem
//  unordered_map, sem std::string como chave, sem ponteiros donos.
//  Busca por nome é linear sobre poucas dezenas de entradas (custo irrelevante,
//  feita só no registro/lookup inicial, nunca no hot path).
// ===========================================================================

#include "SkillTypes.h"
#include "SkillValidator.h"
#include <cstring>   // strncmp, strncpy

#define SKILL_FACTORY_MAX        128   // teto de habilidades registradas
#define SKILL_NOME_MAX            32    // tamanho máximo de um identificador

// ---------------------------------------------------------------------------
// EntradaSkill — uma habilidade registrada: nome + template de SkillData.
//   POD: nome em buffer fixo, SkillData é POD. memcpy-able, serializável.
// ---------------------------------------------------------------------------
struct EntradaSkill {
    char      nome[SKILL_NOME_MAX];
    SkillData modelo;        // template imutável da build
    bool      ocupada;
};

// ---------------------------------------------------------------------------
// SkillFactory — registro estático global (singleton-by-data, sem classe-dona).
//   Implementado como struct com armazenamento estático acessado por funções
//   estáticas. Nenhuma instância é necessária.
// ---------------------------------------------------------------------------
struct SkillFactory {

    // Armazenamento. Definido inline (C++98 permite static membro via função).
    static EntradaSkill* tabela() {
        static EntradaSkill _tab[SKILL_FACTORY_MAX];
        return _tab;
    }
    static int& contador() {
        static int _n = 0;
        return _n;
    }

    // -----------------------------------------------------------------------
    // registrar — adiciona/atualiza uma habilidade pelo nome. Retorna o id
    //   (índice na tabela) ou -1 se a tabela estiver cheia.
    //   O modelo recebido já deve ser uma SkillData válida (montada por
    //   composição de componentes). Atribui modelo.id = id para coerência.
    // -----------------------------------------------------------------------
    static int registrar(const char* nome, const SkillData& modelo) {
        // já existe? atualiza.
        int existente = id(nome);
        if (existente >= 0) {
            tabela()[existente].modelo = modelo;
            tabela()[existente].modelo.id = existente;
            return existente;
        }
        int& n = contador();
        if (n >= SKILL_FACTORY_MAX) return -1;
        EntradaSkill& e = tabela()[n];
        std::strncpy(e.nome, nome, SKILL_NOME_MAX - 1);
        e.nome[SKILL_NOME_MAX - 1] = '\0';
        e.modelo    = modelo;
        e.modelo.id = n;
        e.ocupada   = true;
        return n++;
    }

    // -----------------------------------------------------------------------
    // id — resolve um identificador textual para índice. -1 se não existe.
    //   Busca linear (poucas dezenas de entradas; fora do hot path).
    // -----------------------------------------------------------------------
    static int id(const char* nome) {
        EntradaSkill* t = tabela();
        int n = contador();
        for (int i = 0; i < n; ++i) {
            if (!t[i].ocupada) continue;
            if (std::strncmp(t[i].nome, nome, SKILL_NOME_MAX) == 0)
                return i;
        }
        return -1;
    }

    // -----------------------------------------------------------------------
    // criar — devolve uma CÓPIA do modelo registrado (por id). A cópia é o
    //   que o SkillManager usa como build ativa; instâncias RuntimeSkill são
    //   geradas a partir dela. Se id inválido, devolve uma SkillData zerada
    //   marcada como inativa (fallback seguro, nunca crash).
    // -----------------------------------------------------------------------
    static SkillData criar(int idSkill) {
        if (idSkill < 0 || idSkill >= contador()) {
            SkillData vazia;
            std::memset(&vazia, 0, sizeof(vazia));
            vazia.ativa = false;
            return vazia;
        }
        return tabela()[idSkill].modelo;
    }

    // -----------------------------------------------------------------------
    // criarPorNome — atalho conveniente (lookup + criar).
    // -----------------------------------------------------------------------
    static SkillData criarPorNome(const char* nome) {
        return criar(id(nome));
    }

    static int total() { return contador(); }

    static const char* nomeDe(int idSkill) {
        if (idSkill < 0 || idSkill >= contador()) return "";
        return tabela()[idSkill].nome;
    }
};

// ---------------------------------------------------------------------------
// RegistrarSkill — açúcar sintático no estilo pedido pela Fase 5:
//     RegistrarSkill("Laser", build);
// ---------------------------------------------------------------------------
inline int RegistrarSkill(const char* nome, const SkillData& modelo) {
    return SkillFactory::registrar(nome, modelo);
}

#endif // SKILL_FACTORY_H