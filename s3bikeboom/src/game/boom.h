// S3 BIKE BOOM — take the bike and deliver the drive to the boom.
// The clock on the dock is the other crew.
#pragma once
#include <algorithm>
#include "console/system.h"
#include "game/art.h"

namespace bikeboom {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BIKE BOOM"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return race_; }
    float x() const { return x_; }
    float speed() const { return vel_; }
    float left() const { return std::max(0.f, limit_ - race_); }
    float driveX() const { return x_ + kRack; }
    int phase() const { return phase_; }
    const char* why() const { return why_; }
    // 0 title, 1 on the street, 2 drive under the boom, 3 holding the handoff, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    struct Puff {
        float x = 0, life = 0, rise = 0;
    };

    void begin();
    void showTitle();
    void startRun();
    void controls(float& pedal, float& brake, float& back);
    void pilot(float& pedal, float& brake, float& back);
    void physics(float pedal, float brake, float back);
    bool driveOnBoom() const;
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
    void sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal);
    float sx(float wx) const;

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
    int dustCursor_ = 0;
    float t_ = 0, race_ = 0, odo_ = 0;
    float x_ = 0, vel_ = 0;
    float hold_ = 0, outT_ = 0;
    float camX_ = 0;
    float tickT_ = 0;
    float tone0_ = 0, chimeT_ = 0, thumpT_ = 0;
    float dustT_ = 0, shake_ = 0;
    float pedalIn_ = 0;
    float lift_ = 0;
    float limit_ = 40.f;
    Puff dust_[8]{};
    char why_[80] = {};
};

}  // namespace bikeboom
