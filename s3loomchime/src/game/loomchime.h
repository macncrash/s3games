// S3 LOOMCHIME — play loom until the hour has to chime. Leave when that is true.
// A shed thrown on the hour is the only pick that stays. Anything earlier is pulled.
// A dead pass spends a shuttle. Three shuttles, then the hour is gone.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace loomchime {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LOOMCHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool onTheHour() const { return onHour(); }
    int hour() const;
    int minute() const;
    int second() const;
    int laid() const { return laid_; }
    int early() const { return early_; }
    int dead() const { return dead_; }
    int spent() const { return spent_; }
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Pass, Early, Gap, Chime, Over };

    void begin();
    void throwPick(bool sweet);
    void killPass();
    void fail(const char* why);
    void chime();
    void nextPass();

    int clockSec() const;
    bool onHour() const;
    bool pastHour() const;
    void face(int& h, int& m, int& s) const;

    void draw();
    void sky();
    void spr(const gs::Image& img, float cx, float cy, int pal, bool hflip = false);
    void hand(float cx, float cy, float ang, float len, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    const char* reason_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int spent_ = 0;
    int dead_ = 0;
    int early_ = 0;
    int laid_ = 0;
    int age_ = 0;
    int anim_ = 0;
    int playFrames_ = 0;
    int dir_ = 1;
};

}  // namespace loomchime
