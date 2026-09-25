// S3 BLITZ — a trench run. The magazine is the only clock.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace blitz {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BLITZ"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int rounds() const;
    int towers() const { return towers_; }
    int cells() const { return cells_; }
    int score() const;
    float traveled() const { return dist_; }
    const char* note() const { return note_; }

private:
    enum class Mode { Title, Run, Pause, Win, Dead };
    enum class Kind { Turret, SparLow, SparHigh, Block, Cell, Port };

    struct Mark {
        float z = 0, x = 0, w = 0;
        Kind kind = Kind::Turret;
        int hp = 1;
        bool live = true;
        bool gate = false;
    };
    struct Bolt {
        float local = 0, y = 0, z = 0;
    };
    struct Boom {
        float local = 0, y = 0, z = 0, t = 0;
    };
    struct Stick {
        float steer = 0, climb = 0;
        bool fire = false;
    };
    struct Proj {
        float x = 0, y = 0, s = 0;
        int fog = 0;
        bool ok = false;
    };

    void loadMarks();
    void newRun();
    void update();
    void draw();
    void audio();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    Stick stick() const;
    Stick botStick() const;
    float noseY() const;
    float worldX(float z, float local) const;
    Proj project(float wx, float wy, float wz) const;
    void quad(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog, bool flip = false,
              bool shadow = false);
    void killTurret(Mark& m);
    void boomAt(float local, float y, float z);
    void fail(const char* why);
    void win();
    const char* hint() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    float t_ = 0;
    float dist_ = 0;
    float localX_ = 0;
    float vx_ = 0;
    float alt_ = 0.5f;
    float ammo_ = 0;
    float cool_ = 0;
    float shake_ = 0;
    float flash_ = 0;
    float shotSnd_ = 0;
    float tickSnd_ = 0;
    int towers_ = 0;
    int cells_ = 0;
    int shots_ = 0;
    int lastShown_ = -1;
    char note_[16] = {};
    std::vector<Mark> marks_;
    std::vector<Bolt> bolts_;
    std::vector<Boom> booms_;
};

}  // namespace blitz
