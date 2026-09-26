// S3 SLED TURN — three hooks on a snow trail. Roll the basket too far and it tips.
#pragma once
#include <vector>

#include "art.h"
#include "console/system.h"

namespace sledturn {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SLED TURN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* why() const { return why_ ? why_ : ""; }
    float seconds() const { return race_; }
    float x() const { return x_; }
    float z() const { return z_; }
    float heading() const { return heading_; }
    float speed() const { return speed_; }
    float lean() const { return lean_; }
    float lateral() const;
    int turns() const { return turns_; }
    // 0 title, 1 the straight, 2 in a turn, 3 two turns made, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Win, Fail };
    enum class Kind { StakeR, StakeL, Tree, Cabin, Cache, Sign, Post };

    struct Prop {
        float x, z, h;
        Kind kind;
        int pal;
        int num;
    };
    struct Puff {
        float x, z, life;
    };
    struct Raven {
        float x, z, y, ph;
    };
    struct Flake {
        float x, y, s, v;
    };

    void begin();
    void showTitle();
    void startRun();
    void buildCourse();
    void seedFlakes();
    void controls(float& steer, float& throttle, bool& set);
    void pilot(float& steer, float& throttle, bool& set);
    void physics(float steer, float throttle, bool set);
    void markTurns(float zPrev);
    void win();
    void fail(const char* why);
    void blip(float freq);
    void chime();
    void audio();
    void draw();
    void drawTrail();
    void drawWorld();
    void drawSky();
    void drawHud();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, int fog = 0, bool shadow = false);
    bool project(float wx, float wy, float wz, float& sx, float& sy, float& scale, int& fog, float& rz) const;
    int leanFrame() const;
    float eye() const;
    float back() const;
    float horizon() const;
    const char* tipWhy() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool made_[3] = {};
    bool warned_ = false;
    bool setting_ = false;
    const char* why_ = "";
    int turns_ = 0;
    int chimeN_ = 0, chimeStep_ = 0;
    int puffN_ = 0;
    float t_ = 0, race_ = 0;
    float x_ = 0, z_ = 0, heading_ = 0, speed_ = 0, throttle_ = 0;
    float yaw_ = 0, lean_ = 0, leanVel_ = 0;
    float camBob_ = 0, shake_ = 0;
    float tone0_ = 0, tone1_ = 0, chimeT_ = 0, puffT_ = 0;
    Puff puffs_[10]{};
    Raven ravens_[3]{};
    Flake flakes_[22]{};
    std::vector<Prop> props_;
};

}  // namespace sledturn
