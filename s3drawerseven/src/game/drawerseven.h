// S3 DRAWER SEVEN — a short till. Two clerks. First true close to seven.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace drawerseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 DRAWER SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int you() const { return you_; }
    int them() const { return them_; }

private:
    enum class Mode { Title, Play, Shut, Win, Lose };
    enum class Kind { Penny, Nickel, Button };

    struct Piece {
        Kind kind = Kind::Penny;
        bool belongs = false;
        bool inDrawer = false;
    };

    void begin();
    void deal();
    void toggle();
    void tryClose();
    void afterShut();
    void botAct();
    void human();
    void nudge(int d);
    void blip(float freq);
    int drawerCents() const;
    bool junkIn() const;
    bool matched() const;
    void sky();
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Image& img, float cx, float cy, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Piece pieces_[6]{};
    int n_ = 0;
    int sel_ = 0;
    int you_ = 0;
    int them_ = 0;
    int shutT_ = 0;
    int cool_ = 0;
    int shake_ = 0;
    int hold_[4] = {};
    bool yours_ = true;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
};

}  // namespace drawerseven
