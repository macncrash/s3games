// S3 BARGE MARK — take the barge and set down on the painted mark.
// The clock on the quay is the other crew. When it runs out, they have the mark.
#pragma once
#include <algorithm>
#include "console/system.h"
#include "game/art.h"

namespace bargemark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BARGE MARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* report() const { return report_; }
    const char* why() const { return why_; }
    float x() const { return x_; }
    float y() const { return y_; }
    float speed() const { return vel_; }
    float seconds() const { return race_; }
    float crewLeft() const { return std::max(0.f, limit_ - race_); }
    // 0 title, 1 the cut, 2 on the paint, 3 setting down, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    struct Puff {
        float x = 0, life = 0, rise = 0;
    };

    void begin();
    void showTitle();
    void startRun();
    void controls(float& thrust, float& steer);
    void pilot(float& thrust, float& steer);
    void physics(float thrust, float steer);
    bool onPaint() const;
    bool settled() const;
    void judge();
    void win();
    void fail(const char* why);
    void blip(float freq);
    void chime(int notes);
    void audio();
    void draw();
    void drawHud();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false, bool hflip = false);
    float sx(float wx) const;
    float syLat(float wy) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    const gs::Mipped* banner_ = nullptr;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int chimeN_ = 0, chimeStep_ = 0;
    int wakeCursor_ = 0, smokeCursor_ = 0;
    float t_ = 0, race_ = 0;
    float x_ = 0, y_ = 0, vel_ = 0, latV_ = 0;
    float hold_ = 0;
    float camX_ = 0;
    float tone0_ = 0, chimeT_ = 0, thumpT_ = 0;
    float wakeT_ = 0, smokeT_ = 0, shake_ = 0;
    float thrustIn_ = 0, steerIn_ = 0;
    float limit_ = 52.f;
    Puff wake_[10]{};
    Puff smoke_[5]{};
    char why_[72] = {};
    char report_[160] = {};
};

}  // namespace bargemark
