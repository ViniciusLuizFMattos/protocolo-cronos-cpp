// Protocolo Cronos++ - áudio 100% procedural (síntese em memória + mixer waveOut)
#pragma once
#include <map>
#include <string>
#include <vector>

class Audio {
public:
    Audio();
    ~Audio();
    // Sintetiza todos os sons. real=false: não toca nada (testes automáticos).
    void iniciar(bool real = true);
    // Chamado pela thread de áudio da plataforma: preenche n amostras (mono, 16 bits, 44100 Hz).
    void mixar(short* saida, int n);
    static constexpr int TAXA = 44100;
    void tocar(const std::string& nome, float volume = 0.55f);
    bool tem(const std::string& nome) const { return sons.count(nome) > 0; }
    bool alternar_mudo();
    void definir_volume(float v);
    bool mudo = false;
    float volume = 0.7f;       // volume geral (0 a 1)
    bool ativo = false;

private:
    struct Voz { const std::vector<short>* dados; size_t pos; float vol; bool loop; bool musica; };
    std::map<std::string, std::vector<short>> sons;
    std::vector<Voz> vozes;
    struct Impl;
    Impl* im = nullptr;
    static constexpr float VOL_MUSICA = 0.32f;
    void gerar();
    void iniciar_musica();
};
