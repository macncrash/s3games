// S3 MOSAIC MARK — lay tesserae until the mark is finished.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace mark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MOSAIC MARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int stamps() const { return stamps_; }
    // 0 title, 1 laying, 2 the mark just sealed, 3 finished
    int marker() const;

private:
    enum class Phase { Title, Lay, Seal, Victory };

    void update();
    void draw();
    void backdrop();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Image& img, int x, int y, int w, int h);
    void blip(float freq);
    bool done() const;
    bool confirm() const;
    void stampHere();
    const char* inkName() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Phase phase_ = Phase::Title;
    int board_[CELLS] = {};
    int cx_ = 3;
    int cy_ = 3;
    int ink_ = 1;
    int stamps_ = 0;
    int t_ = 0;
    int beep_ = 0;
    bool bot_ = false;
    bool won_ = false;
    bool over_ = false;
    bool paused_ = false;
};

}  // namespace mark
