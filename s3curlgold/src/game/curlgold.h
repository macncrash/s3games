// S3 CURL GOLD — one end, four stones a side, hammer last.
// A stone in the gold counts two. A stone in the cream counts one.
// Cream does not buy the double: the end is won only when the shot rock lies in the gold.
#pragma once

#include "console/system.h"
#include "game/art.h"
#include "game/sheet.h"

namespace curlgold {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CURL GOLD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int red() const { return red_; }
    int yel() const { return yel_; }
    int gold() const { return taker_ == 0 ? takerGold_ : 0; }
    int cream() const { return taker_ == 0 ? takerCream_ : 0; }
    float redLie() const { return redLie_; }
    float yelLie() const { return yelLie_; }
    int delivered() const { return shot_; }
    bool sliding() const { return mode_ == Mode::Slide; }

private:
    enum class Mode { Title, Aim, Slide, Result, Pause };

    struct Puff {
        float x = 0, y = 0, life = 0;
    };

    void begin();
    void enterAim();
    void launch(int side, bool ai);
    void onRest();
    void finishEnd();
    Shot choose(int side) const;
    void predict(int side, float sweep);
    void updateAim();
    void updateSlide();
    float focusY() const;
    void seekCam(float focus, float bias, bool snap);
    void tickAudio();
    void blip(float freq, float vol, float hold);
    void draw();
    void drawRock(float x, float y, int side, int curl, bool shadow);
    void spr(const gs::Image& img, float cx, float cy, int w, int h, int pal, bool flip, bool shadow, int fog);
    void sprM(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow, int fog);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    int throwSide() const { return (shot_ % 2 == 0) ? 1 : 0; }

    gs::System* sys_ = nullptr;
    Art art_{};
    Sheet sheet_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool manual_ = false;
    bool picked_ = false;
    bool aiThrow_ = false;
    bool camSet_ = false;
    bool snap_ = false;
    bool ghostIn_ = false;
    int taker_ = -1;
    int takerGold_ = 0;
    int takerCream_ = 0;
    int red_ = 0;
    int yel_ = 0;
    int shot_ = 0;
    int thrown_ = -1;
    int think_ = 0;
    int slideFrames_ = 0;
    int curl_ = 1;
    int fanStep_ = -1;
    int hitCool_ = 0;
    int ghostN_ = 0;
    float aim_ = 0;
    float power_ = 0.62f;
    float planSweep_ = 0;
    float osc_ = 0;
    float clock_ = 0;
    float camTop_ = 100;
    float toneT_ = 0;
    float fanT_ = 0;
    float redLie_ = 99;
    float yelLie_ = 99;
    float ghostX_[36]{};
    float ghostY_[36]{};
    float ghostStopY_ = 100;
    Puff puffs_[10]{};
};

}  // namespace curlgold
