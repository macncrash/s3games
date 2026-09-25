// S3 ARCH — thirty-six arrows. Gold counts double.
#pragma once

#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace arch {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 ARCH"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int golds() const { return golds_; }
    int shot() const { return shot_; }
    int line() const { return kLine; }
    // 0 title, 1 aiming, 2 in the air or called, 3 win, 4 short
    int marker() const;

private:
    enum class Phase { Title, Aim, Nock, Flight, Call, Win, Lose };

    struct Shot {
        float x = 0;
        float y = 0;
        Ring ring{};
    };

    void beginRound();
    void beginArrow();
    void enterNock();
    void startBotNock();
    void humanAim();
    void nockTick();
    void loose();
    void flightTick();
    void arrive();
    void callTick();
    void finish();
    void endTick();
    void steer();
    bool drawHeld() const;
    Shot predict() const;
    void backdrop();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void stamp(const gs::Image& img, float x, float y, float w, float h, int pal, bool flip = false, bool shadow = false);
    void at(const gs::Image& img, float cx, float cy, int pal);
    void queueTune(const float* hz, const int* frames, int n);
    void pumpAudio();
    const gs::Image& dotFor(const Ring& ring) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Phase phase_ = Phase::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool skipHold_ = false;
    int score_ = 0;
    int golds_ = 0;
    int shot_ = 0;
    int wind_ = 0;
    int drawTick_ = 0;
    int steady_ = 0;
    int flightT_ = 0;
    int callT_ = 0;
    int t_ = 0;
    int tuneLeft_ = 0;
    int blipLeft_ = 0;
    float aimX_ = kCx;
    float aimY_ = kCy;
    Shot pending_{};
    Shot last_{};
    std::vector<Shot> marks_;
    std::vector<float> tuneHz_;
    std::vector<int> tuneN_;
};

}  // namespace arch
