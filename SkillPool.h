#ifndef SKILL_POOL_H
#define SKILL_POOL_H

// ===========================================================================
//  SkillPool.h — Wrapper de retrocompatibilidade; inclui RuntimeSkill.h.
// ===========================================================================

#include "RuntimeSkill.h"

// Inicializa RuntimeSkill como inativo; memset seguro pois é struct POD.
#include <cstring>
inline void zerarRuntime(RuntimeSkill& r) {
    std::memset(&r, 0, sizeof(RuntimeSkill));
    r.ativo  = false;
    r.idAlvo = -1;
}

// Conta instâncias ativas num pool — usado por HUD e diagnóstico.
#include <vector>
inline int contarAtivos(const std::vector<RuntimeSkill>& pool) {
    int n = 0;
    for (size_t i = 0; i < pool.size(); ++i)
        if (pool[i].ativo) ++n;
    return n;
}

#endif // SKILL_POOL_H