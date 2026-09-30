// S3 SCORE SEVEN — a short written score. First reader to seven leaves.
#pragma once
#include <string>

#include "console/gfx.h"
#include "console/system.h"

namespace scoreseven {

enum Pal { PAL_HUD = 0, PAL_WOOD = 1, PAL_PAPER = 2, PAL_INK = 3, PAL_GOLD = 4, PAL_ROOM = 5 };

struct Art {
    gs::Mipped desk, sheet, clef, note, bar, hand, lamp, word;
    int font[96] = {};
};

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SCORE SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int you() const { return you_; }
    int them() const { return them_; }

private:
    enum class Mode { Title, Play, Hold, Over };

    void buildArt();
    void backdrop();
    void update(float dt);
    void draw();
    void hud(int col, int row, const std::string& s);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void blip(float freq);
    void fanfare();
    bool press(gs::Button b) const;
    int lanePressed() const;
    void spawn();
    void hit();
    void miss();
    void rivalPoint();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int you_ = 0;
    int them_ = 0;
    int lane_ = 0;
    int live_ = 0;  // 0 none, 1 flying, 2 struck, 3 missed
    float nx_ = 0;
    float t_ = 0;
    float rival_ = 0;
    float flash_ = 0;
    float msgT_ = 0;
    std::string msg_;
    static constexpr float kLine = 108.f;
    static constexpr float kSpeed = 210.f;
};

}  // namespace scoreseven
