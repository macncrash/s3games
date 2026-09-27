// S3 SKATEBELL — skate the street, ring the bell, leave.
// Three tries. A fall in the gap or a miss under the bell kills the try.
// The third dead try ends it. Leaving after the bell is the win.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace skatebell {

constexpr float kGroundY = 176.f;
constexpr float kGap0 = 392.f;
constexpr float kGap1 = 496.f;
constexpr float kBellX = 820.f;
constexpr float kBellY = 108.f;
constexpr float kLeaveX = 1040.f;
constexpr float kWorld = 1200.f;
constexpr int kTries = 3;

inline bool groundAt(float x) {
    if (x < 16.f || x > kWorld - 8.f) return false;
    if (x >= kGap0 && x < kGap1) return false;
    return true;
}

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SKATEBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    bool rules() const { return rules_; }
    bool riding() const { return mode_ == Mode::Ride && px_ > 180.f && px_ < kBellX; }
    int deadTries() const { return dead_; }
    int tryNo() const { return tryNo_; }

private:
    enum class Mode { Title, Ride, Dead, Leave, Over };

    void toTitle();
    void newGame();
    void beginTry();
    void ride();
    void ring();
    void dieTry(const char* why);
    bool audit() const;
    void blip(int ch, float freq, float vol, int frames);
    void pumpAudio();
    void draw();
    void backdrop();
    void stamp(const gs::Image& img, float x, float y, int pal, bool shadow = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    bool rules_ = false;
    bool onGround_ = true;
    bool gapJumped_ = false;
    bool bellJumped_ = false;
    int dead_ = 0;
    int tryNo_ = 0;
    int t_ = 0;
    int wait_ = 0;
    int blipLeft_ = 0;
    float px_ = 48.f;
    float py_ = kGroundY;
    float vx_ = 0.f;
    float vy_ = 0.f;
    float bellPh_ = 0.f;
    float bellAmp_ = 0.12f;
    int bellTick_ = 0;
    char last_[12] = {};
};

}  // namespace skatebell
