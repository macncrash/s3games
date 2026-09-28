// S3 SCULL TURN — three river bends in a single scull. Tip, or miss the end, and the leg is gone.
#pragma once
#include <vector>

#include "art.h"
#include "console/system.h"

namespace scullturn {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SCULL TURN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* why() const { return why_ ? why_ : ""; }
    float seconds() const { return race_; }
    float x() const { return x_; }
    float z() const { return z_; }
    float lean() const { return lean_; }
    int turns() const { return turns_; }
    // 0 title, 1 the reach, 2 in a bend, 3 two turns made, 4 the leg is over
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Win, Fail };
    enum class Kind { Post, Reed, House, Flag, Mark, Heron };

    struct Prop {
        float x, z, h;
        Kind kind;
        int num;
    };
    struct Wake {
        float x, z, life;
    };

    void begin();
    void showTitle();
    void startRun();
    void buildCourse();
    void controls(float& steer, float& rating, bool& set);
    void pilot(float& steer, float& rating, bool& set);
    void physics(float steer, float rating, bool set);
    void markTurns(float zPrev);
    void win();
    void fail(const char* why);
    void blip(float freq);
    void audio();
    void draw();
    void drawRiver();
    void drawWorld();
    void drawSky();
    void drawHud();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, int fog = 0, bool shadow = false);
    bool project(float wx, float wy, float wz, float& sx, float& sy, float& scale, int& fog, float& rz) const;
    int leanFrame() const;
    const char* tipWhy() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool made_[3] = {};
    bool setting_ = false;
    const char* why_ = "";
    int turns_ = 0;
    int wakeN_ = 0;
    float t_ = 0, race_ = 0;
    float x_ = 0, z_ = 0, heading_ = 0, speed_ = 0, rating_ = 0;
    float yaw_ = 0, lean_ = 0, leanVel_ = 0;
    float camBob_ = 0, shake_ = 0;
    float tone0_ = 0, wakeT_ = 0, catchT_ = 0;
    Wake wakes_[10]{};
    std::vector<Prop> props_;
};

}  // namespace scullturn
