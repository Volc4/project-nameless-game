#ifndef SKILL_POOL_H
#define SKILL_POOL_H

// ===========================================================================
//  SkillPool.h — Pool de instâncias RuntimeSkill
//
//  As funções de pool (adicionarAoPool, compactarPool, zerarRuntime,
//  contarAtivos) foram consolidadas em RuntimeSkill.h junto com a struct,
//  eliminando a duplicação que causava ODR violations.
//
//  Este header existe para retrocompatibilidade: qualquer código que
//  incluía SkillPool.h continua funcionando sem alteração.
// ===========================================================================

#include "RuntimeSkill.h"

// zerarRuntime — inicializa um RuntimeSkill como inativo/limpo.
//   Definida aqui (não em RuntimeSkill.h) para evitar duplicata.
//   POD, então memset é seguro e barato.
#include <cstring>
inline void zerarRuntime(RuntimeSkill& r) {
    std::memset(&r, 0, sizeof(RuntimeSkill));
    r.ativo  = false;
    r.idAlvo = -1;
}

// contarAtivos — utilitário de diagnóstico/HUD.
#include <vector>
inline int contarAtivos(const std::vector<RuntimeSkill>& pool) {
    int n = 0;
    for (size_t i = 0; i < pool.size(); ++i)
        if (pool[i].ativo) ++n;
    return n;
}

#endif // SKILL_POOL_H