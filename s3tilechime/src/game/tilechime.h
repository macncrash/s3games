// S3 TILECHIME — set the floor. The hour has to chime.
// A chime tile seated in the twelve socket while the clock stands on twelve
// makes the hour chime. A plain tile mutes it. A chime tile before the hour
// is pulled. The hour does not wait.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace tilechime {

constexpr int kStartSec = 11 * 3600 + 59 * 60 + 50;
constexpr int kHourSec = 12 * 3600;
constexpr int kFpc = 6;
constexpr int kGraceSec = 3;
constexpr int kPulls = 3;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TILECHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool onTheHour() const { return onHour(); }
    int chimes() const { return chimes_; }
    int hour() const;
    int minute() const;
    int second() const;
    const char* reason() const { return reason_; }
    const char* tile() const { return tile_; }

private:
    enum class Mode { Title, Play, Chime, Fail };
    enum class Kind { Chime, Plain };

    void toTitle();
    void begin();
    void seat(Kind kind);
    void beginChime();
    void beginFail(const char* why);

    int clockSec() const;
    bool onHour() const;
    bool pastHour() const;
    void faceTime(int& h, int& m, int& s) const;

    void draw();
    void sky();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal);
    void hand(float cx, float cy, float ang, float len, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    const char* reason_ = "";
    const char* tile_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int chimes_ = 0;
    int pulls_ = 0;
    int playFrames_ = 0;
    int hold_ = 0;
    bool socket_ = false;
    Kind seated_ = Kind::Plain;
};

}  // namespace tilechime
