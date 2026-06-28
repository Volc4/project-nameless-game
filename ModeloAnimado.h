#ifndef MODELO_ANIMADO_H
#define MODELO_ANIMADO_H

// =============================================================================
//  ModeloAnimado.h — Animação esquelética FBX (Mixamo) com Assimp
//
//  CPU Skinning real:
//    vertice_final = Σ peso_i · ( GlobalAnimado(osso_i) · OffsetMatrix_i ) · v_bind
//
//  - Lê bones (mesh->mBones), pesos por vértice, offset matrices
//  - Percorre a hierarquia de nós aplicando keyframes interpolados
//  - Interpolação: LERP de posição/escala, SLERP de rotação (quaternions)
//  - Texturas embutidas (Mixamo) via Assimp; externas via stb_image
//  - API: carregar / tocarAnimacao / atualizar(deltaTime) / renderizar()
//
//  Requisitos: OpenGL/FreeGLUT, Assimp, GLM (header-only), C++17
//
//  *** stb_image ***
//  Em UM único .cpp do projeto (ex.: Main.cpp), ANTES de incluir este header,
//  defina:
//      #define STB_IMAGE_IMPLEMENTATION
//  Coloque stb_image.h ao lado dos seus headers. Aqui ele é incluído sem a
//  macro, então só a declaração entra (a implementação fica no Main).
// =============================================================================

#include <GL/freeglut.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "stb_image.h"

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <cmath>
#include <functional>
#include <cstdlib>

class ModeloAnimado {
public:
    static const int MAX_PESOS = 4; // pesos por vértice (Mixamo usa <= 4)

private:
    // ---- Estruturas internas --------------------------------------------
    struct Vertice {
        glm::vec3 posBind;     // posição original (bind pose)
        glm::vec3 normalBind;  // normal original
        glm::vec2 uv;
        int   ossos[MAX_PESOS]; // índices de osso (-1 = vazio)
        float pesos[MAX_PESOS];
    };

    struct Malha {
        std::vector<Vertice>      vertices;
        std::vector<unsigned int> indices;   // triângulos (já triangulado)
        GLuint                    textura;    // 0 = sem textura
        glm::vec3                 corDifusa;  // fallback se não houver textura

        // buffer de posições/normais já "skinned" para este frame (CPU skinning)
        std::vector<glm::vec3> posSkin;
        std::vector<glm::vec3> normalSkin;
    };

    struct InfoOsso {
        glm::mat4 offset;       // bone offset matrix (mesh-space -> bone-space)
        glm::mat4 transformacaoFinal; // recalculada a cada frame
    };

    // ---- Estado Assimp / cena -------------------------------------------
    Assimp::Importer importer;
    const aiScene*   scene = nullptr;
    std::string      caminhoFicheiro;
    std::string      diretorioBase;

    glm::mat4 transformacaoGlobalInversa = glm::mat4(1.0f);

    // ---- Dados processados ----------------------------------------------
    std::vector<Malha>   malhas;
    std::vector<InfoOsso> ossos;
    std::map<std::string, int> mapaOssos; // nome do osso -> índice em 'ossos'

    std::map<std::string, int> mapaAnimacoes; // nome da animação -> índice

    // ---- Estado de reprodução -------------------------------------------
    int   animacaoAtual      = -1;
    float tempoAnimacao      = 0.0f;
    bool  emLoop             = true;
    float velocidadeAnimacao = 1.0f; // 1=normal, 0=congelado, -1=reversa

    // remoção de root motion: zera TODA translação do nó raiz animado
    bool        removerRootMotion    = true;
    std::string nomeOssoRaiz         = "mixamorig:Hips";
    bool        _rootBoneEncontrado  = false; // reset a cada frame antes de percorrerHierarquia

    // texturas embutidas já carregadas, indexadas por ponteiro aiTexture
    std::map<const aiTexture*, GLuint> cacheTexturasEmbutidas;

    // =====================================================================
    //  Conversão aiMatrix4x4 (row-major) -> glm::mat4 (column-major)
    //  Basta transpor.
    // =====================================================================
    static glm::mat4 paraGlm(const aiMatrix4x4& m) {
        glm::mat4 r;
        r[0][0]=m.a1; r[1][0]=m.a2; r[2][0]=m.a3; r[3][0]=m.a4;
        r[0][1]=m.b1; r[1][1]=m.b2; r[2][1]=m.b3; r[3][1]=m.b4;
        r[0][2]=m.c1; r[1][2]=m.c2; r[2][2]=m.c3; r[3][2]=m.c4;
        r[0][3]=m.d1; r[1][3]=m.d2; r[2][3]=m.d3; r[3][3]=m.d4;
        return r;
    }

    // =====================================================================
    //  Carregamento de textura
    // =====================================================================
    GLuint criarTexturaGL(unsigned char* dados, int largura, int altura, int canais) {
        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);

        GLenum formato = GL_RGB;
        if (canais == 1) formato = GL_LUMINANCE;
        else if (canais == 3) formato = GL_RGB;
        else if (canais == 4) formato = GL_RGBA;

        // gluBuild2DMipmaps funciona em GL legado sem precisar de glGenerateMipmap
        gluBuild2DMipmaps(GL_TEXTURE_2D, formato, largura, altura, formato,
                          GL_UNSIGNED_BYTE, dados);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindTexture(GL_TEXTURE_2D, 0);
        return tex;
    }

    GLuint carregarTexturaEmbutida(const aiTexture* aitex) {
        auto it = cacheTexturasEmbutidas.find(aitex);
        if (it != cacheTexturasEmbutidas.end()) return it->second;

        int largura = 0, altura = 0, canais = 0;
        unsigned char* dados = nullptr;

        if (aitex->mHeight == 0) {
            // textura comprimida (png/jpg) dentro do FBX
            dados = stbi_load_from_memory(
                reinterpret_cast<unsigned char*>(aitex->pcData),
                (int)aitex->mWidth, &largura, &altura, &canais, 0);
        } else {
            // textura crua ARGB8888 (aiTexel) -> converte para RGBA
            largura = aitex->mWidth;
            altura  = aitex->mHeight;
            canais  = 4;
            dados = (unsigned char*)malloc(largura * altura * 4);
            for (int i = 0; i < largura * altura; ++i) {
                dados[i*4+0] = aitex->pcData[i].r;
                dados[i*4+1] = aitex->pcData[i].g;
                dados[i*4+2] = aitex->pcData[i].b;
                dados[i*4+3] = aitex->pcData[i].a;
            }
        }

        GLuint tex = 0;
        if (dados) {
            tex = criarTexturaGL(dados, largura, altura, canais);
            free(dados);
        } else {
            std::cout << "[Textura] Falha ao decodificar textura embutida\n";
        }
        cacheTexturasEmbutidas[aitex] = tex;
        return tex;
    }

    GLuint carregarTexturaExterna(const std::string& arquivo) {
        std::string caminho = diretorioBase.empty() ? arquivo
                                                     : diretorioBase + "/" + arquivo;
        int largura, altura, canais;
        unsigned char* dados = stbi_load(caminho.c_str(), &largura, &altura, &canais, 0);
        if (!dados) {
            std::cout << "[Textura] Nao foi possivel abrir: " << caminho << "\n";
            return 0;
        }
        GLuint tex = criarTexturaGL(dados, largura, altura, canais);
        stbi_image_free(dados);
        return tex;
    }

    GLuint resolverTexturaDifusa(const aiMaterial* mat) {
        if (!mat) return 0;
        if (mat->GetTextureCount(aiTextureType_DIFFUSE) == 0) return 0;

        aiString caminhoTex;
        if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &caminhoTex) != AI_SUCCESS)
            return 0;

        // textura embutida vem como "*0", "*1"...  ou Assimp resolve via GetEmbeddedTexture
        const aiTexture* embutida = scene->GetEmbeddedTexture(caminhoTex.C_Str());
        if (embutida) return carregarTexturaEmbutida(embutida);

        return carregarTexturaExterna(caminhoTex.C_Str());
    }

    // =====================================================================
    //  Processamento da malha: vértices, normais, UVs, ossos e pesos
    // =====================================================================
    void processarMalha(const aiMesh* mesh) {
        Malha m;
        m.textura = 0;
        m.corDifusa = glm::vec3(0.8f);

        size_t nv = mesh->mNumVertices;
        m.vertices.resize(nv);

        for (size_t i = 0; i < nv; ++i) {
            Vertice& v = m.vertices[i];
            v.posBind = glm::vec3(mesh->mVertices[i].x,
                                  mesh->mVertices[i].y,
                                  mesh->mVertices[i].z);
            if (mesh->HasNormals())
                v.normalBind = glm::vec3(mesh->mNormals[i].x,
                                         mesh->mNormals[i].y,
                                         mesh->mNormals[i].z);
            else
                v.normalBind = glm::vec3(0,1,0);

            if (mesh->mTextureCoords[0])
                v.uv = glm::vec2(mesh->mTextureCoords[0][i].x,
                                 mesh->mTextureCoords[0][i].y);
            else
                v.uv = glm::vec2(0.0f);

            for (int k = 0; k < MAX_PESOS; ++k) { v.ossos[k] = -1; v.pesos[k] = 0.0f; }
        }

        // índices (já triangulado por aiProcess_Triangulate)
        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace& face = mesh->mFaces[f];
            for (unsigned int k = 0; k < face.mNumIndices; ++k)
                m.indices.push_back(face.mIndices[k]);
        }

        // ---- Ossos e pesos ----
        for (unsigned int b = 0; b < mesh->mNumBones; ++b) {
            const aiBone* bone = mesh->mBones[b];
            std::string nome = bone->mName.C_Str();

            int indiceOsso;
            auto it = mapaOssos.find(nome);
            if (it == mapaOssos.end()) {
                indiceOsso = (int)ossos.size();
                InfoOsso info;
                info.offset = paraGlm(bone->mOffsetMatrix);
                info.transformacaoFinal = glm::mat4(1.0f);
                ossos.push_back(info);
                mapaOssos[nome] = indiceOsso;
            } else {
                indiceOsso = it->second;
            }

            for (unsigned int w = 0; w < bone->mNumWeights; ++w) {
                unsigned int vid = bone->mWeights[w].mVertexId;
                float peso       = bone->mWeights[w].mWeight;
                if (peso <= 0.0f) continue;

                Vertice& v = m.vertices[vid];
                // insere no primeiro slot livre; se cheio, substitui o menor
                int slotMenor = 0;
                bool inserido = false;
                for (int k = 0; k < MAX_PESOS; ++k) {
                    if (v.ossos[k] < 0) {
                        v.ossos[k] = indiceOsso;
                        v.pesos[k] = peso;
                        inserido = true;
                        break;
                    }
                    if (v.pesos[k] < v.pesos[slotMenor]) slotMenor = k;
                }
                if (!inserido && peso > v.pesos[slotMenor]) {
                    v.ossos[slotMenor] = indiceOsso;
                    v.pesos[slotMenor] = peso;
                }
            }
        }

        // normaliza pesos (garante soma = 1; se vértice sem osso, fica estático)
        for (auto& v : m.vertices) {
            float soma = 0.0f;
            for (int k = 0; k < MAX_PESOS; ++k) soma += v.pesos[k];
            if (soma > 0.0001f)
                for (int k = 0; k < MAX_PESOS; ++k) v.pesos[k] /= soma;
        }

        // ---- Material / textura ----
        if (mesh->mMaterialIndex < scene->mNumMaterials) {
            const aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
            m.textura = resolverTexturaDifusa(mat);

            aiColor3D cor(0.8f, 0.8f, 0.8f);
            if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, cor) == AI_SUCCESS)
                m.corDifusa = glm::vec3(cor.r, cor.g, cor.b);
        }

        m.posSkin.resize(nv);
        m.normalSkin.resize(nv);
        malhas.push_back(std::move(m));
    }

    // =====================================================================
    //  Interpolação de keyframes
    // =====================================================================
    static unsigned int acharIndice(double t, unsigned int n,
                                    std::function<double(unsigned int)> tempoDe) {
        for (unsigned int i = 0; i + 1 < n; ++i)
            if (t < tempoDe(i + 1)) return i;
        return (n > 0) ? n - 1 : 0;
    }

    glm::vec3 interpolarPosicao(const aiNodeAnim* canal, double t) {
        if (canal->mNumPositionKeys == 1) {
            auto v = canal->mPositionKeys[0].mValue;
            return glm::vec3(v.x, v.y, v.z);
        }
        unsigned int i = acharIndice(t, canal->mNumPositionKeys,
            [&](unsigned int k){ return canal->mPositionKeys[k].mTime; });
        unsigned int j = i + 1;
        if (j >= canal->mNumPositionKeys) j = i;

        double t0 = canal->mPositionKeys[i].mTime;
        double t1 = canal->mPositionKeys[j].mTime;
        float fator = (t1 > t0) ? (float)((t - t0) / (t1 - t0)) : 0.0f;

        auto a = canal->mPositionKeys[i].mValue;
        auto b = canal->mPositionKeys[j].mValue;
        glm::vec3 va(a.x, a.y, a.z), vb(b.x, b.y, b.z);
        return glm::mix(va, vb, fator);
    }

    glm::quat interpolarRotacao(const aiNodeAnim* canal, double t) {
        if (canal->mNumRotationKeys == 1) {
            auto q = canal->mRotationKeys[0].mValue;
            return glm::quat(q.w, q.x, q.y, q.z);
        }
        unsigned int i = acharIndice(t, canal->mNumRotationKeys,
            [&](unsigned int k){ return canal->mRotationKeys[k].mTime; });
        unsigned int j = i + 1;
        if (j >= canal->mNumRotationKeys) j = i;

        double t0 = canal->mRotationKeys[i].mTime;
        double t1 = canal->mRotationKeys[j].mTime;
        float fator = (t1 > t0) ? (float)((t - t0) / (t1 - t0)) : 0.0f;

        auto a = canal->mRotationKeys[i].mValue;
        auto b = canal->mRotationKeys[j].mValue;
        glm::quat qa(a.w, a.x, a.y, a.z);
        glm::quat qb(b.w, b.x, b.y, b.z);
        return glm::normalize(glm::slerp(qa, qb, fator));
    }

    glm::vec3 interpolarEscala(const aiNodeAnim* canal, double t) {
        if (canal->mNumScalingKeys == 1) {
            auto v = canal->mScalingKeys[0].mValue;
            return glm::vec3(v.x, v.y, v.z);
        }
        unsigned int i = acharIndice(t, canal->mNumScalingKeys,
            [&](unsigned int k){ return canal->mScalingKeys[k].mTime; });
        unsigned int j = i + 1;
        if (j >= canal->mNumScalingKeys) j = i;

        double t0 = canal->mScalingKeys[i].mTime;
        double t1 = canal->mScalingKeys[j].mTime;
        float fator = (t1 > t0) ? (float)((t - t0) / (t1 - t0)) : 0.0f;

        auto a = canal->mScalingKeys[i].mValue;
        auto b = canal->mScalingKeys[j].mValue;
        glm::vec3 va(a.x, a.y, a.z), vb(b.x, b.y, b.z);
        return glm::mix(va, vb, fator);
    }

    const aiNodeAnim* canalDoNo(const aiAnimation* anim, const std::string& nome) {
        for (unsigned int i = 0; i < anim->mNumChannels; ++i)
            if (std::string(anim->mChannels[i]->mNodeName.C_Str()) == nome)
                return anim->mChannels[i];
        return nullptr;
    }

    // =====================================================================
    //  Percorre a hierarquia acumulando transformações e preenchendo
    //  ossos[].transformacaoFinal
    // =====================================================================
    void percorrerHierarquia(const aiNode* node, const glm::mat4& paiTransform,
                             double tempoTicks, const aiAnimation* anim) {
        std::string nome = node->mName.C_Str();
        glm::mat4 transformacaoNo = paraGlm(node->mTransformation);

        const aiNodeAnim* canal = anim ? canalDoNo(anim, nome) : nullptr;
        if (canal) {
            glm::vec3 pos = interpolarPosicao(canal, tempoTicks);
            glm::quat rot = interpolarRotacao(canal, tempoTicks);
            glm::vec3 esc = interpolarEscala(canal, tempoTicks);

            // Remove root motion: zera TODA translação (X,Y,Z) do nó raiz animado.
            // Detectado por nome OU como o primeiro nó com canal nesta frame.
            if (removerRootMotion && (!_rootBoneEncontrado || nome == nomeOssoRaiz)) {
                pos = glm::vec3(0.0f);
                _rootBoneEncontrado = true;
            }

            glm::mat4 T = glm::translate(glm::mat4(1.0f), pos);
            glm::mat4 R = glm::mat4_cast(rot);
            glm::mat4 S = glm::scale(glm::mat4(1.0f), esc);
            transformacaoNo = T * R * S;
        }

        glm::mat4 global = paiTransform * transformacaoNo;

        auto it = mapaOssos.find(nome);
        if (it != mapaOssos.end()) {
            int idx = it->second;
            ossos[idx].transformacaoFinal =
                transformacaoGlobalInversa * global * ossos[idx].offset;
        }

        for (unsigned int i = 0; i < node->mNumChildren; ++i)
            percorrerHierarquia(node->mChildren[i], global, tempoTicks, anim);
    }

    // =====================================================================
    //  CPU Skinning: aplica as matrizes finais aos vértices da bind pose
    // =====================================================================
    void aplicarSkinning() {
        bool temAnim = (animacaoAtual >= 0 && !ossos.empty());

        for (auto& m : malhas) {
            for (size_t i = 0; i < m.vertices.size(); ++i) {
                const Vertice& v = m.vertices[i];

                if (!temAnim || v.ossos[0] < 0) {
                    // sem ossos: mantém a bind pose
                    m.posSkin[i]    = v.posBind;
                    m.normalSkin[i] = v.normalBind;
                    continue;
                }

                glm::mat4 skin(0.0f);
                for (int k = 0; k < MAX_PESOS; ++k) {
                    if (v.ossos[k] < 0) continue;
                    skin += ossos[v.ossos[k]].transformacaoFinal * v.pesos[k];
                }

                glm::vec4 p = skin * glm::vec4(v.posBind, 1.0f);
                m.posSkin[i] = glm::vec3(p);

                glm::mat3 normalMat = glm::mat3(skin);
                m.normalSkin[i] = glm::normalize(normalMat * v.normalBind);
            }
        }
    }

    // =====================================================================
    //  Avança a hierarquia para o tempo atual (chamado em atualizar/render)
    // =====================================================================
    void calcularPose() {
        if (animacaoAtual < 0 || (unsigned)animacaoAtual >= scene->mNumAnimations) {
            // sem animação ativa: ossos em pose neutra (identidade da hierarquia)
            const aiNode* raiz = scene->mRootNode;
            percorrerHierarquia(raiz, glm::mat4(1.0f), 0.0, nullptr);
            aplicarSkinning();
            return;
        }

        const aiAnimation* anim = scene->mAnimations[animacaoAtual];
        double tps = (anim->mTicksPerSecond != 0.0) ? anim->mTicksPerSecond : 25.0;
        double duracaoSeg = anim->mDuration / tps;

        double t = tempoAnimacao;
        if (emLoop && duracaoSeg > 0.0)
            t = std::fmod(t, duracaoSeg);
        else if (t > duracaoSeg)
            t = duracaoSeg;

        double tempoTicks = t * tps;

        _rootBoneEncontrado = false; // reseta a cada frame para detectar o root bone
        percorrerHierarquia(scene->mRootNode, glm::mat4(1.0f), tempoTicks, anim);
        aplicarSkinning();
    }

public:
    ModeloAnimado() = default;

    // ---------------------------------------------------------------------
    bool carregar(const std::string& caminho) {
        caminhoFicheiro = caminho;

        size_t barra = caminho.find_last_of("/\\");
        diretorioBase = (barra == std::string::npos) ? "" : caminho.substr(0, barra);

        scene = importer.ReadFile(caminho,
            aiProcess_Triangulate |
            aiProcess_FlipUVs |
            aiProcess_GenSmoothNormals |
            aiProcess_LimitBoneWeights |   // garante <= 4 pesos por vértice
            aiProcess_JoinIdenticalVertices);

        if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
            std::cout << "Erro ao carregar FBX: " << importer.GetErrorString() << std::endl;
            scene = nullptr;
            return false;
        }

        transformacaoGlobalInversa =
            glm::inverse(paraGlm(scene->mRootNode->mTransformation));

        // processa todas as malhas
        malhas.clear();
        ossos.clear();
        mapaOssos.clear();
        for (unsigned int i = 0; i < scene->mNumMeshes; ++i)
            processarMalha(scene->mMeshes[i]);

        // mapeia animações
        mapaAnimacoes.clear();
        std::cout << "\n--- Animacoes Encontradas no FBX ---" << std::endl;
        for (unsigned int i = 0; i < scene->mNumAnimations; ++i) {
            std::string nome = scene->mAnimations[i]->mName.C_Str();
            mapaAnimacoes[nome] = (int)i;
            std::cout << "-> Nome para usar no codigo: \"" << nome << "\""
                      << "  (" << scene->mAnimations[i]->mNumChannels << " canais)"
                      << std::endl;
        }
        std::cout << "Ossos: " << ossos.size()
                  << " | Malhas: " << malhas.size() << std::endl;
        std::cout << "------------------------------------\n" << std::endl;

        // calcula uma pose inicial (bind/neutra)
        calcularPose();
        return true;
    }

    // ---------------------------------------------------------------------
    //  Troca a animação por nome. Reinicia o tempo da animação.
    //  Retorna false se o nome não existir.
    // ---------------------------------------------------------------------
    bool tocarAnimacao(const std::string& nome, bool loop = true) {
        auto it = mapaAnimacoes.find(nome);
        if (it == mapaAnimacoes.end()) {
            std::cout << "[Animacao] Nome nao encontrado: \"" << nome << "\"\n";
            return false;
        }
        if (animacaoAtual != it->second) {
            animacaoAtual = it->second;
            tempoAnimacao = 0.0f;
        }
        emLoop = loop;
        return true;
    }

    // Congela em um frame específico da animação atual (calculando pelo TPS)
    void congelarNoFrame(int frame) {
        velocidadeAnimacao = 0.0f;
        if (scene && animacaoAtual >= 0 && (unsigned)animacaoAtual < scene->mNumAnimations) {
            const aiAnimation* anim = scene->mAnimations[animacaoAtual];
            double tps = (anim->mTicksPerSecond != 0.0) ? anim->mTicksPerSecond : 25.0;
            // tempo = frame / ticks_por_segundo
            tempoAnimacao = (float)(frame / tps);
        } else {
            tempoAnimacao = 0.0f;
        }
    }

    // Para completamente e volta à bind pose.
    void pararAnimacao() {
        animacaoAtual      = -1;
        tempoAnimacao      = 0.0f;
        velocidadeAnimacao = 1.0f;
    }

    // 1.0=normal | 0.0=congelado | -1.0=reversa
    void definirVelocidade(float v) { velocidadeAnimacao = v; }

    // ---------------------------------------------------------------------
    //  Avança o tempo da animação e recalcula a pose + skinning.
    // ---------------------------------------------------------------------
    void atualizar(float deltaTime) {
        if (!scene) return;

        if (animacaoAtual < 0 || velocidadeAnimacao == 0.0f) {
            calcularPose();
            return;
        }

        tempoAnimacao += deltaTime * velocidadeAnimacao;

        // loop com suporte a valores negativos
        if (emLoop && (unsigned)animacaoAtual < scene->mNumAnimations) {
            const aiAnimation* anim = scene->mAnimations[animacaoAtual];
            double tps = (anim->mTicksPerSecond != 0.0) ? anim->mTicksPerSecond : 25.0;
            float dur = (float)(anim->mDuration / tps);
            if (dur > 0.0f) {
                tempoAnimacao = fmodf(tempoAnimacao, dur);
                if (tempoAnimacao < 0.0f) tempoAnimacao += dur;
            }
        }

        calcularPose();
    }

    // ---------------------------------------------------------------------
    //  Desenha as malhas já "skinned" deste frame.
    //  Respeita a matriz de modelview corrente (posição/escala do chamador).
    // ---------------------------------------------------------------------
    void renderizar() {
        if (!scene) return;

        glEnableClientState(GL_VERTEX_ARRAY); // só p/ deixar estado limpo
        glDisableClientState(GL_VERTEX_ARRAY);

        for (const auto& m : malhas) {
            bool usaTextura = (m.textura != 0);

            if (usaTextura) {
                glEnable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, m.textura);
                glColor3f(1.0f, 1.0f, 1.0f);
            } else {
                glDisable(GL_TEXTURE_2D);
                glColor3f(m.corDifusa.r, m.corDifusa.g, m.corDifusa.b);
            }

            glBegin(GL_TRIANGLES);
            for (size_t i = 0; i < m.indices.size(); ++i) {
                unsigned int idx = m.indices[i];
                const Vertice& v = m.vertices[idx];

                if (usaTextura) glTexCoord2f(v.uv.x, v.uv.y);

                const glm::vec3& n = m.normalSkin[idx];
                glNormal3f(n.x, n.y, n.z);

                const glm::vec3& p = m.posSkin[idx];
                glVertex3f(p.x, p.y, p.z);
            }
            glEnd();
        }

        glDisable(GL_TEXTURE_2D);
        glColor3f(1.0f, 1.0f, 1.0f);
    }

    // ---------------------------------------------------------------------
    //  Compatibilidade com a assinatura antiga:
    //     renderizar(tempoGlobal, nomeAnimacao)
    //  Se o seu Main ainda chamar assim, isto continua funcionando:
    //  troca a animação (se mudou) e desenha com o tempo absoluto dado.
    //  PREFIRA usar atualizar(deltaTime) + renderizar() no loop principal.
    // ---------------------------------------------------------------------
    void renderizar(float tempoGlobal, const std::string& nomeAnimacao) {
        if (!scene) return;
        if (!nomeAnimacao.empty()) tocarAnimacao(nomeAnimacao, true);
        else                       pararAnimacao();
        tempoAnimacao = tempoGlobal; // tempo absoluto
        calcularPose();
        renderizar();
    }

    const std::map<std::string,int>& animacoesDisponiveis() const {
        return mapaAnimacoes;
    }

    // Força uma textura PNG/JPG externa em todas as malhas (útil quando o
    // material do FBX não referencia textura, como nos exports do Mixamo/Blender).
    void definirTexturaManual(const std::string& arquivoImagem) {
        GLuint tex = carregarTexturaExterna(arquivoImagem);
        if (tex == 0) {
            std::cout << "[Textura] definirTexturaManual falhou: "
                      << arquivoImagem << std::endl;
            return;
        }
        for (auto& m : malhas) m.textura = tex;
    }

    // Liga/desliga remoção de root motion e (opcional) muda o osso raiz.
    void configurarRootMotion(bool remover, const std::string& ossoRaiz = "") {
        removerRootMotion = remover;
        if (!ossoRaiz.empty()) nomeOssoRaiz = ossoRaiz;
    }
};

#endif // MODELO_ANIMADO_H