// S3 HORNMARK — one ridge, one hunting call.
// Four notes sit on the staff. Blow each in order and let the last breath go.
// That finished mark ends the cartridge. A sour note is still open.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace hornmark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HORNMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finished() const { return finished_; }
    bool closed() const { return closed_; }
    bool blowing() const { return blowing_; }
    int notes() const { return locked_; }
    const char* callName() const;

private:
    enum class Mode { Title, Call, Done };

    struct Input {
        bool blow = false;
        bool start = false;
        int pitch = -1;  // -1 keep, else 0..2
    };

    static constexpr int kNotes = 4;
    static constexpr int kPitches = 3;

    void begin();
    void lockNote();
    void sour();
    void finish();
    void stepCall(const Input& in);
    Input readPad(const gs::Pad& pad) const;
    Input botInput() const;
    void toneAt(int pitch, bool on);
    void fanfare();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void backdrop();
    void draw();
    int staffY(int pitch) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    const int phrase_[kNotes] = {0, 2, 1, 2};
    int pitch_ = 0;
    int locked_ = 0;
    int hold_ = 0;
    int sourHold_ = 0;
    int titleWait_ = 0;
    int doneWait_ = 0;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finished_ = false;
    bool closed_ = false;
    bool blowing_ = false;
    bool released_ = true;
    float clock_ = 0;
};

}  // namespace hornmark
