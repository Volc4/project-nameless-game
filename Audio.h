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
// EFEITOS SONOROS
// Volume: 0 (mudo) a 1000 (máximo)
// =======================================================
inline void tocarEfeitoComVolume(const char* caminhoSom, int volume) {
    static int contadorCanal = 0;
    contadorCanal = (contadorCanal + 1) % 10;

    std::stringstream ss;
    ss << "sfx_" << contadorCanal;
    std::string alias = ss.str();

    mciSendString((std::string("close ") + alias).c_str(), NULL, 0, NULL);
    mciSendString((std::string("open \"") + caminhoSom + "\" type mpegvideo alias " + alias).c_str(), NULL, 0, NULL);
    mciSendString((std::string("setaudio ") + alias + " volume to " + std::to_string(volume)).c_str(), NULL, 0, NULL);
    mciSendString((std::string("play ") + alias).c_str(), NULL, 0, NULL);
}

inline void tocarEfeito(const char* caminhoSom) {
    tocarEfeitoComVolume(caminhoSom, 150);
}

// Sorteia e toca um dos três sons de morte de zumbi em volume reduzido
inline void tocarMorteZumbi() {
    int sorteio = rand() % 3;
    const char* sons[] = { "Sons/morte1.mp3", "Sons/morte2.mp3", "Sons/morte3.mp3" };
    tocarEfeitoComVolume(sons[sorteio], 50);
}

// Toca a música de derrota em volume reduzido (alias fixo para poder parar depois)
inline void tocarGameOver() {
    mciSendString("stop sfx_gameover", NULL, 0, NULL);
    mciSendString("close sfx_gameover", NULL, 0, NULL);
    mciSendString("open \"Sons/gameover.mp3\" type mpegvideo alias sfx_gameover", NULL, 0, NULL);
    mciSendString("setaudio sfx_gameover volume to 50", NULL, 0, NULL);
    mciSendString("play sfx_gameover", NULL, 0, NULL);
}

inline void pararGameOver() {
    mciSendString("stop sfx_gameover", NULL, 0, NULL);
    mciSendString("close sfx_gameover", NULL, 0, NULL);
}

#endif // AUDIO_H