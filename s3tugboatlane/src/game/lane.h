// S3 TUGBOAT LANE — stay in the lane for the whole leg.
#pragma once
#include <vector>

#include "art.h"
#include "console/system.h"

namespace tuglane {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TUGBOAT LANE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* why() const { return why_ ? why_ : ""; }
    float seconds() const { return float(race_); }
    float x() const { return float(x_); }
    float y() const { return float(y_); }
    float heading() const { return float(heading_); }
    float speed() const;
    float lateral() const;
    float clearance() const;
    // 0 title, 1 the lane, 2 on the edge, 3 the gate is ahead, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Win, Fail };
    enum class Kind { BuoyR, BuoyG, Shed, House, Crate, Bollard, Lamp, Post };

    struct Prop {
        double x, y;
        float h;
        Kind kind;
        int pal;
        bool flip;
    };
    struct Puff {
        double x, y, life;
    };

    void begin();
    void showTitle();
    void startRun();
    void buildCourse();
    void controls(double& steer);
    void pilot(double& steer);
    void physics(double steer);
    void judge();
    void win();
    void fail(const char* why);
    void blip(float freq);
    void horn(float seconds);
    void chime(int notes);
    void audio();
    void camera();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool shadow = false);
    void sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal);
    void place(const gs::Mipped& m, double wx, double wy, float worldH, int pal, float minPx = 0, bool flip = false);
    void worldRect(const gs::Mipped& m, double wx, double wy, double ww, double hh, int pal);
    int hullFrame() const;
    void puff(Puff* ring, int& cursor, int n, double x, double y);
    void stackAt(double& sx, double& sy) const;
    double cornerClearance() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool warned_ = false;
    bool sawGate_ = false;
    const char* why_ = "";
    int chimeN_ = 0, chimeStep_ = 0;
    int wakeCursor_ = 0, smokeCursor_ = 0;
    double t_ = 0, race_ = 0;
    double x_ = 0, y_ = 0, heading_ = 0, surge_ = 0, throttle_ = 0, yaw_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 1.f;
    float tone0_ = 0, tone1_ = 0, chimeT_ = 0, hornT_ = 0;
    float wakeT_ = 0, smokeT_ = 0, shake_ = 0;
    Puff wake_[16]{};
    Puff smoke_[8]{};
    std::vector<Prop> props_;
};

}  // namespace tuglane
