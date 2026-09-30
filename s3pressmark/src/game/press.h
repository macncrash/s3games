// S3 PRESSMARK — one letterpress, one impression.
// Set the sheet on the registration mark, lay the ink, and pull when the
// bar sits on the gold line. That finished mark ends the cartridge.
// A short pull or a hard pull leaves the mark open.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace pressmark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PRESSMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finished() const { return finished_; }
    int tries() const { return tries_; }
    int pulls() const { return pulls_; }

private:
    enum class Mode { Title, Set, Ink, Pull, Slam, Win, Lose };

    void buildSound();
    void blip(float freq);
    void fanfare();
    void thunk();
    bool press(gs::Button b) const;
    bool held(gs::Button b) const;
    float steer() const;
    float sweep() const;
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void backdrop();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finished_ = false;
    bool inked_ = false;
    bool locked_ = false;
    int tries_ = 3;
    int pulls_ = 0;
    float t_ = 0;
    float sheet_ = -36.f;
    float roll_ = 0;
    float slam_ = 0;
    float msgT_ = 2.f;
    const char* msg_ = "FINISH THE MARK";
};

}  // namespace pressmark
