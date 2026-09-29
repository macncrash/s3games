// S3 ANVILCHIME — a short anvil. The hour has to chime.
// The hammer walks a long rail. Only the short face, struck while the clock
// stands on twelve, makes the hour chime. Iron before the hour is pulled.
// Three blows, then the hour is gone.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace anvilchime {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 ANVILCHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool onTheHour() const { return onHour(); }
    int blows() const { return blows_; }
    int hour() const;
    int minute() const;
    int second() const;
    const char* reason() const { return reason_; }
    const char* face() const { return face_; }

private:
    enum class Mode { Title, Wait, Gap, Chime, Fail, Over };

    void toTitle();
    void begin();
    void strike();
    void beginChime();
    void beginGap(const char* why);
    void beginFail(const char* why);

    int clockSec() const;
    int phase() const;
    bool onHour() const;
    bool pastHour() const;
    bool onFace() const;
    void faceTime(int& h, int& m, int& s) const;

    void draw();
    void sky();
    void hand(float cx, float cy, float ang, float len, int pal);
    void spr(const gs::Image& img, float cx, float cy, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    const char* reason_ = "";
    const char* face_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int blows_ = 0;
    int playFrames_ = 0;
    int hold_ = 0;
    int strikes_ = 0;
    int titleWait_ = 0;
    float hammerX_ = 0;
    float hammerY_ = 0;
};

}  // namespace anvilchime
