// S3 WICKET SEVEN — one pitch, two sides. First to seven runs, then leave.
// A single ball is worth 1, 2, 4, or 6. Six is still short. The ground
// empties the moment either side reaches seven.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace wicketseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 WICKET SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int you() const { return you_; }
    int them() const { return them_; }
    int balls() const { return faced_; }
    const char* say() const { return say_; }

private:
    enum class Mode { Title, Play, Pause, Leave, Lose };
    enum class Phase { Run, Bowl, Result };
    enum class Call { None, One, Two, Four, Six, Soon, Late, Line, Misread, Caught, Beaten };

    struct Del {
        int line = 1;
        int length = 1;
        int shot = 0;
        bool yours = true;
    };

    Del book() const;
    void beginMatch();
    void beginBall();
    void tick();
    void steer(const Del& d);
    void swing(const Del& d);
    void enterResult(const Del& d, Call c);
    void stepResult();
    void advance();
    void finish(bool win);
    void stepEnd();
    void flightPos(const Del& d, int frame, float& x, float& y) const;
    void resultPos(const Del& d, float u, float& x, float& y) const;
    bool fits(int length, int shot) const;
    int shotRuns(int shot) const;
    Call shotCall(int shot) const;
    bool cpu(const Del& d) const { return bot_ || !d.yours; }
    void tone(int ch, float freq, float vol, int hold);
    void pumpAudio();
    bool swingPressed() const;
    bool startPressed() const;
    void draw();
    void backdrop();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip = false);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Phase phase_ = Phase::Run;
    Call call_ = Call::None;
    const char* say_ = "FIRST TO SEVEN";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool attempted_ = false;
    bool bounced_ = false;
    bool broke_ = false;
    int line_ = 1;
    int shot_ = 2;
    int ball_ = 0;
    int you_ = 0;
    int them_ = 0;
    int faced_ = 0;
    int runsThis_ = 0;
    int phaseFrame_ = 0;
    int resultT_ = 0;
    int anim_ = 1;
    int total_ = 1;
    int t_ = 0;
    int toneUntil_[3] = {};
    int fanStep_ = -1;
    int hold_ = 0;
    float ballX_ = 0.f;
    float ballY_ = 0.f;
};

}  // namespace wicketseven
