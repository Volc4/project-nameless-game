#ifndef SKILL_FACTORY_H
#define SKILL_FACTORY_H

// ===========================================================================
//  SkillFactory.h — Fábrica de skills por nome ou id
//
//  Responsabilidades:
//    - Guardar um catálogo global de SkillData registradas por nome textual.
//    - Fornecer criação por id (criar), busca de id por nome (id),
//      nome por id (nomeDe), total de skills registradas (total).
//    - RegistrarSkill() é a única entrada de registro; chamada uma vez
//      em registrarSkillsPadrao() durante a inicialização do jogo.
//
//  A factory NÃO conhece nenhuma skill concreta — elas estão em
//  SkillRegistry.h. Ela é apenas o repositório.
//
//  C++98: array fixo, sem std::map, sem std::string (C-strings).
// ===========================================================================

#include "SkillTypes.h"
#include <cstring>   // strncpy, strcmp

// Número máximo de skills distintas no catálogo
#define MAX_SKILLS_CATALOGO  128
// Tamanho máximo do nome de uma skill
#define SKILL_NOME_MAX        64

// ---------------------------------------------------------------------------
// _EntradaFactory — par (nome, SkillData) no catálogo interno
// ---------------------------------------------------------------------------
struct _EntradaFactory {
    char      nome[SKILL_NOME_MAX];
    SkillData dados;
    bool      ocupada;
};

// ---------------------------------------------------------------------------
// _getCatalogo — catálogo singleton (array estático com guard de acesso).
//   Evita o problema de inicialização de globais em C++98.
// ---------------------------------------------------------------------------
inline _EntradaFactory* _getCatalogo() {
    static _EntradaFactory catalogo[MAX_SKILLS_CATALOGO];
    static bool inicializado = false;
    if (!inicializado) {
        inicializado = true;
        for (int i = 0; i < MAX_SKILLS_CATALOGO; ++i) {
            catalogo[i].ocupada = false;
            catalogo[i].nome[0] = '\0';
        }
    }
    return catalogo;
}

inline int& _getTotalRegistradas() {
    static int total = 0;
    return total;
}

// ---------------------------------------------------------------------------
// RegistrarSkill — registra uma nova skill no catálogo global.
//   Chamado apenas por registrarSkillsPadrao() (SkillRegistry.h).
//   Preenche o campo id da SkillData com o índice de registro.
// ---------------------------------------------------------------------------
inline void RegistrarSkill(const char* nome, SkillData dados) {
    _EntradaFactory* cat = _getCatalogo();
    int& total = _getTotalRegistradas();
    if (total >= MAX_SKILLS_CATALOGO) return;   // catálogo cheio

    dados.id = total;
    dados.ativa = true;

    _EntradaFactory& e = cat[total];
    std::strncpy(e.nome, nome, SKILL_NOME_MAX - 1);
    e.nome[SKILL_NOME_MAX - 1] = '\0';
    e.dados   = dados;
    e.ocupada = true;

    total++;
}

// ===========================================================================
//  SkillFactory — namespace de acesso ao catálogo (funções estáticas)
// ===========================================================================
struct SkillFactory {

    // Devolve o id de uma skill pelo nome (-1 se não encontrada).
    static int id(const char* nome) {
        _EntradaFactory* cat = _getCatalogo();
        int total = _getTotalRegistradas();
        for (int i = 0; i < total; ++i) {
            if (cat[i].ocupada && std::strcmp(cat[i].nome, nome) == 0)
                return i;
        }
        return -1;
    }

    // Cria (devolve por valor) a SkillData de um id.
    // Se o id for inválido, devolve uma SkillData inativa.
    static SkillData criar(int idSkill) {
        _EntradaFactory* cat = _getCatalogo();
        int total = _getTotalRegistradas();
        if (idSkill < 0 || idSkill >= total || !cat[idSkill].ocupada) {
            SkillData vazio = buildBase();
            vazio.ativa = false;
            return vazio;
        }
        return cat[idSkill].dados;
    }

    // Devolve o nome de uma skill pelo id (string literal ou "?" se inválido).
    static const char* nomeDe(int idSkill) {
        _EntradaFactory* cat = _getCatalogo();
        int total = _getTotalRegistradas();
        if (idSkill < 0 || idSkill >= total || !cat[idSkill].ocupada)
            return "?";
        return cat[idSkill].nome;
    }

    // Número total de skills registradas.
    static int total() {
        return _getTotalRegistradas();
    }

private:
    // Não instanciável — apenas funções estáticas.
    SkillFactory();
};

#endif // SKILL_FACTORY_H
