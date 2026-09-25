// S3 YARD — last machine still running takes the purse.
#pragma once

#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace yard {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 YARD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int live() const;
    int youHp() const;
    float px() const;
    float py() const;
    const char* why() const { return why_; }
    // 0 title, 1 the yard, 2 the purse, 3 engine dead
    int marker() const;

private:
    enum class Mode { Title, Bout, Pause, Won, Lost };

    struct Rig {
        float x = 0, y = 0, hdg = 0, spd = 0;
        float hp = 1, maxHp = 1;
        float steer = 0, throttle = 0;
        float flash = 0, stuck = 0;
        float cool[4] = {};
        bool alive = true;
        bool player = false;
        bool boost = false;
        int pal = PAL_YOU;
    };
    struct Bit {
        float x = 0, y = 0, t = 0, life = 0.5f, h = 12;
        bool spark = false;
    };

    void place();
    void begin();
    void readInput(gs::System& sys);
    void driveHuman(const gs::Pad& pad);
    void botDrive();
    void rivalDrive(Rig& r, int index);
    void update(float dt);
    void celebrate(float dt);
    void reap();
    void physics(float dt);
    void collide(int i, int j, bool hurt);
    void shovePile(Rig& r);
    void clampWall(Rig& r);
    void tickBits(float dt);
    void puff(float x, float y, bool spark);
    void burst(float x, float y);
    void audio();
    void fanfare();
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void bar(int col, int row, float hp, float maxHp);
    int rivalsAlive() const;
    int frameOf(float hdg) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    std::vector<Rig> rigs_;
    std::vector<Bit> bits_;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool engineOn_ = false;
    const char* why_ = "ready";
    float t_ = 0;
    float banner_ = 0;
    float purseX_ = 36, purseY_ = 24, purseLift_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    int fanStep_ = -1;
    float fanT_ = 0;
};

}  // namespace yard
