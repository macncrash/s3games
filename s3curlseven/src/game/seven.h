// S3 CURL SEVEN — a short sheet, two rocks a side.
// The closer side counts each stone that bites the house and beats the other side.
// First to seven wins. A count that steps past seven still stands.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace curlseven {

constexpr int kRace = 7;
constexpr int kMaxEnds = 12;

struct Rock {
    float x = 0, y = 0, vx = 0, vy = 0;
    int side = 0;
    int handle = 1;
    bool gone = false;
    bool moving = false;
};

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CURL SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    int you() const { return you_; }
    int them() const { return them_; }
    bool sliding() const { return mode_ == Mode::Slide; }

private:
    enum class Mode { Title, Aim, Slide, Between, Win, Lose, Pause };

    struct Puff {
        float x = 0, y = 0, life = 0;
    };

    bool yourTurn() const;
    void begin();
    void toTitle();
    void updateAim();
    void updateSlide();
    void onRest();
    void scoreEnd();
    void nextEnd();
    void win();
    void lose();
    void planBot(int side);
    void planSmart();
    void predict();
    void launch();
    bool audit();
    void blip(float a, float b, float c, float hold);
    void decay();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip = false, bool shadow = false);
    void sprM(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudR(int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void lamps(float x, int score, int pal);
    void stoneAt(float wx, float wy, int pal, int handle);
    void shadeAt(float wx, float wy);
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    Rock rock_[4]{};
    Puff puff_[8]{};
    float pathX_[16]{};
    float pathY_[16]{};
    int pathN_ = 0;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool picked_ = false;
    bool aiThrow_ = false;
    bool ghostGone_ = false;
    bool youHammer_ = true;
    int you_ = 0;
    int them_ = 0;
    int ends_ = 0;
    int end_ = 1;
    int n_ = 0;
    int thrown_ = -1;
    int took_ = 0;
    int handle_ = 1;
    int humanHandle_ = 1;
    int think_ = 0;
    int hold_ = 0;
    int slideFrames_ = 0;
    int hitCool_ = 0;
    float aim_ = 0;
    float weight_ = 0.58f;
    float humanAim_ = 0;
    float humanWeight_ = 0.58f;
    float ghostX_ = 0;
    float ghostY_ = 0;
    float clock_ = 0;
    float toneT_ = 0;
};

}  // namespace curlseven
