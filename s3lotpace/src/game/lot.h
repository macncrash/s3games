// S3 LOTPACE — the vacant lot. Wait for the third pace, then fire.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace lot {

// Footfalls, in frames from the start of the walk. The shot is legal only
// inside the window that opens on the third pace.
constexpr int kPaceAt[3] = {80, 160, 240};
constexpr int kWindow = 36;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LOTPACE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int pace() const { return pace_; }

private:
    enum class Mode { Title, Walk, Win, Lose };

    void begin();
    void paintSky();
    void spr(const gs::Image& img, int x, int y, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int pace_ = 0;
    int walk_ = 0;
    int since_ = 0;
    int dust_ = 0;
    int flash_ = 0;
    const char* why_ = "";
};

}  // namespace lot
