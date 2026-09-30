// S3 PAWNMARK — a short pawn. The gold square is the mark.
// Stepping onto it finishes the mark. That finished mark ends the board.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace pawnmark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PAWNMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finished() const { return finished_; }

private:
    enum class Mode { Title, Brief, Play, Done, Fail };

    struct Man {
        int f = 0;
        int r = 0;
        bool black = false;
        bool alive = true;
    };

    void begin();
    void play(float dt);
    void botAct();
    void humanAct();
    void stepBlacks();
    bool empty(int f, int r) const;
    bool walk(Man& m, int df, int dr);
    void finish();
    void blip(bool high);
    void draw();
    void text(const std::string& s, float x, float y, float scale, int pal, int align = 0);
    void hud(int col, int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, int fog = 0);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finished_ = false;
    float t_ = 0;
    float step_ = 0;
    float foe_ = 0;
    float clock_ = 14;
    float hold_ = 0;
    Man men_[4]{};
};

}  // namespace pawnmark
