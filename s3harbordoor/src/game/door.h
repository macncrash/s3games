// S3 HARBOR DOOR — hold the harbor gate for three minutes.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace harbordoor {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HARBOR DOOR"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int sunk() const { return sunk_; }
    int hull() const { return hull_; }
    // 0 title, 1 the hold, 2 the door stood, 3 the door fell
    int marker() const;

private:
    enum class Mode { Title, Hold, Won, Lost };

    struct Craft {
        float x, y, base, amp, freq, phase, vx;
        int hp, dmg, kind;
    };
    struct Bolt {
        float x, y;
    };
    struct Puff {
        float x, y, t;
    };

    void beginHold();
    void update(float dt);
    void draw();
    void harbor();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void word(const std::string& s, float cx, float y, float h, int pal);
    float rnd();
    void blip(float freq);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int hull_ = 100;
    int sunk_ = 0;
    int banner_ = 0;
    float t_ = 0;
    float py_ = 120;
    float fire_ = 0;
    float spawn_ = 0.4f;
    float beep_ = 0;
    float shake_ = 0;
    uint32_t rng_ = 0xD00Du;
    std::vector<Craft> craft_;
    std::vector<Bolt> bolts_;
    std::vector<Puff> puffs_;
};

}  // namespace harbordoor
