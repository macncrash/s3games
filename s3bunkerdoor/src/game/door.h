// S3 BUNKER DOOR — hold the blast door for three minutes.
// A heave you miss opens the seal. The watch ends if the door gives.
#pragma once
#include <vector>

#include "art.h"
#include "console/system.h"

namespace door {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BUNKER DOOR"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int sealPct() const { return sealPct_; }
    int secondsLeft() const { return secondsLeft_; }

private:
    enum class Mode { Title, Play, Won, Lost };

    struct Heave {
        int at = 0;
        int dir = 1;
    };

    void bootPictures();
    void begin();
    void buildHeaves();
    void act();
    void playTick();
    void endTick();
    void draw();
    void sprite(const gs::Mipped& m, float x, float y, float h, int pal, bool flip);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void tone(int ch, float freq, float vol);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool bracing_ = false;
    int age_ = 0;
    int play_ = 0;
    int lean_ = 0;
    int heaveIx_ = 0;
    int flash_ = 0;
    int shake_ = 0;
    int toneT_ = 0;
    int sealPct_ = 0;
    int secondsLeft_ = 180;
    float seal_ = 0;
    std::vector<Heave> heaves_;
};

}  // namespace door
