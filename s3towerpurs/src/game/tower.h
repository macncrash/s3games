// S3 TOWER PURSE — be the last machine still running, or the watch is over.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace tpurs {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TOWER PURSE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int stalled() const { return stalled_; }
    int fleet() const { return fleet_; }
    // 0 title, 1 the tower, 2 one other machine left, 3 the watch has ended
    int marker() const;

private:
    enum class Mode { Title, Watch, Pause, Victory, Over };

    struct Mach {
        int kind = 0;
        int hp = 2;
        int points = 0;
        float ang = 0;
        float speed = 0.6f;
        float flash = 0;
        bool live = true;
        bool out = false;
    };

    void beginWatch();
    void update(float dt);
    void steerBot();
    void tryRam();
    void stall(Mach& m);
    void winWatch();
    void loseWatch(const char* why);
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog = 0, bool feet = false);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void blip(float freq, float vol);
    int others() const { return fleet_ > stalled_ ? fleet_ - stalled_ : 0; }
    const gs::Mipped& bodyOf(int kind) const;
    int palOf(int kind) const;
    const char* whoOf(int kind) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool engineOn_ = false;
    const char* reason_ = "THE WATCH IS OVER";
    int score_ = 0;
    int stalled_ = 0;
    int fleet_ = 5;
    int grit_ = 3;
    int next_ = 0;
    int fanStep_ = -1;
    float bearing_ = 0;
    float steer_ = 0;
    float ram_ = 0;
    float ramCd_ = 0;
    float gap_ = 0.4f;
    float watch_ = 0;
    float t_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    float fanT_ = 0;
    std::vector<Mach> machs_;
};

}  // namespace tpurs
