// S3 CISTERN PURSE — be the last machine still running, or the watch is over.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace purse {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CISTERN PURSE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int stalled() const { return stalled_; }
    int fleet() const { return fleet_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the rim, 2 one other machine left, 3 the watch has ended
    int marker() const;

private:
    enum class Mode { Title, Watch, Pause, Victory, Over };
    enum Kind { LIGHT = 0, HEAVY = 1, SKIT = 2 };

    struct Spawn {
        float t = 0;
        int kind = 0;
        float ang = 0;
    };
    struct Mach {
        int kind = 0;
        int hp = 1;
        int points = 0;
        float ang = 0;
        float speed = 0.4f;
        float cool = 0;
        float flash = 0;
        bool alive = true;
    };

    void beginWatch();
    void update(float dt);
    void strike();
    void stall(Mach& m);
    void winWatch();
    void loseWatch(const char* why);
    void place(float ang, float& x, float& y) const;
    int nearest() const;
    int others() const { return fleet_ > stalled_ ? fleet_ - stalled_ : 0; }
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog = 0);
    void text(const char* s, float x, float y, float scale, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* reason_ = "THE WATCH IS OVER";
    int score_ = 0;
    int stalled_ = 0;
    int fleet_ = 0;
    int grit_ = 3;
    int spawnAt_ = 0;
    int fanStep_ = -1;
    int flash_ = 0;
    float you_ = 0;
    float watch_ = 0;
    float modeT_ = 0;
    float strikeCd_ = 0;
    float shake_ = 0;
    float fanT_ = 0;
    float shx_ = 0, shy_ = 0;
    std::vector<Spawn> script_;
    std::vector<Mach> machs_;
};

}  // namespace purse
