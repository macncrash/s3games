// S3 TILE SEVEN — a short tile. First course to seven pips wins.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace tileseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TILE SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int you() const { return you_; }
    int them() const { return them_; }

private:
    enum class Mode { Title, Play, Win, Lose };

    void open();
    void deal();
    void claim(bool yours);
    void finishIf();
    void botAct();
    void human();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void pips(float cx, float cy, int n, float h, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool yours_ = true;
    int tile_ = 1;
    int you_ = 0;
    int them_ = 0;
    int deck_ = 0;
    int life_ = 0;
    int t_ = 0;
    int act_ = 0;
    int beep_ = 0;
    int flash_ = 0;
};

}  // namespace tileseven
