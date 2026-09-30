// S3 HORNSEVEN — a bandstand call. Blow the horn on the beat.
// First to seven takes the stand. Leave when that is true.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace hornseven {

constexpr int kGoal = 7;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HORNSEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int you() const { return you_; }
    int rival() const { return rival_; }
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Play, Lead, Leave, Lost, Over, Pause };

    void toTitle();
    void beginPlay();
    void blow(bool sweet);
    void endBar();
    void beginLead();
    void beginLeave();
    void beginLost(const char* why);
    void tickAudio(float dt);
    void bot();
    void draw();
    void backdrop();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool feet = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void lamps(float x0, int n, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Play;
    const char* reason_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool barHit_ = false;
    int you_ = 0;
    int rival_ = 0;
    int phase_ = 0;
    int showT_ = 0;
    int titleT_ = 0;
    int step_ = 0;
    float walk_ = 70.f;
    float puff_ = 0.f;
    float toneT_ = 0.f;
    float toneF_ = 0.f;
    int noteN_ = 0;
};

}  // namespace hornseven
