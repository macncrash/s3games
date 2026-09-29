// S3 CAUSEWAY DOOR — hold the iron door for three minutes.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace causewaydoor {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CAUSEWAY DOOR"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float gap() const { return gap_; }
    // 0 title, 1 the door, 2 held, 3 the watch is over
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Won, Lost };
    enum Kind { LEAN_L, LEAN_R, BRACE, CHAIN, SHOULDER };

    struct Beat {
        int at = 0;
        int dur = 0;
        int kind = 0;
    };

    void begin();
    void toTitle();
    void finish(bool held);
    void layBeats();
    void update();
    void audio();
    void draw();
    void sky();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0);

    bool holdLeft() const;
    bool holdRight() const;
    bool holdA() const;
    bool holdB() const;
    bool holdC() const;
    bool correct() const;
    const char* cue() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int age_ = 0;
    int beatCount_ = 0;
    int beat_ = 0;
    float gap_ = 0.15f;
    float shake_ = 0;
    float lamp_ = 0;
    float blip_ = 0;
    Beat beats_[72]{};
};

}  // namespace causewaydoor
