// S3 SKIFF PLAT — one skiff, one platform. Stop level with it.
#pragma once
#include "art.h"
#include "console/system.h"

namespace skiffplat {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SKIFF PLAT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* why() const { return why_ ? why_ : ""; }
    float seconds() const { return race_; }
    float x() const { return x_; }
    float y() const { return y_; }
    float heading() const { return heading_; }
    float speed() const { return ground_; }
    // 0 title, 1 the channel, 2 alongside, 3 holding level, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Win, Fail };

    struct Wake {
        float x = 0, y = 0, life = 0;
    };

    void begin();
    void showTitle();
    void startRun();
    void controls(float& steer, float& throttle);
    void pilot(float& steer, float& throttle);
    void physics(float steer, float throttle);
    bool sampleHull(float& east, const char*& why) const;
    bool inSlot(bool& yOk, bool& xOk, bool& hOk, bool& vOk) const;
    void win();
    void fail(const char* why);
    void blip(float freq);
    void chime();
    void audio();
    void camera();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, float minPx = 0);
    int hullFrame() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool wasSlot_ = false;
    const char* why_ = "";
    int chimeN_ = 0, chimeStep_ = 0;
    int wakeN_ = 0;
    float t_ = 0, race_ = 0, tide_ = 0, hold_ = 0;
    float x_ = 0, y_ = 0, heading_ = 0, speed_ = 0, throttle_ = 0;
    float gvx_ = 0, gvy_ = 0, ground_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 1.f;
    float tone0_ = 0, tone1_ = 0, chimeT_ = 0, wakeT_ = 0;
    Wake wakes_[16]{};
};

}  // namespace skiffplat
