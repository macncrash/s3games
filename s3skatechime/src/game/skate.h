// S3 SKATECHIME — skate until the hour has to chime. Leave when that is true.
// An ollie over the gold, while the clock is on twelve, is the only chime.
// Rolling the gold, or arriving after the grace, spends the try. Three tries.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace skatechime {

constexpr float kGroundY = 176.f;
constexpr float kGold0 = 540.f;
constexpr float kGold1 = 640.f;
constexpr float kTowerX = 574.f;
constexpr float kLeaveX = 900.f;
constexpr float kWorld = 1120.f;
constexpr int kTries = 3;
constexpr int kGraceSec = 8;
constexpr int kFpc = 6;
constexpr int kLeadSec = 32;
constexpr int kHourSec = 12 * 3600;

inline bool onGold(float x) { return x >= kGold0 && x < kGold1; }

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SKATECHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool onTheHour() const { return onHour(); }
    bool chimed() const { return chimed_; }
    bool riding() const { return mode_ == Mode::Ride && px_ > 160.f && px_ < kGold0; }
    int hour() const;
    int minute() const;
    int second() const;
    int tries() const { return tryNo_; }
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Ride, Early, Chime, Leave, Fail, Over };

    void toTitle();
    void newGame();
    void beginTry();
    void ride();
    void beginChime();
    void beginEarly();
    void beginFail(const char* why);
    bool audit() const;
    int clockSec() const;
    bool onHour() const;
    bool pastHour() const;
    void face(int& h, int& m, int& s) const;
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
    const char* reason_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool chimed_ = false;
    bool onGround_ = true;
    bool jumped_ = false;
    int dead_ = 0;
    int tryNo_ = 0;
    int t_ = 0;
    int wait_ = 0;
    int playFrames_ = 0;
    int blipLeft_ = 0;
    int bellTick_ = 0;
    int strikes_ = 0;
    float px_ = 48.f;
    float py_ = kGroundY;
    float vx_ = 0.f;
    float vy_ = 0.f;
    float bellPh_ = 0.f;
    float bellAmp_ = 0.1f;
};

}  // namespace skatechime
