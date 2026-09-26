// S3 SKIFF TURN — three bends in a creek. Heel too far and the skiff tips.
#pragma once
#include <vector>

#include "art.h"
#include "console/system.h"

namespace skiffturn {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SKIFF TURN"; }
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
    float heel() const { return heel_; }
    float lateral() const;
    int turns() const { return turns_; }
    // 0 title, 1 the straight, 2 in a bend, 3 two turns made, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Win, Fail };
    enum class Kind { BuoyR, BuoyG, Board, Reed, Heron, Shack, Dock };

    struct Prop {
        float x, z, h;
        Kind kind;
        int pal;
        int num;
    };
    struct Wake {
        float x, z, life;
    };
    struct Gull {
        float x, z, y, ph;
    };

    void begin();
    void showTitle();
    void startRun();
    void buildCourse();
    void controls(float& steer, float& throttle);
    void pilot(float& steer, float& throttle);
    void physics(float steer, float throttle);
    void markTurns(float zPrev);
    void win();
    void fail(const char* why);
    void blip(float freq);
    void chime();
    void audio();
    void draw();
    void drawCreek();
    void drawWorld();
    void drawHud();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, int fog = 0, bool shadow = false);
    bool project(float wx, float wy, float wz, float& sx, float& sy, float& scale, int& fog, float& rz) const;
    int heelFrame() const;
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
    const char* why_ = "";
    int turns_ = 0;
    int chimeN_ = 0, chimeStep_ = 0;
    int wakeN_ = 0;
    float t_ = 0, race_ = 0;
    float x_ = 0, z_ = 0, heading_ = 0, speed_ = 0, throttle_ = 0;
    float yaw_ = 0, heel_ = 0, heelVel_ = 0;
    float camBob_ = 0, shake_ = 0;
    float tone0_ = 0, tone1_ = 0, chimeT_ = 0, wakeT_ = 0;
    Wake wakes_[12]{};
    Gull gulls_[4]{};
    std::vector<Prop> props_;
};

}  // namespace skiffturn
