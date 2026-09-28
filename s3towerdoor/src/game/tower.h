// One stone tower. Hold the door for three minutes.
#pragma once
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace tower {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TOWER DOOR"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int secondsLeft() const;
    int barPct() const;

private:
    enum class Mode { Title, Play, Won, Lost };

    struct Climb {
        int at;
        int lane;
    };

    void begin();
    void act();
    void playTick();
    void draw();
    void sky();
    void hudText(int col, int row, const char* s, int pal);
    void spr(const gs::Image& img, int x, int y, int w, int h, int pal, bool flip = false);
    void tone(int ch, float freq, float vol);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int age_ = 0;
    int play_ = 0;
    int lane_ = 1;
    int lean_ = 0;
    bool bracing_ = false;
    float bar_ = 1.f;
    int flash_ = 0;
    int shake_ = 0;
    int climbIx_ = 0;
    int toneT_ = 0;
    std::vector<Climb> climbs_;
};

}  // namespace tower
