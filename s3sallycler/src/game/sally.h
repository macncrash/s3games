#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace sally {

// You have the sally. Clear every heap off the ground before the clock dies.
class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SALLY CLER"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int left() const;
    int clockLeft() const { return clock_; }
    const char* reason() const { return reason_.c_str(); }

private:
    static constexpr int kCols = 4;
    static constexpr int kRows = 3;
    static constexpr int kHeaps = kCols * kRows;
    static constexpr int kWork = 26;
    static constexpr int kWalk = 10;
    static constexpr int kClock = 60 * 16;

    enum class Mode { Title, Play, Over };

    void begin();
    void update();
    void draw();
    void sky();
    void ground();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void blip(int ch, float freq, float vol);
    void figure();
    bool wantClear() const;
    void steer(int& dc, int& dr) const;
    int dirtyCount() const;
    void finish(bool win, const char* why);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool heap_[kHeaps] = {};
    int age_ = 0;
    int clock_ = 0;
    int pc_ = 0;
    int pr_ = 0;
    int fromC_ = 0;
    int fromR_ = 0;
    int toC_ = 0;
    int toR_ = 0;
    int walk_ = 0;
    int work_ = 0;
    int tone_ = 0;
    int sweep_ = 0;
    std::string reason_;
};

}  // namespace sally
