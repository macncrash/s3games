// S3 WICKET — six balls. Drive the full one into the stumps, or loft the short one over the rope.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace wicket {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 WICKET"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int wickets() const { return wickets_; }
    int sixes() const { return sixes_; }

private:
    enum class Mode { Title, Play, Pause, Win, Lose };
    enum class Phase { Run, Bowl, Result };
    enum class Call { None, Wicket, Six, Bowled, Soon, Late, Sky, Block };

    void beginOver();
    void beginBall();
    void tick();
    void steer();
    void swing();
    void enterResult(Call c);
    void stepResult();
    void advance();
    void finish(bool win);
    void stepEnd();
    static int callKind(Call c);
    void flightPos(int frame, float& x, float& y) const;
    void resultPos(float u, float& x, float& y) const;
    float shotU() const;
    void draw();
    void backdrop();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip = false);
    void tone(int ch, float freq, float vol, int hold);
    void pumpAudio();
    bool swingPressed() const;
    bool startPressed() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Phase phase_ = Phase::Run;
    Call call_ = Call::None;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool attempted_ = false;
    bool loft_ = false;
    int line_ = 1;
    int ball_ = 0;
    int wickets_ = 0;
    int sixes_ = 0;
    int phaseFrame_ = 0;
    int resultT_ = 0;
    int t_ = 0;
    int toneUntil_[3] = {};
    int sixNote_ = -1;
    int fanStep_ = -1;
    int hold_ = 0;
    float ballX_ = 0;
    float ballY_ = 0;
    char mark_[6] = {'.', '.', '.', '.', '.', '.'};
};

}  // namespace wicket
