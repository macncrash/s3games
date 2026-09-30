// S3 PRESSCHIME — a short press. The hour has to chime.
// The platen walks a long stroke. Only the short span, taken while the clock
// stands on twelve, makes the hour chime. A long pull, or a short one before
// the hour, leaves the shop silent.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace presschime {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PRESSCHIME"; }
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
    const char* face() const { return face_; }

private:
    enum class Mode { Title, Wait, Chime, Fail, Over };

    void toTitle();
    void begin();
    void pull();
    void beginChime();
    void beginFail(const char* why);

    int clockSec() const;
    int phase() const;
    bool onHour() const;
    bool pastHour() const;
    bool onShort() const;
    void faceTime(int& h, int& m, int& s) const;
    float drop() const;

    void draw();
    void shop();
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
    int pulls_ = 0;
    int playFrames_ = 0;
    int hold_ = 0;
    int strikes_ = 0;
    int titleWait_ = 0;
};

}  // namespace presschime
