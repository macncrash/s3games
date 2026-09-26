// S3 WICKET GOLD — a short pitch. The line is 6.
// A gold ball counts two. A cream ball counts one.
// Cream that would reach the line does not count.
// Only gold can finish, and the undoubled hits stay under the line.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace wicketgold {

constexpr int kLine = 6;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 WICKET GOLD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finisherGold() const { return finisherGold_; }
    int gold() const { return gold_; }
    int cream() const { return cream_; }
    int score() const { return score_; }
    int balls() const { return faced_; }
    int line() const { return kLine; }
    int bare() const { return gold_ + cream_; }
    const char* say() const { return say_; }

private:
    enum class Mode { Title, Play, Pause, Win, Lose };
    enum class Phase { Run, Bowl, Result };
    enum class Call { None, Gold, Cream, Refuse, Bowled, Soon, Late, Sky, Block, Beaten };

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
    bool paid() const;
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
    const char* say_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finisherGold_ = false;
    bool attempted_ = false;
    bool loft_ = false;
    bool bounced_ = false;
    int linePick_ = 1;
    int ball_ = 0;
    int gold_ = 0;
    int cream_ = 0;
    int score_ = 0;
    int refused_ = 0;
    int faced_ = 0;
    int phaseFrame_ = 0;
    int resultT_ = 0;
    int t_ = 0;
    int toneUntil_[3] = {};
    int fanStep_ = -1;
    int hold_ = 0;
    float ballX_ = 0;
    float ballY_ = 0;
    char card_[6] = {'.', '.', '.', '.', '.', '.'};
};

}  // namespace wicketgold
