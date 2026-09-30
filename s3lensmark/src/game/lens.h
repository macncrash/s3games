// S3 LENSMARK — a short lens. Focus, frame, and trip the shutter.
// The print is done only when the finished mark sits in the corner.
#pragma once
#include <string>

#include "console/gfx.h"
#include "console/system.h"

namespace lens {

enum Pal {
    PAL_HUD = 0,
    PAL_BRASS = 1,
    PAL_SCENE = 2,
    PAL_FIGURE = 3,
    PAL_PAPER = 4,
    PAL_GLASS = 5,
    PAL_INK = 6
};

struct Art {
    gs::Mipped body, barrel, ring, glass, haze, finder;
    gs::Mipped figure, lamp, pier, scale, caret, stamp, print, corner;
    gs::Mipped glyph[96];
    int font[96] = {};
};

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LENSMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int plates() const { return plates_; }

private:
    enum class Mode { Title, Focus, Frame, Expose, Develop, Over };

    void buildArt();
    void backdrop();
    void update(float dt);
    void draw();
    void hud(int col, int row, const std::string& s);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void blip(float freq);
    void fanfare();
    bool press(gs::Button b) const;
    bool held(gs::Button b) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool sharp_ = false;
    bool framed_ = false;
    int plates_ = 3;
    float t_ = 0;
    float focus_ = 0.18f;
    float trueFocus_ = 0.71f;
    float camX_ = -34.f;
    float camY_ = 18.f;
    float phase_ = 0.15f;
    float slide_ = 0;
    float msgT_ = 0;
    std::string msg_;
};

}  // namespace lens
