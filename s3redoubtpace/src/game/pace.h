// S3 REDOUBT PACE — at the redoubt, wait until the third pace before you fire.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace redoubtpace {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 REDOUBT PACE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int pace() const { return pace_; }
    int shotPace() const { return shotPace_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the watch, 2 the third pace, 3 the redoubt holds, 4 the watch is over
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Over };
    enum class Phase { Hold, Stride };

    void beginWatch();
    void beginPace(int n);
    void updatePlay();
    bool wantFire();
    void resolveShot();
    void win();
    void lose(const char* why);
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet);
    void sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    bool startPressed() const;
    bool firePressed();
    void serviceAudio();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Phase phase_ = Phase::Hold;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool shot_ = false;
    bool downed_ = false;
    bool trigWas_ = false;
    bool dirRight_ = true;
    int pace_ = 0;
    int shotPace_ = 0;
    const char* reason_ = "";
    float phaseT_ = 0.f;
    float t_ = 0.f;
    float walkerX_ = 36.f;
    float fromX_ = 36.f;
    float toX_ = 248.f;
    float sightX_ = 150.f;
    float flash_ = 0.f;
    float blip_ = 0.f;
    int fanStep_ = -1;
    float fanT_ = 0.f;
    bool fanGood_ = false;
};

}  // namespace redoubtpace
