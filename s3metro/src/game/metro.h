// S3 METRO — stop the cab mark in the box. Past the far edge is a fail.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace metro {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 METRO"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int marker() const;
    std::string report() const;

private:
    enum class Mode { Title, Brief, Run, Dock, Overshoot, Victory, Over, Pause };

    void startLine();
    void titlePose();
    void armStation();
    void update();
    int botCmd() const;
    void physics(float power, float brake);
    void enterDock();
    void enterOvershoot();
    void sound();
    void draw();
    void backdrop();
    void banner();
    void blit(const gs::Mipped& m, float left, float top, int pal, bool flip = false);
    void text(const std::string& s, float x, float y, float scale, int pal, int align = 0);
    void hud(int col, int row, const std::string& s, int pal);
    float sx(float meters, float par = 1.f) const;
    bool goPressed() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Run;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool braking_ = false;
    bool powering_ = false;
    bool wasIn_ = false;
    int station_ = 0;
    int score_ = 0;
    int tries_ = 4;
    int hold_ = 0;
    int bannerT_ = 0;
    int runT_ = 0;
    int lastAward_ = 0;
    int errCm_ = 0;
    int theme_ = 0;
    float nose_ = 0, speed_ = 0, boxL_ = 0, boxR_ = 0, boxW_ = 0;
    float t_ = 0, shake_ = 0, shakeX_ = 0, shakeY_ = 0, joint_ = 0, odo_ = 0;
    const char* reason_ = "unfinished";
};

}  // namespace metro
