// S3 CLOCK — three hands. The hour has to chime.
// Grip one hand at a time, turn it, and haul the rope only on a true hour.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace clk {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CLOCK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int struck() const { return target_; }
    int ropes() const { return ropes_; }

private:
    enum class Mode { Title, Play, Chime, Won, Dead };

    void begin();
    void layFixed();
    void layRandom();
    void botAct();
    void human(const gs::Pad& pad);
    void turn(int dir);
    void haul();
    void advanceSecond();
    void updateChime();
    void draw();
    void sky();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void banner(const std::string& s, float y, int pal);
    void image(const gs::Image& img, float cx, float cy, int pal);
    void axle(const gs::Image& img, int pal);
    int hourStep() const;
    bool aligned() const;
    void blip();
    void tock(bool hour);
    void silenceTicks();
    uint32_t rnd();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool opened_ = false;
    bool over_ = false;
    bool won_ = false;
    bool pause_ = false;
    int hour_ = 2;
    int minute_ = 41;
    int second_ = 17;
    int target_ = 7;
    int grip_ = 0;
    int ropes_ = 3;
    int dwell_ = 0;
    int secAcc_ = 0;
    int foul_ = 0;
    int playFrame_ = 0;
    int rep_ = 0;
    int strikesLeft_ = 0;
    int strikeWait_ = 0;
    int chimeHold_ = 0;
    int bellSwing_ = 0;
    int ropeY_ = 0;
    int tockLeft_ = 0;
    int blipLeft_ = 0;
    uint32_t rng_ = 0xC10CCu;
};

}  // namespace clk
