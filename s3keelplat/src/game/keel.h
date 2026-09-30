// S3 KEELPLAT — take the keel and stop level with the platform.
// The clock is the other crew. Short or past fails the leg.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace keelplat {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 KEELPLAT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int berth() const { return berth_; }
    float seconds() const { return time_; }
    const char* why() const { return why_; }

private:
    enum class Mode { Title, Play, Dock, Pause, Win, Fail };

    void begin();
    void placeBerth();
    void step(float dt);
    void pilot(float& thrust, float& helm) const;
    void fail(const char* why);
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    bool levelled() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int berth_ = 0;
    float x_ = 40, y_ = 120, vx_ = 0, vy_ = 0;
    float platX_ = 200, deck_ = 100;
    float clock_ = 56;
    float time_ = 0;
    float hold_ = 0;
    float t_ = 0;
    float dockT_ = 0;
    float tone_ = 0;
    float wind_ = 0;
    const char* why_ = "";
    static constexpr int kBerths = 3;
    static constexpr float kPlatW = 108.f;
    static constexpr float kHalf = 28.f;
};

}  // namespace keelplat
