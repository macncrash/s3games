// S3 BELLCHIME — a short bell. The hour has to chime.
// Three ropes. Only the short bell, pulled while the clock stands on twelve,
// makes the hour chime. A long or deep bell is the wrong voice. Three pulls,
// then the hour is gone.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace bellchime {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BELLCHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool onTheHour() const { return onHour(); }
    int pulls() const { return pulls_; }
    int hour() const;
    int minute() const;
    int second() const;
    const char* reason() const { return reason_; }
    const char* bell() const { return bell_; }

private:
    enum class Mode { Title, Wait, Gap, Chime, Fail, Over };

    void toTitle();
    void begin();
    void pull();
    void beginChime();
    void beginGap(const char* why);
    void beginFail(const char* why);

    int clockSec() const;
    bool onHour() const;
    bool pastHour() const;
    void faceTime(int& h, int& m, int& s) const;

    void draw();
    void sky();
    void hand(float cx, float cy, float ang, float len, int pal);
    void spr(const gs::Image& img, float cx, float cy, int pal, int dw = 0, int dh = 0);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    const char* reason_ = "";
    const char* bell_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int pulls_ = 0;
    int sel_ = 0;
    int playFrames_ = 0;
    int hold_ = 0;
    int strikes_ = 0;
    int titleWait_ = 0;
    float yank_ = 0;
};

}  // namespace bellchime
