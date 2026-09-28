// S3 PARADE GOLD — a short parade. Only the gold counts double.
// Cream scores its face. A cream token that would reach the line does not count.
// Leave when a gold double is what puts the score over the line, and the
// undoubled faces are still short of it.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace paradegold {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PARADE GOLD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int bare() const { return bare_; }
    int golds() const { return golds_; }
    int cream() const { return cream_; }
    int lives() const { return lives_; }
    const char* phase() const;

private:
    enum class Mode { Title, March, Pause, Win, Dead };

    struct Lane {
        float y;
        float speed;
        float spacing;
        float width;
        float phase;
    };
    struct Token {
        float x, y, vx;
        bool gold;
        bool taken;
    };

    void toTitle();
    void begin();
    void logic();
    void botStep();
    void humanStep();
    bool danger(float x, float y, int fr) const;
    bool crossSafe(float x, float y0, float y1) const;
    void takeToken(Token& t);
    void hurt();
    void finish();
    void audio();
    void draw();
    void backdrop();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool shadow = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::March;
    Lane lanes_[4] = {};
    Token tok_[8] = {};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool goldOut_ = false;
    int lives_ = 3;
    int score_ = 0;
    int bare_ = 0;
    int golds_ = 0;
    int cream_ = 0;
    int frame_ = 0;
    int age_ = 0;
    int inv_ = 0;
    int flash_ = 0;
    int fan_ = -1;
    int lastBeat_ = -1;
    float px_ = 160.f;
    float py_ = 208.f;
    float face_ = 1.f;
    float commitX_ = 0.f;
    float commitY_ = 0.f;
    float slide_ = 1.f;
    bool committed_ = false;
};

}  // namespace paradegold
