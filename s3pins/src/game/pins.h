// S3 PINS — ten frames. The first strike or spare is a mark, and the mark ends it.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace pins {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PINS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int frameNo() const { return markFrame_; }
    int pinsDown() const { return pinsDown_; }
    const char* kind() const { return kind_; }

private:
    enum class Mode { Title, Approach, Swing, Roll, Hold, Mark, Over, Pause };

    struct Pin {
        float x = 0, z = 0, vx = 0, vz = 0;
        float homeX = 0, homeZ = 0;
        bool down = false;
        float fall = 0;
    };

    void newGame();
    void resetRack();
    void beginSwing();
    void release();
    void judge();
    void finishMark(bool strike);
    void physics(float dt);
    void draw();
    void project(float x, float z, float& sx, float& sy, float& ppm) const;
    float meter() const;
    float aimX() const;
    float aimTarget() const;
    int standing() const;
    bool onPocket() const;
    int pose() const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog = 0, bool feet = false);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog = 0, bool shadow = false);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void blip(float freq);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Approach;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* kind_ = "open";
    int markFrame_ = 1;
    int pinsDown_ = 0;
    int frame_ = 0;
    int ball_ = 0;
    char board_[10] = {};
    float stance_ = 0;
    float swingT_ = 0;
    float clock_ = 0;
    float wait_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    float hook_ = 0;
    float rollT_ = 0;
    float rippleT_ = -1;
    float markT_ = 0;
    float entry_ = 0;
    bool entryTaken_ = false;
    bool pocket_ = false;
    bool gutter_ = false;
    bool gutterSnd_ = false;
    bool ballLive_ = false;
    bool wasPocket_ = false;
    int crashed_ = 0;
    float ballX_ = 0, ballZ_ = 0, ballVx_ = 0, ballVz_ = 0;
    Pin pin_[10];
};

}  // namespace pins
