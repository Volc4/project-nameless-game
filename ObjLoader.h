#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

#include <GL/glut.h>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdio>

struct ObjModelo {
    std::vector<float> vertices;  // x,y,z por vértice
    std::vector<float> normais;
    std::vector<int>   indices;   // faces trianguladas
    std::vector<int>   indicesNormais;
    bool carregado = false;
};

inline bool carregarOBJ(const char* caminho, ObjModelo& modelo) {
    std::ifstream arquivo(caminho);
    if (!arquivo.is_open()) {
        printf("[OBJ] Nao foi possivel abrir: %s\n", caminho);
        return false;
    }

    std::vector<float> posV, normV;
    std::string linha;

    while (std::getline(arquivo, linha)) {
        std::istringstream ss(linha);
        std::string token;
        ss >> token;

        if (token == "v") {
            float x, y, z;
            ss >> x >> y >> z;
            posV.push_back(x);
            posV.push_back(y);
            posV.push_back(z);
        } else if (token == "vn") {
            float nx, ny, nz;
            ss >> nx >> ny >> nz;
            normV.push_back(nx);
            normV.push_back(ny);
            normV.push_back(nz);
        } else if (token == "f") {
            // Suporta formatos: v, v/vt, v//vn, v/vt/vn
            std::vector<int> idxV, idxVN;
            std::string part;
            while (ss >> part) {
                int v = 0, vt = 0, vn = 0;
                // Conta barras
                int barras = 0;
                for (char c : part) if (c == '/') barras++;

                if (barras == 0) {
                    sscanf(part.c_str(), "%d", &v);
                } else if (barras == 1) {
                    sscanf(part.c_str(), "%d/%d", &v, &vt);
                } else {
                    sscanf(part.c_str(), "%d/%d/%d", &v, &vt, &vn);
                    if (vt == 0) sscanf(part.c_str(), "%d//%d", &v, &vn);
                }
                idxV.push_back(v - 1);
                idxVN.push_back(vn - 1);
            }
            // Triangula faces com mais de 3 vértices (fan)
            for (int i = 1; i + 1 < (int)idxV.size(); ++i) {
                modelo.indices.push_back(idxV[0]);
                modelo.indices.push_back(idxV[i]);
                modelo.indices.push_back(idxV[i + 1]);
                modelo.indicesNormais.push_back(idxVN[0]);
                modelo.indicesNormais.push_back(idxVN[i]);
                modelo.indicesNormais.push_back(idxVN[i + 1]);
            }
        }
    }

    modelo.vertices = posV;
    modelo.normais  = normV;
    modelo.carregado = true;
    printf("[OBJ] Carregado: %s (%d triangulos)\n",
           caminho, (int)modelo.indices.size() / 3);
    return true;
}

inline void desenharOBJ(const ObjModelo& m) {
    if (!m.carregado) return;
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < (int)m.indices.size(); ++i) {
        int iv = m.indices[i];
        int in_ = m.indicesNormais[i];
        if (in_ >= 0 && in_ * 3 + 2 < (int)m.normais.size()) {
            glNormal3f(m.normais[in_*3], m.normais[in_*3+1], m.normais[in_*3+2]);
        }
        glVertex3f(m.vertices[iv*3], m.vertices[iv*3+1], m.vertices[iv*3+2]);
    }
    glEnd();
}

#endif