// S3 BOARDCHIME — a night switchboard under a wall clock.
// Four lamps have to be seated on their own lines. The board stays up.
// Nothing is won early. At twelve the hour either chimes or passes quiet.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace boardchime {

constexpr int kOpenSec = 11 * 3600 + 59 * 60 + 52;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BOARDCHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool chimed() const { return chimed_; }
    int made() const { return made_; }
    int hour() const { return hour_; }
    int minute() const { return minute_; }
    int second() const { return second_; }
    const char* reason() const { return why_; }

private:
    enum class Mode { Title, Patch, Chime, Fail, Over };

    struct Call {
        bool on = false;
        int trunk = 0;
        int line = 0;
        float life = 0;
        float lifeMax = 1;
    };

    void toTitle();
    void begin();
    void spawn();
    void lift();
    void seat();
    void miss(const char* why);
    void strike();
    void passHour(const char* why);
    void steer();
    void botAct();
    void readClock();
    void audio(float dt);
    void draw();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    int irnd(int n);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Call call_{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool chimed_ = false;
    bool held_ = false;
    int made_ = 0;
    int bank_ = 0;
    int row_ = 0;
    int holdTrunk_ = 0;
    int holdLine_ = 0;
    int playFrames_ = 0;
    int gap_ = 0;
    int hold_ = 0;
    int hour_ = 11;
    int minute_ = 59;
    int second_ = 52;
    int chimeStep_ = 0;
    const char* why_ = "hour silent";
    float clockVis_ = 0;
    float toneT_ = 0;
    uint32_t rng_ = 0xC41Cu;
};

}  // namespace boardchime
