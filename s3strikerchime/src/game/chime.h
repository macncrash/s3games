// S3 STRIKER CHIME — one swing on a carnival tower.
// The bell answers only when the puck meets it as the hour strikes.
// A ring before twelve is not this cartridge.
#pragma once

#include <string>

#include "console/system.h"
#include "game/art.h"

namespace strikerchime {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 STRIKER CHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool chimed() const { return chimed_; }
    int hour() const { return frozen_ ? fh_ : liveHour(); }
    int minute() const { return frozen_ ? fm_ : liveMinute(); }
    int second() const { return frozen_ ? fs_ : liveSecond(); }
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Ready, Rise, Fall, Chime, Fail };

    void begin();
    void swing();
    void strikeHour();
    void passHour(const char* why);
    void decay();
    void splitLive(int& h, int& m, int& s) const;
    int liveHour() const;
    int liveMinute() const;
    int liveSecond() const;
    float meter() const;
    bool onNotch() const;
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void word(const gs::Image& img, float cx, float y, int pal);
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool chimed_ = false;
    bool frozen_ = false;
    bool swung_ = false;
    bool clean_ = false;
    int playFrames_ = 0;
    int releaseFrame_ = 0;
    int arrive_ = -1;
    int hold_ = 0;
    int fh_ = 12, fm_ = 0, fs_ = 0;
    float bellAmp_ = 0;
    float toneT_ = 0;
    const char* reason_ = "hour silent";
};

}  // namespace strikerchime
