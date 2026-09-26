// S3 TUGBOAT GRASS — land on the grass and come to a full stop.
// The flood sets you toward a grass bank. The bank keeps pulling. Hold the stop.
#pragma once
#include "art.h"
#include "console/system.h"

namespace tuggrass {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TUGBOAT GRASS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return race_; }
    const char* why() const { return why_; }
    float x() const { return x_; }
    float y() const { return y_; }
    float heading() const { return heading_; }
    float speed() const { return speed_; }
    float engine() const { return throttle_; }
    bool onGrass() const { return full_; }
    int phase() const { return phase_; }
    // 0 title, 1 the channel, 2 on the grass, 3 holding the stop, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    struct Puff {
        float x = 0, y = 0, life = 0;
    };
    struct Ext {
        float minX, maxX, minY, maxY;
    };

    void begin();
    void showTitle();
    void startRun();
    void controls(float& steer);
    void pilot(float& steer);
    void physics(float steer);
    Ext extents() const;
    bool hullOnGrass(const Ext& e) const;
    float grassCover(const Ext& e) const;
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
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, float minPx = 0, bool flip = false);
    void worldRect(const gs::Mipped& m, float wx, float wy, float ww, float hh, int pal);
    int hullFrame() const;
    void puff(Puff* ring, int& cursor, int n, float x, float y);
    const char* hint() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool full_ = false;
    bool deep_ = false;
    bool launched_ = false;
    int phase_ = 0;
    int chimeN_ = 0, chimeStep_ = 0;
    int wakeCursor_ = 0, smokeCursor_ = 0;
    float t_ = 0, race_ = 0, hold_ = 0, shortT_ = 0, noseT_ = 0, cover_ = 0;
    float x_ = 0, y_ = 0, heading_ = 0, vx_ = 0, vy_ = 0, throttle_ = 0, speed_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 1.f;
    float tone0_ = 0, tone1_ = 0, chimeT_ = 0, thumpT_ = 0, hornT_ = 0;
    float wakeT_ = 0, smokeT_ = 0, shake_ = 0, stuckT_ = 0, stuckX_ = 0, stuckY_ = 0;
    char why_[64] = {};
    Puff wake_[16]{};
    Puff smoke_[8]{};
};

}  // namespace tuggrass
