// S3 CLOCK SEVEN — one clock, two keepers. First true hour to seven.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace c7 {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CLOCK SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int you() const { return you_; }
    int them() const { return them_; }

private:
    enum class Mode { Title, Play, Chime, Win, Lose };

    void begin();
    void deal();
    void turn(int dir);
    void haul();
    void act();
    void human(const gs::Pad& pad);
    void tickSecond();
    void finishChime();
    void draw();
    void sky();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void image(const gs::Image& img, float cx, float cy, int pal, int w = 0, int h = 0);
    int hourStep() const;
    bool aligned() const;
    void blip();
    void silence();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool yours_ = true;
    bool pause_ = false;
    int hour_ = 6;
    int minute_ = 2;
    int second_ = 4;
    int target_ = 7;
    int grip_ = 0;
    int you_ = 0;
    int them_ = 0;
    int foul_ = 0;
    int rep_ = 0;
    int secAcc_ = 0;
    int chime_ = 0;
    int ropeY_ = 0;
    int swing_ = 0;
    int scored_ = 0;
};

}  // namespace c7
