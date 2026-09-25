// S3 DEPOT — clear the yard before the shift whistle.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace depot {

class Game : public gs::Cart {
public:
    static constexpr int kGoal = 6;

    const char* title() const override { return "S3 DEPOT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int cleared() const { return cleared_; }
    int goal() const { return kGoal; }
    int shiftSeconds() const;
    const char* why() const { return why_; }
    float tugX() const { return x_; }
    float tugY() const { return y_; }

private:
    struct Load {
        float x = 0, y = 0;
        int kind = 0;
        bool live = true;
    };
    struct Bay {
        float x = 0, y = 0;
        int col = 0;
        int kind = 0;
        bool full = false;
    };
    struct Puff {
        float x = 0, y = 0, vx = 0, vy = 0, t = 0;
    };

    enum class Mode { Title, Play, Pause, Win, Lose };

    void tune();
    void layWorld();
    void park();
    void paintBay(int i);
    void paintYard();
    void begin();
    void retarget();
    bool focusOk() const;
    float near(float x, float y) const;
    void hitch(int i);
    void deliver(int i);
    void hook();
    void botDrive(float& steer, float& gas);
    void drive(float& steer, float& gas);
    void move(float steer, float gas);
    void burst(float x, float y);
    void puff();
    void win();
    void lose();
    void chime(int kind);
    void sky();
    void draw();
    void audio();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    const gs::Mipped& freight(int kind) const;
    int freightPal(int kind) const;
    float freightH(int kind) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Load load_[kGoal];
    Bay bay_[kGoal];
    Puff puff_[10];
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool horn_ = false;
    int cleared_ = 0;
    int carry_ = -1;
    int focus_ = -1;
    int clock_ = 0;
    int stall_ = 0;
    int song_ = -1;
    int songKind_ = 0;
    int pop_ = 0;
    int wrong_ = -1;
    float x_ = 0, y_ = 0, heading_ = 0, speed_ = 0;
    float anchorX_ = 0, anchorY_ = 0;
    float popX_ = 0, popY_ = 0;
    const char* why_ = "shift";
};

}  // namespace depot
