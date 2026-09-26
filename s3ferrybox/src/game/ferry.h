// S3 FERRY BOX — the ferry has one job: stop inside the box.
// Close to the box is still outside. The east set will not let her stop by itself.
#pragma once
#include "game/art.h"
#include "console/system.h"

namespace ferrybox {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FERRY BOX"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return race_; }
    float x() const { return x_; }
    float y() const { return y_; }
    float heading() const { return heading_; }
    float speed() const { return speed_; }
    int phase() const { return phase_; }
    const char* why() const { return why_; }
    // 0 title, 1 on the way, 2 in the box, 3 holding the stop, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    struct Puff {
        float x = 0, y = 0, life = 0;
    };

    void begin();
    void showTitle();
    void startRun();
    void controls(float& thrust, float& rudder, float& bow);
    void pilot(float& thrust, float& rudder, float& bow);
    void physics(float thrust, float rudder, float bow);
    void resolveBanks();
    void corners(float xs[4], float ys[4]) const;
    int insideCount() const;
    bool hullInside() const;
    void judge();
    void win();
    void fail(const char* why);
    void horn();
    void blip(float freq);
    void chime(int notes);
    void audio();
    void driftFx(float dt);
    void camera();
    void draw();
    void drawHud();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal);
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal);
    void worldToScreen(float wx, float wy, float& sx, float& sy) const;
    void localWorld(float lx, float ly, float& wx, float& wy) const;
    int shipFrame() const;
    void edge(float x0, float y0, float x1, float y1, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    const gs::Mipped* banner_ = nullptr;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool announced_ = false;
    int phase_ = 0;
    int chimeN_ = 0, chimeStep_ = 0;
    int wakeCursor_ = 0, smokeCursor_ = 0;
    float t_ = 0, race_ = 0;
    float x_ = 0, y_ = 0, heading_ = 0;
    float surge_ = 0, sway_ = 0, yaw_ = 0;
    float speed_ = 0;
    float hold_ = 0, outT_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 1.f;
    float hornT_ = 0, hornF_ = 98.f;
    float tone0_ = 0, chimeT_ = 0, thumpT_ = 0;
    float wakeT_ = 0, smokeT_ = 0, shake_ = 0;
    float thrustIn_ = 0;
    Puff wake_[14]{};
    Puff smoke_[8]{};
    Puff foam_[8]{};
    char why_[72] = {};
};

}  // namespace ferrybox
