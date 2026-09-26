// S3 SKIFF PASS — one skiff. Clear the pass before the storm clock dies.
#pragma once
#include "art.h"
#include "console/system.h"

namespace skiffpass {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SKIFF PASS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* why() const { return why_ ? why_ : ""; }
    float seconds() const { return playT_; }
    float clockLeft() const;
    float x() const { return x_; }
    float y() const { return y_; }
    float heading() const { return hdg_; }
    float speed() const { return speed_; }
    float stormY() const { return stormY_; }
    // 0 title, 1 the pass, 2 the narrows, 3 the mouth, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Win, Fail };
    static constexpr int PATH_N = 180;

    struct Wake {
        float x = 0, y = 0, life = 0;
    };
    struct Rock {
        float x = 0, y = 0, r = 1, bias = 0;
    };

    void resetPose();
    void bake();
    void begin();
    void showTitle();
    void snapCamera();
    void followCamera();
    void step();
    void controls();
    void pilot(float& thrust, float& steer) const;
    void integrate(float dt);
    const char* contacts();
    bool boatClear() const;
    bool stormTouches() const;
    void sampleHull(float* xs, float* ys, int& n) const;
    void fail(const char* why);
    void win();
    void scrape();
    void updateGust();
    void wakes();
    void chime(bool big);
    void audio();
    void draw();
    void backdrop();
    void drawWorld();
    void drawBoat();
    void drawStorm();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false, bool flip = false,
             int fog = 0);
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, bool flip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal);
    float centerAt(float y) const;
    float halfAt(float y) const;
    float clearance(float x, float y) const;
    float pathX(float y) const;
    float sx(float wx) const;
    float sy(float wy) const;
    int yawFrame() const;
    float urgency() const;
    const char* hint() const;
    bool flashOn() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool stormFail_ = false;
    const char* why_ = "";
    float t_ = 0;
    float playT_ = 0;
    float x_ = 0, y_ = 8, hdg_ = 0;
    float speed_ = 0, yawV_ = 0;
    float driftX_ = 0, driftY_ = 0;
    float thrust_ = 0, steer_ = 0, throttle_ = 0;
    float gust_ = 0;
    float stormY_ = -8;
    float camX_ = 0, camY_ = 16, zoom_ = 4.f;
    float shake_ = 0, shx_ = 0, shy_ = 0;
    float wakeT_ = 0;
    float scrapeT_ = -1.f;
    float beepHold_ = 0;
    int beepSec_ = -1;
    int wakeN_ = 0;
    int chimeStep_ = -1;
    float chimeT_ = 0;
    bool chimeBig_ = false;
    bool flashWas_ = false;
    int rockN_ = 0;
    Rock rock_[8]{};
    float path_[PATH_N]{};
    Wake wakes_[16]{};
};

}  // namespace skiffpass
