// S3 TUGBOAT PLAT — stop the tug level with the platform.
// The clock is the other crew. Close is not level: midships on the stripe,
// a step off the face, squared up, and a full stop, held.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace tugplat {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TUGBOAT PLAT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* why() const { return why_ ? why_ : ""; }
    float seconds() const { return race_; }
    float crewLeft() const;
    float x() const { return x_; }
    float y() const { return y_; }
    float heading() const { return heading_; }
    float speed() const { return ground_; }
    // 0 title, 1 the channel, 2 alongside, 3 holding level, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Win, Fail };

    struct Puff {
        float x = 0, y = 0, life = 0;
    };

    void begin();
    void showTitle();
    void startRun();
    void controls(float& steer, float& throttle);
    void pilot(float& steer, float& throttle) const;
    void pose(float& steer, float& throttle) const;
    void physics(float steer, float throttle);
    bool sampleHull(float& east, const char*& why) const;
    bool inSlot(bool& yOk, bool& xOk, bool& hOk, bool& vOk) const;
    void win();
    void fail(const char* why);
    void blip(float freq);
    void horn(float seconds);
    void chime();
    void audio();
    void puffs();
    void camera();
    void draw();
    void backdrop();
    void drawBoat(float wx, float wy, float hdg, int pal, float bob);
    void drawWorld();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, bool shadow = false);
    void text(const char* s, float x, float y, float scale, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void hudR(int row, const char* s, int pal);
    void rivalAt(float& x, float& y, float& h) const;
    const char* hint() const;
    int yawOf(float h) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool wasSlot_ = false;
    const char* why_ = "";
    float t_ = 0, race_ = 0, hold_ = 0;
    float x_ = 0, y_ = 0, heading_ = 0, speed_ = 0, throttle_ = 0;
    float gvx_ = 0, gvy_ = 0, ground_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 1.f;
    float hornT_ = 0, blipT_ = 0, chimeT_ = 0, smokeT_ = 0, wakeT_ = 0;
    int chimeN_ = 0, chimeStep_ = 0;
    int smokeN_ = 0, wakeN_ = 0;
    Puff smokes_[8]{};
    Puff wakes_[12]{};
};

}  // namespace tugplat
