#ifndef AUDIO_H
#define AUDIO_H

#include <windows.h>
#include <mmsystem.h>
#include <string>
#include <sstream>

// Troca a faixa de fundo em loop; para a anterior primeiro.
// Volume MCI: 0 (mudo) a 1000 (máximo) — fixado em 100 para não cobrir os efeitos.
inline void tocarMusicaFundo(const char* caminhoMp3) {
    mciSendString("stop musica_fundo", NULL, 0, NULL);
    mciSendString("close musica_fundo", NULL, 0, NULL);

    std::string comandoOpen = std::string("open \"") + caminhoMp3 + "\" type mpegvideo alias musica_fundo";
    mciSendString(comandoOpen.c_str(), NULL, 0, NULL);
    mciSendString("setaudio musica_fundo volume to 100", NULL, 0, NULL);
    mciSendString("play musica_fundo repeat", NULL, 0, NULL);
}

inline void pausarMusicaFundo() {
    mciSendString("pause musica_fundo", NULL, 0, NULL);
}

inline void retomarMusicaFundo() {
    mciSendString("resume musica_fundo", NULL, 0, NULL);
}

// Toca um efeito sonoro em pool rotativo de 10 aliases (sfx_0..sfx_9).
// Cada chamada avança o slot — sons antigos são sobrescritos mas nunca cortados abruptamente.
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

// Toca um efeito em volume padrão (150).
inline void tocarEfeito(const char* caminhoSom) {
    tocarEfeitoComVolume(caminhoSom, 150);
}

// Toca uma fala da protagonista no canal exclusivo sfx_fala.
// Canal separado do pool sfx_0..9 para que sons de zumbis não interrompam a voz.
// Para a fala anterior antes de começar.
inline void tocarFalaPersonagem(const char* caminhoSom, int volume) {
    mciSendString("stop sfx_fala",  NULL, 0, NULL);
    mciSendString("close sfx_fala", NULL, 0, NULL);
    std::string open = std::string("open \"") + caminhoSom +
                       "\" type mpegvideo alias sfx_fala";
    mciSendString(open.c_str(), NULL, 0, NULL);
    std::string vol  = std::string("setaudio sfx_fala volume to ") +
                       std::to_string(volume);
    mciSendString(vol.c_str(), NULL, 0, NULL);
    mciSendString("play sfx_fala", NULL, 0, NULL);
}

// Retorna true enquanto sfx_fala ainda estiver reproduzindo — usado para suprimir sons de zumbi.
inline bool personagemEstaFalando() {
    char modo[32] = "";
    mciSendString("status sfx_fala mode", modo, sizeof(modo), NULL);
    return (std::string(modo) == "playing");
}

// Sorteia um dos três sons de morte de zumbi e toca em volume reduzido.
inline void tocarMorteZumbi() {
    int sorteio = rand() % 3;
    const char* sons[] = { "Sons/morte1.mp3", "Sons/morte2.mp3", "Sons/morte3.mp3" };
    tocarEfeitoComVolume(sons[sorteio], 50);
}

// Toca a música de derrota em alias fixo (sfx_gameover) para que pararGameOver() possa interrompê-la.
inline void tocarGameOver() {
    mciSendString("stop sfx_gameover", NULL, 0, NULL);
    mciSendString("close sfx_gameover", NULL, 0, NULL);
    mciSendString("open \"Sons/gameover.mp3\" type mpegvideo alias sfx_gameover", NULL, 0, NULL);
    mciSendString("setaudio sfx_gameover volume to 50", NULL, 0, NULL);
    mciSendString("play sfx_gameover", NULL, 0, NULL);
}

// Para e fecha sfx_gameover — chamada ao reiniciar para não sobrepor à música do jogo.
inline void pararGameOver() {
    mciSendString("stop sfx_gameover", NULL, 0, NULL);
    mciSendString("close sfx_gameover", NULL, 0, NULL);
}

#endif // AUDIO_H