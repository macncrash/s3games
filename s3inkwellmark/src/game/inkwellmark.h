// S3 INKWELLMARK — one quill and a glass well.
// A wet dip lays a stroke. Four wet strokes finish the mark.
// That finished mark ends the cartridge. A dry dip does not.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace inkwellmark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 INKWELLMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool finished() const { return finished_; }
    bool inked() const { return inked_; }
    int strokes() const { return good_; }
    int dips() const { return dips_; }

private:
    enum class Mode { Title, Play, Show, Lose };

    void begin();
    void dip();
    void finishMark();
    void fail();
    void botPlay();
    bool inWell() const;
    float nibY() const;

    void blip(bool high);
    void wet();
    void hush();

    void draw();
    void backdrop();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
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
    bool inked_ = false;
    bool botHeld_ = false;
    int good_ = 0;
    int dips_ = 0;
    int dry_ = 0;
    float phase_ = 0;
    float modeT_ = 0;
    float flash_ = 0;
    float toneT_ = 0;
};

}  // namespace inkwellmark
