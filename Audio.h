#ifndef AUDIO_H
#define AUDIO_H

#include <windows.h>
#include <mmsystem.h>
#include <string>
#include <sstream>

// =======================================================
// MUSICA DE FUNDO (Loop infinito)
// =======================================================
inline void tocarMusicaFundo(const char* caminhoMp3) {
    mciSendString("stop musica_fundo", NULL, 0, NULL);
    mciSendString("close musica_fundo", NULL, 0, NULL);

    std::string comandoOpen = std::string("open \"") + caminhoMp3 + "\" type mpegvideo alias musica_fundo";
    mciSendString(comandoOpen.c_str(), NULL, 0, NULL);

    // --- CODIGO NOVO PARA CONTROLAR O VOLUME ---
    // O volume vai de 0 a 1000. Comece testando com 200.
    mciSendString("setaudio musica_fundo volume to 100", NULL, 0, NULL);
    // -------------------------------------------

    mciSendString("play musica_fundo repeat", NULL, 0, NULL);
}

inline void pausarMusicaFundo() {
    mciSendString("pause musica_fundo", NULL, 0, NULL);
}

inline void retomarMusicaFundo() {
    mciSendString("resume musica_fundo", NULL, 0, NULL);
}

// =======================================================
// EFEITOS SONOROS (Pronto para o futuro)
// =======================================================
inline void tocarEfeito(const char* caminhoSom) {
    static int contadorCanal = 0;
    // Sistema de pool circular: recicla 10 canais (0 a 9) para evitar vazamento de memória/MCI
    contadorCanal = (contadorCanal + 1) % 10;

    std::stringstream ss;
    ss << "sfx_" << contadorCanal;
    std::string alias = ss.str();

    // Fecha o canal correspondente antes de sobrescrevê-lo
    std::string comandoClose = std::string("close ") + alias;
    mciSendString(comandoClose.c_str(), NULL, 0, NULL);

    std::string comandoOpen = std::string("open \"") + std::string(caminhoSom) + "\" type mpegvideo alias " + alias;
    mciSendString(comandoOpen.c_str(), NULL, 0, NULL);

    std::string comandoPlay = std::string("play ") + alias;
    mciSendString(comandoPlay.c_str(), NULL, 0, NULL);
}

#endif // AUDIO_H