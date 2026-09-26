// S3 RIDGE DOOR — at the ridge, hold the door for three minutes.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace rdoor {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 RIDGE DOOR"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float watch() const { return watch_; }
    float give() const { return give_; }
    int blocks() const { return blocks_; }
    int misses() const { return misses_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the door, 2 a shove answered, 3 the late watch, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Won, Lost };

    struct Wave {
        int frame;
        int lane;
    };
    struct Gust {
        int frame;
        int dur;
    };
    struct Foe {
        float z = 0;
        float speed = 1;
        int lane = 0;
        bool on = false;
    };
    struct Puff {
        float x = 0, y = 0, vx = 0, vy = 0, t = 0;
    };
    struct Rock {
        float z;
        float lane;
        float h;
    };

    void bootTitle();
    void begin();
    void schedule();
    void update();
    void botPlan(int& post, bool& hold, bool& wedge);
    void plant(int lane);
    void spawn(int lane, float speed);
    void strike(Foe& f);
    void win();
    void lose();
    void blip(int ch, float freq, float hold);
    void puffAt(float x, float y, int n);
    void serviceAudio();
    void draw();
    void layRoad();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0,
             bool shadow = false);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip = false, int fog = 0,
               bool shadow = false);
    float bend(float row) const;
    int horizon() const;
    bool alignedTo(int lane) const;
    int soonest(int skipLane) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool holding_ = false;
    bool stamLock_ = false;
    bool wedging_ = false;
    const char* reason_ = "THE WATCH IS OVER";
    int age_ = 0;
    int post_ = 0;
    int heldDir_ = 0;
    int wedgeOn_ = -1;
    int wedgeLane_ = 0;
    int blocks_ = 0;
    int misses_ = 0;
    int fanStep_ = -1;
    int waveCount_ = 0;
    int waveNext_ = 0;
    int gustCount_ = 0;
    int gustNext_ = 0;
    int gustLeft_ = 0;
    float watch_ = 0;
    float give_ = 0;
    float u_ = 0;
    float stam_ = 1;
    float shake_ = 0;
    float dirCool_ = 0;
    float wedgeLife_ = 0;
    float wedgeCd_ = 0;
    float wedgeArm_ = 0;
    float beep0_ = 0;
    float beep2_ = 0;
    float fanT_ = 0;
    float blockFlash_ = 0;
    float shx_ = 0;
    float shy_ = 0;
    float scroll_ = 0;
    float t_ = 0;
    Wave waves_[64]{};
    Gust gusts_[8]{};
    Foe foes_[6]{};
    Puff puffs_[16]{};
};

}  // namespace rdoor
