// S3 SHUFFLE SEVEN — a short deck shuffle. First to seven.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"

namespace shuffle {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SHUFFLE SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int you() const { return you_; }
    int house() const { return house_; }

private:
    struct Disc {
        float x = 0, y = 0, vx = 0, vy = 0;
        int side = 0;  // 0 you, 1 house
        bool on = true;
    };

    void buildArt();
    void newRack();
    void launch(int side, float x, float power);
    void physics(std::vector<Disc>& d, int steps);
    bool resting(const std::vector<Disc>& d) const;
    int zoneAt(float y) const;
    void scoreRack();
    bool planShot(int side, float targetY, float& x, float& power);
    void draw();
    void blitText(const std::string& s, float x, float y, int pal);
    void tone(float freq, float vol);

    gs::System* sys_ = nullptr;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int you_ = 0;
    int house_ = 0;
    int rack_ = 1;
    int thrown_ = 0;
    int wait_ = 0;
    int banner_ = 0;
    int lastPts_[2] = {};
    float aim_ = 160;
    float power_ = 0.62f;
    float beep_ = 0;
    enum class Mode { Title, Play, Banner, End } mode_ = Mode::Title;

    std::vector<Disc> discs_;
    gs::Image board_{};
    gs::Image puck_[2]{};
    gs::Image mark_{};
    gs::Image chip_{};
    gs::Image glyphs_[96]{};
};

}  // namespace shuffle
