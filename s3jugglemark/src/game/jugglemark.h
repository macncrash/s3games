// S3 JUGGLEMARK — a short cascade.
// The gold ball is the mark. Catch it clean three times.
// That finished mark ends the cartridge.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace jugglemark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 JUGGLEMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool finished() const { return finished_; }
    int marks() const { return marks_; }
    int catches() const { return catches_; }

private:
    enum class Mode { Title, Play, Drop, Over };

    struct Air {
        bool on = false;
        int ball = 0;
        int to = 0;
        int age = 0;
    };

    void begin();
    void pattern();
    void finishMark();
    void miss();
    void stepPlay();
    void blip(float freq);
    void chord(float a, float b, float c);
    void ageTone();
    void draw();
    void backdrop();
    void ballAt(int ball, float x, float y, float scale);
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool finished_ = false;
    int marks_ = 0;
    int catches_ = 0;
    int tick_ = 0;
    int wait_ = 0;
    int titleWait_ = 0;
    int latch_[2] = {};
    int hold_[2][2] = {};
    int hn_[2] = {};
    Air air_[3]{};
    float beep_ = 0;
};

}  // namespace jugglemark
