// S3 LANTERNCHIME — the hour has to chime.
// Kindle every post lantern, then pull the bell rope while the hand is on twelve.
// A pull off the hour, or before the lamps are lit, spends a strike. Three strikes and the hour is gone.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace lanternchime {

constexpr int kLamps = 4;
constexpr int kStrikes = 3;
constexpr float kPosts[kLamps] = {40.f, 112.f, 208.f, 280.f};
constexpr float kRopeX = 160.f;
constexpr int kCycle = 150;
constexpr int kWindow = 28;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LANTERNCHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int lamps() const;
    int strikes() const { return strikes_; }
    int hour() const { return hour_; }
    int minute() const { return minute_; }
    int second() const { return second_; }
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Play, Chime, Fail, Over };

    void toTitle();
    void beginPlay();
    void beginChime();
    void beginFail(const char* why);
    void finish();
    void botIntent();
    void tickPlay();
    bool inWindow() const;
    bool allLit() const;
    int nearestDark() const;
    void readClock();
    void note(int ch, float freq, float vol);
    void draw();
    void sky();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool shadow = false);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    const char* reason_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool lit_[kLamps] = {};
    bool wantL_ = false;
    bool wantR_ = false;
    bool wantA_ = false;
    bool prevA_ = false;
    bool faceLeft_ = false;
    bool ropeTap_ = false;
    int strikes_ = 0;
    int playFrames_ = 0;
    int timer_ = 0;
    int charge_ = -1;
    int chargeT_ = 0;
    int tolls_ = 0;
    int hour_ = 11;
    int minute_ = 59;
    int second_ = 0;
    float x_ = 40.f;
};

}  // namespace lanternchime
