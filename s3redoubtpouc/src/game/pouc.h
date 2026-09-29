// You have the redoubt. Carry the pouch across. Anything else is a loss.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace pouc {

class Game : public gs::Cart {
public:
    const char* title() const override { return "REDOUBT POUCH"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    // 0 title, 1 crossing, 2 pouch is across, 3 pouch lost
    int marker() const;

private:
    enum class Mode { Title, Run, Won, Lost };

    void resetRun();
    void logic();
    void draw();
    void spr(const gs::Image& img, float x, float y, float w, float h, int pal, bool flip = false, int fog = 0);
    void bot(bool& left, bool& right, bool& jump) const;
    void chime(bool win);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int runFrame_ = 0;
    float px_ = 24;
    float py_ = 180;
    float vy_ = 0;
    bool ground_ = true;
    bool face_ = true;
    float guard_ = 160;
    float guardDir_ = 1;
    bool have_ = true;
    float pouchX_ = 24;
    float pouchY_ = 160;
    float pouchVy_ = 0;
};

}  // namespace pouc
