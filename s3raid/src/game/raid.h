// S3 RAID — ride the road into the yard, then clear the stacks.
#pragma once
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace raid {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 RAID"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int cleared() const { return cleared_; }
    int hull() const { return hull_; }
    float seconds() const { return playTime_; }
    int phase() const { return int(mode_); }
    float travel() const { return s_; }
    float lateral() const { return x_; }
    float bikeX() const { return px_; }
    float bikeY() const { return py_; }
    int markMask() const;

private:
    enum class Mode { Title, Ride, Arrive, Yard, Pause, Win, Dead };

    struct Block {
        float s, x;
        bool live;
    };
    struct Mark {
        float x, y;
        int kind;
        bool live;
    };
    struct Bullet {
        float x, y, vx, vy, life;
    };
    struct Puff {
        float a, b, life;
        int space;  // 0 yard x/y, 1 road distance/lateral
    };
    struct Wspr {
        float depth, cx, cy, h;
        const gs::Mipped* img;
        int pal, fog, clip;
        bool flip, shadow;
    };

    void showTitle();
    void beginRide();
    void beginYard();
    void victory();
    void killRun();
    bool hurt();
    void clearMark(int i);
    void controls(float& steer, float& drive, bool& brake, bool& fire);
    void pilotRide(float& steer, float& drive, bool& brake);
    void pilotYard(float& steer, float& drive, bool& brake, bool& fire);
    int pickMark() const;
    void updateRide(float dt, float steer, float drive, bool brake, bool fire);
    void updateYard(float dt, float steer, float drive, bool brake, bool fire);
    void shoot();
    void audio();
    void draw();
    void drawRoad(float s, float x, float steer, float speed, bool title);
    void drawYard();
    void drawHud();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    bool project(float wx, float ws, float& sx, float& sy, float& scale, float& depth) const;
    void addW(const gs::Mipped& m, float depth, float cx, float bottom, float h, int pal, int fog, bool flip, bool shadow);
    void blit(const gs::Mipped& m, float cx, float cy, float ph, int pal, bool flip, int fog, int clip, bool feet, bool shadow);
    void spawnPuff(float a, float b, int space);
    int topFrame() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Ride;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int score_ = 0;
    int cleared_ = 0;
    int hull_ = 4;
    int fanStep_ = -1;
    int puffWrite_ = 0;
    float t_ = 0;
    float playTime_ = 0;
    float demo_ = 20;
    float arriveT_ = 0;
    float winT_ = 0;
    float hurtT_ = 0;
    float shake_ = 0;
    float fireCd_ = 0;
    float truckCd_ = 0;
    float stuckT_ = 0;
    float stuckX_ = 0;
    float stuckY_ = 0;
    float s_ = 0;
    float x_ = 0;
    float speed_ = 0;
    float px_ = 0;
    float py_ = 0;
    float heading_ = 0;
    float truckX_ = 0;
    float truckDir_ = 1;
    float camS_ = 0;
    float camX_ = 0;
    float shownSteer_ = 0;
    int stage_ = 0;
    Block blocks_[5]{};
    Mark marks_[6]{};
    Bullet bullets_[8]{};
    Puff puffs_[12]{};
    std::vector<Wspr> world_;
};

}  // namespace raid
