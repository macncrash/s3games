// S3 PUTTBELL — one green, three tries.
// The bell has to ring before the third try dies. Then you leave.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace puttbell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PUTTBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    bool rolling() const { return mode_ == Mode::Roll && rollT_ > 0.12f; }
    int deadTries() const { return dead_; }
    int tryNo() const { return tryNo_; }

private:
    enum class Mode { Title, Aim, Roll, Dead, Ring, Leave, Pause, Over };

    struct Lie {
        float x = 0, y = 0, vx = 0, vy = 0;
        bool rest = true;
    };
    enum class Halt { Fly, Rest, Bell };

    static Halt stepLie(Lie& b, float dt);
    void toTitle();
    void newGame();
    void beginAim();
    void putt(float ang, float spd);
    void ring();
    void dieTry();
    bool reaches(float ang, float spd) const;
    bool solve(float& ang, float& spd) const;
    void blip(float a, float b, float hold);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool shadow = false);
    void patch(const gs::Mipped& m, float x, float y, float w, float h, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    Lie ball_{};
    float aim_ = -1.2f;
    float meter_ = 0;
    float meterDir_ = 1;
    float clock_ = 0;
    float aimT_ = 0;
    float rollT_ = 0;
    float deadT_ = 0;
    float ringT_ = 0;
    float leaveT_ = 0;
    float bellPh_ = 0;
    float bellAmp_ = 0.2f;
    float toneHold_ = 0;
    float bellTick_ = 0;
    float botSpd_ = 190;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    bool swinging_ = false;
    bool botReady_ = false;
    int dead_ = 0;
    int tryNo_ = 0;
};

}  // namespace puttbell
