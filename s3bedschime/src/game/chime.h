// S3 BEDSCHIME — six beds under a wall clock.
// Pour each bed into the gold band and leave it. Nothing is won early.
// At twelve the hour either chimes or passes quiet.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace bedschime {

constexpr int kOpenSec = 11 * 3600 + 59 * 60 + 48;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BEDSCHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool chimed() const { return chimed_; }
    int wet() const { return wet_; }
    int hour() const { return hour_; }
    int minute() const { return minute_; }
    int second() const { return second_; }
    const char* reason() const { return why_; }

private:
    enum class Mode { Title, Pour, Chime, Fail, Over };

    struct Bed {
        float fill = 0;
        bool held = false;
    };

    void toTitle();
    void begin();
    void readClock();
    void lockBed();
    void flood();
    void miss(const char* why);
    void strike();
    void passHour(const char* why);
    void steer();
    void botAct();
    void audio(float dt);
    void draw();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    static constexpr float kGoldLo = 0.50f;
    static constexpr float kGoldHi = 0.82f;
    static constexpr float kPour = 1.15f;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Bed bed_[kBeds]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool chimed_ = false;
    bool pouring_ = false;
    int wet_ = 0;
    int cursor_ = 0;
    int playFrames_ = 0;
    int hold_ = 0;
    int hour_ = 11;
    int minute_ = 59;
    int second_ = 48;
    int chimeStep_ = 0;
    const char* why_ = "hour silent";
    float vis_ = 0;
    float toneT_ = 0;
};

}  // namespace bedschime
