// S3 CURL — four ends. Closest rock to the button takes the end.
#pragma once

#include <string>

#include "console/system.h"
#include "game/art.h"
#include "game/ice.h"

namespace curl {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CURL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int red() const { return red_; }
    int yel() const { return yel_; }
    int ends() const { return endsDone_; }

private:
    enum class Mode { Title, Aim, Slide, Between, Result, Pause };

    void beginMatch();
    void enterAim();
    void launch(int side, bool ai);
    void onRest();
    void scoreEnd();
    void finishMatch();
    Shot choose(int side) const;
    void predict(int side, float sweep);
    void updateAim();
    void updateSlide();
    float focusY() const;
    void seekCam(float focus, float bias, bool snap);
    void tickAudio();
    void blip(float freq, float vol, float hold);
    void draw();
    void drawRock(float x, float y, int side, bool shadow);
    void blit(const gs::Image& img, float cx, float cy, int w, int h, int pal, int fog = 0);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
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
    int red_ = 0;
    int yel_ = 0;
    int end_ = 1;
    int endsDone_ = 0;
    int shot_ = 0;
    int thrown_ = -1;
    int think_ = 0;
    int slideFrames_ = 0;
    int hold_ = 0;
    int curl_ = 1;
    int lastTaker_ = -1;
    int fanStep_ = -1;
    float fanT_ = 0;
    float aim_ = 0;
    float power_ = 0.55f;
    float planSweep_ = 0;
    float osc_ = 0;
    float t_ = 0;
    float camTop_ = 100;
    float toneT_ = 0;
    float redDist_ = 99;
    float yelDist_ = 99;
    float ghostX_[40]{};
    float ghostY_[40]{};
    int ghostN_ = 0;
    float ghostStopY_ = 100;
    bool ghostIn_ = false;
    struct Puff {
        float x, y, life;
    };
    Puff puffs_[12]{};
    int hitCool_ = 0;
};

}  // namespace curl
