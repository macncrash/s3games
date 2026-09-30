// S3 LENSCHIME — play the lens until the hour has to chime. Leave when that is true.
// Rack focus and pan the tower clock. A plate that cracks before noon is spent.
// Leaving before twelve is a loss. Only a sharp leave inside the hour is the chime.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace lenschime {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LENSCHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool onTheHour() const { return onHour(); }
    int hour() const;
    int minute() const;
    int second() const;
    int plates() const { return plates_; }
    int cracked() const { return cracked_; }
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Play, Crack, Chime, Leave, Fail };

    void begin();
    void crack();
    void chime();
    void leave();
    void fail(const char* why);
    void botAim();
    void human(const gs::Pad& pad);
    bool sharp() const;
    bool audit() const;
    int clockSec() const;
    bool onHour() const;
    bool pastHour() const;
    void face(int& h, int& m, int& s) const;
    float wantF() const;
    float wantX() const;

    void draw();
    void sky();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0);
    void ray(float cx, float cy, float ang, float len, int pal);
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
    int plates_ = kPlates;
    int cracked_ = 0;
    int playFrames_ = 0;
    int hold_ = 0;
    int bad_ = 0;
    int t_ = 0;
    int strikes_ = 0;
    float focus_ = 0.35f;
    float pan_ = -20.f;
};

}  // namespace lenschime
