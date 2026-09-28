// S3 TOWER PACE — one tower. Wait until the third pace before you fire.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace tower {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TOWER PACE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int pace() const { return pace_; }
    int shotPace() const { return shotPace_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the tower, 2 the third pace, 3 fired and done, 4 the shot failed
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Over };
    enum class Phase { Intro, Hold, Stride };

    void resetPose();
    void beginWatch();
    void beginPace(int n);
    void updatePlay();
    bool steer();
    void resolveShot();
    void win();
    void lose(const char* why);
    void measure();
    float holdDur() const;
    float strideDur() const;
    void draw();
    void drawYard(float shx);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow = false);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    bool startPressed() const;
    bool firePressed();
    void blip(float freq, float vol, float hold);
    void serviceAudio();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Phase phase_ = Phase::Intro;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool shot_ = false;
    bool down_ = false;
    bool trigWas_ = false;
    int pace_ = 0;
    int shotPace_ = 0;
    const char* reason_ = "";
    float lat_ = 0.f;
    float fromLat_ = 0.f, toLat_ = 0.f;
    float phaseT_ = 0.f;
    float step_ = 0.f;
    float t_ = 0.f;
    float sightX_ = 160.f;
    float walkerX_ = 160.f;
    float walkerH_ = 36.f;
    float walkerFeet_ = 118.f;
    float walkerChest_ = 100.f;
    float flash_ = 0.f;
    float flashX_ = 160.f, flashY_ = 100.f;
    float shake_ = 0.f;
    float puff_ = 0.f;
    float blip_ = 0.f;
    int fanStep_ = -1;
    float fanT_ = 0.f;
    bool fanGood_ = false;
};

}  // namespace tower
