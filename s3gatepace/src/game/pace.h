// S3 GATE PACE — one gate. Wait until the third pace before you fire.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace pace {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 GATE PACE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int pace() const { return pace_; }
    int shotPace() const { return shotPace_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the watch, 2 the third pace, 3 fired and held, 4 the watch failed
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Over };
    enum class Phase { Intro, Hold, Stride };

    struct Proj {
        float x = 0, y = 0, ppm = 0;
        bool ok = false;
    };

    void resetPose();
    void beginWatch();
    void beginPace(int n);
    void updatePlay();
    bool steer();
    void resolveShot();
    void win();
    void lose(const char* why);
    void measure();
    Proj project(float lat, float z) const;
    int fogFor(float z) const;
    float holdDur() const;
    float strideDur() const;
    void draw();
    void drawRoad(float shx);
    void drawWorld(float shx);
    void drawGate(float shx);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow = false);
    void sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog);
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
    bool fell_ = false;
    bool trigWas_ = false;
    int pace_ = 0;
    int shotPace_ = 0;
    const char* reason_ = "";
    float z_ = 16.f;
    float lat_ = 0.f;
    float fromZ_ = 16.f, toZ_ = 16.f;
    float fromLat_ = 0.f, toLat_ = 0.f;
    float phaseT_ = 0.f;
    float step_ = 0.f;
    float t_ = 0.f;
    float sightX_ = 160.f;
    float walkerX_ = 160.f;
    float walkerH_ = 8.f;
    float walkerFeet_ = 120.f;
    float walkerChest_ = 100.f;
    int walkerFog_ = 0;
    float flash_ = 0.f;
    float flashX_ = 160.f, flashY_ = 100.f;
    float shake_ = 0.f;
    float puff_ = 0.f;
    float blip_ = 0.f;
    int fanStep_ = -1;
    float fanT_ = 0.f;
    bool fanGood_ = false;
};

}  // namespace pace
