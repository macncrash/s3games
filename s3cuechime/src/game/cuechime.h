// S3 CUECHIME — a short cue. The hour has to chime.
// The object ball has to fall on the hour. A pot while the clock is still short does not count.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace cuechime {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CUECHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool potted() const { return potted_; }
    bool travelling() const { return mode_ == Mode::Travel && travel_ > 8 && travel_ < kTravel - 4; }
    int strokes() const { return strokes_; }
    int hour() const;
    int minute() const;
    int second() const;
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Aim, Travel, Early, Thin, Chime, Fail };

    static constexpr int kTravel = 42;

    void showTitle();
    void begin();
    void stroke();
    void resolve();
    void botPlay();
    bool inWindow() const;
    float meter() const;
    float cueTip() const;
    int clockSec() const;
    int framesUntilHour() const;
    bool onHour() const;
    bool pastHour() const;
    void split(int& h, int& m, int& s) const;
    void hush();

    void draw();
    void backdrop();
    void clockAt(float cx, float cy);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool hflip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool potted_ = false;
    bool botHeld_ = false;
    int strokes_ = 0;
    int playFrames_ = 0;
    int titleFrames_ = 0;
    int modeFrames_ = 0;
    int travel_ = 0;
    int strikes_ = 0;
    int lastSec_ = -1;
    const char* reason_ = "";
    float phase_ = 0.2f;
    float ballT_ = 0;
    float toneT_ = 0;
    float swing_ = 0;
};

}  // namespace cuechime
