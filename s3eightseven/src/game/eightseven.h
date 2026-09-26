// S3 EIGHT SEVEN — one table, two groups, the 8 stays up.
// Solids are yours, stripes are theirs. A ball counts only in the called pocket.
// First tally to reach 7 wins, and the match ends. A 6 is still short.
// Pocketing the 8 does not count and does not finish it.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace eightseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 EIGHT SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool eightUp() const { return !ball_[8].down; }
    bool rolling() const { return mode_ == Mode::Roll; }
    int you() const { return you_; }
    int them() const { return them_; }
    int shots() const { return shots_; }

private:
    enum class Mode { Title, Aim, Roll, Hold, Pause, Leave, Lose };

    struct Ball {
        float x = 0, y = 0, vx = 0, vy = 0;
        float homeX = 0, homeY = 0;
        bool down = false;
        bool fell = false;
        int pocket = -1;
    };

    void rack();
    void begin();
    void setupShot();
    void shoot();
    void physics(float dt);
    void resolve();
    void finish();
    void fail(const char* why);
    void park(int n);
    void sink(Ball& b, int pocket);
    bool audit() const;
    bool anyMoving(float v) const;
    bool inTable(float x, float y) const;
    bool overlaps(float x, float y, int ignore) const;
    bool freePoint(float x, float y, int ignore) const;
    void forceStop();
    int sunk(int lo, int hi) const;

    void blip(float freq, float vol);
    void silence();
    void fanfare();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void pips(char* out, int n) const;
    void backdrop();
    void aimAid();
    void cueDraw();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    Ball ball_[16]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool yours_ = true;
    bool guided_ = false;
    bool upShot_ = true;
    bool retry_ = false;
    bool pocketSnd_ = false;
    bool clack_ = false;
    int you_ = 0;
    int them_ = 0;
    int shots_ = 0;
    int turn_ = 0;
    int retries_ = 0;
    int live_ = 1;
    int called_ = 1;
    int solvedPocket_ = 1;
    int fanStep_ = -1;
    float aim_ = -1.5707963f;
    float solvedAim_ = -1.5707963f;
    float power_ = 0.56f;
    float t_ = 0;
    float rollT_ = 0;
    float holdT_ = 0;
    float think_ = 0;
    float toneT_ = 0;
    float fanT_ = 0;
    const char* say_ = "FIRST TO SEVEN";
};

}  // namespace eightseven
