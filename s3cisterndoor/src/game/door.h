// One cistern. Hold the door for three minutes. Then it is done.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace cistern {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CISTERN DOOR"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return phase_ == Phase::Win || phase_ == Phase::Lose; }
    bool won() const { return phase_ == Phase::Win; }
    int seal() const;
    int heldSec() const;
    // 0 title, 1 holding, 2 ended
    int marker() const;

private:
    enum class Phase { Title, Play, Win, Lose };

    void paint();
    void spr(const gs::Image& img, float x, float y, float w, float h, int pal, bool flip = false);
    void text(const std::string& s, int x, int y, int pal);
    int braceWanted() const;
    int braceHeld() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Phase phase_ = Phase::Title;
    bool bot_ = false;
    int age_ = 0;
    int play_ = 0;
    float open_ = 0.28f;
    int beat_ = -1;
};

}  // namespace cistern
