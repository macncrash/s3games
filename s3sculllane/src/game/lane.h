// S3 SCULL LANE — stay inside the buoyed lane for the whole leg.
// The other crew is only a clock. Leave the lane, or let them finish first, and the leg is lost.
#pragma once
#include "art.h"
#include "console/system.h"

namespace sculllane {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SCULL LANE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* why() const { return why_ ? why_ : ""; }
    float seconds() const { return clock_; }
    float lateral() const { return x_; }
    float along() const { return dist_; }
    // 0 title, 1 the lane, 2 near a buoy line, 3 the finish is in sight, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Row, Pause, Win, Fail };

    struct Proj {
        float x = 0, y = 0, ppm = 0;
        int fog = 0;
        bool ok = false;
    };

    void toTitle();
    void begin();
    void update(float dt);
    void finish(bool good, const char* why);
    void audio();
    void draw();
    void water(float view);
    void buoys(float along);
    void shellAt();
    void text(int col, int row, const char* s, int pal);
    void textC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0);
    Proj project(float wx, float ahead) const;
    float bendAt(float s) const;
    float halfAt(float s) const;
    float driftAt(float s) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* why_ = "";
    float t_ = 0, clock_ = 0, dist_ = 0, x_ = 0, vx_ = 0, helm_ = 0, speed_ = 0, stroke_ = 0;
    float scenery_ = 0;
};

}  // namespace sculllane
