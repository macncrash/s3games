#pragma once

#include "console/system.h"
#include "game/art.h"

namespace solitaire {

enum class Mode { Title, Play, Win };

class Game : public gs::Cart {
public:
    const char* title() const override { return "SOLITAIRE MARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool won() const { return won_; }
    bool over() const { return mode_ == Mode::Win; }
    bool finished() const { return finished_; }
    int placed() const { return placed_; }

private:
    void start();
    void playAt(int slot);
    void stepBot();
    void draw(gs::System& sys);
    void text(gs::VDP& vdp, const char* s, int x, int y, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool won_ = false;
    bool finished_ = false;
    int rank_[kCards] = {};
    bool taken_[kCards] = {};
    int cursor_ = 0;
    int need_ = 1;
    int placed_ = 0;
    int hold_ = 0;
    int botWait_ = 0;
    int flash_ = 0;
};

}  // namespace solitaire
