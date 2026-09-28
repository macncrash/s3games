// S3 LOT DOOR — hold the lot gate for three minutes.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace lotdoor {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LOT DOOR"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int stopped() const { return stopped_; }
    int gate() const { return gate_; }
    // 0 title, 1 the hold, 2 the door stood, 3 the door fell
    int marker() const;

private:
    enum class Mode { Title, Hold, Won, Lost };

    struct Rig {
        float x, y, base, amp, freq, phase, vx;
        int hp, dmg, kind;
    };
    struct Flare {
        float x, y;
    };
    struct Spark {
        float x, y, t;
    };

    void beginHold();
    void update(float dt);
    void draw();
    void lot();
    void hud(int col, int row, const std::string& s, int pal);
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
    int gate_ = 100;
    int stopped_ = 0;
    int banner_ = 0;
    float t_ = 0;
    float py_ = 128;
    float fire_ = 0;
    float spawn_ = 0.5f;
    float beep_ = 0;
    float shake_ = 0;
    float crank_ = 0;
    uint32_t rng_ = 0x10D00Du;
    std::vector<Rig> rigs_;
    std::vector<Flare> flares_;
    std::vector<Spark> sparks_;
};

}  // namespace lotdoor
