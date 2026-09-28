// S3 LOT PURSE — one lot. Be the last machine still running. Then it is done.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace lotp {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LOT PURSE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int stalled() const { return stalled_; }
    int fleet() const { return fleet_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the lot, 2 one other machine left, 3 the lot is taken
    int marker() const;

private:
    enum class Mode { Title, Drive, Pause, Victory, Over };

    struct Mach {
        int kind = 0;
        int hp = 1;
        int wp = 0;
        int points = 0;
        float x = 0, y = 0;
        float ang = 0;
        float spd = 0;
        float cd = 0;
        bool you = false;
        bool live = true;
    };
    struct Spark {
        float x = 0, y = 0, t = 0;
    };

    void beginLot();
    void update(float dt);
    void collide(Mach& a, Mach& b);
    void stall(Mach& m);
    void winLot();
    void loseLot();
    void draw();
    void audio();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    const gs::Mipped& bodyOf(const Mach& m) const;
    int palOf(const Mach& m) const;
    int others() const { return fleet_ > stalled_ ? fleet_ - stalled_ : 0; }

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* reason_ = "YOU STALLED";
    int score_ = 0;
    int stalled_ = 0;
    int fleet_ = 0;
    int hull_ = 0;
    float t_ = 0;
    float modeT_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    bool ramming_ = false;
    float stuckT_ = 0;
    float prevX_ = 0, prevY_ = 0;
    std::vector<Mach> machs_;
    std::vector<Spark> sparks_;
};

}  // namespace lotp
