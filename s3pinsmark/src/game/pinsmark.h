// S3 PINSMARK — ten-pin. A strike or a spare opens a mark.
// That mark ends the game only after its bonus balls are rolled and the count closes.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace pinsmark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PINSMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rolling() const { return rolling_; }
    bool marked() const { return marked_; }
    bool strike() const { return strike_; }
    int markFrame() const { return markFrame_; }
    int bonus(int i) const { return (i >= 0 && i < bonusN_) ? bonus_[i] : 0; }

private:
    enum class Mode { Title, Aim, Swing, Roll, Hold, Over, Pause };
    enum class After { None, Second, Fill, Frame, Close };

    struct Pin {
        float x = 0, z = 0, vx = 0, vz = 0;
        float homeX = 0, homeZ = 0;
        int row = 0;
        bool down = false;
        bool swept = false;
        float fall = 0;
    };

    void newGame();
    void resetRack();
    void sweepDead();
    void beginSwing();
    void release();
    void judge();
    void closeMark();
    void openLoss();
    void commit();
    int physics(float dt);
    void draw();
    void project(float x, float z, float& sx, float& sy, float& ppm) const;
    float meter() const;
    float aimX() const;
    float aimTarget() const;
    int standing() const;
    bool lined() const;
    int pose() const;
    void blip(float freq);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog = 0, bool feet = false);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog = 0, bool shadow = false);
    void word(const gs::Image& img, float cx, float cy, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void writeCount(char* dst, int n) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    After after_ = After::None;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rolling_ = false;
    bool marked_ = false;
    bool strike_ = false;
    bool fullRack_ = false;
    bool ballLive_ = false;
    bool gutter_ = false;
    bool pocket_ = false;
    bool sent_ = false;
    bool wasLined_ = false;
    int frame_ = 0;
    int ball_ = 0;
    int bonus_[2] = {};
    int bonusN_ = 0;
    int markFrame_ = 1;
    int armed_ = 10;
    int framePins_ = 0;
    char board_[10] = {};
    float stance_ = 0;
    float swingT_ = 0;
    float clock_ = 0;
    float wait_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    float hook_ = 0;
    float rollT_ = 0;
    float entry_ = 0;
    float ballX_ = 0, ballZ_ = 0, ballVx_ = 0, ballVz_ = 0;
    Pin pin_[10];
};

}  // namespace pinsmark
