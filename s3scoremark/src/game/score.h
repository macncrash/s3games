// S3 SCOREMARK — play the written score. The phrase ends only when the
// rehearsal mark is finished on the final bar. An open mark is still short.
#pragma once
#include <string>

#include "console/gfx.h"
#include "console/system.h"

namespace score {

enum Pal { PAL_HUD = 0, PAL_WOOD = 1, PAL_PAPER = 2, PAL_INK = 3, PAL_GOLD = 4, PAL_ROOM = 5 };

struct Art {
    gs::Mipped desk, sheet, clef, note, rest, bar, mark, lamp, hand;
    int font[96] = {};
};

struct Note {
    int lane = 0;
    float beat = 0;
    int state = 0;  // 0 waiting, 1 struck, 2 missed
};

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SCOREMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int struck() const { return struck_; }
    int tries() const { return tries_; }

private:
    enum class Mode { Title, Play, Seal, Over };

    void buildArt();
    void backdrop();
    void update(float dt);
    void draw();
    void hud(int col, int row, const std::string& s);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void blip(float freq);
    void fanfare();
    bool press(gs::Button b) const;
    float noteX(const Note& n) const;
    void strike(Note& n);
    void miss(Note& n);
    int lanePressed() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int tries_ = 3;
    int struck_ = 0;
    float t_ = 0;
    float song_ = 0;
    float seal_ = 0;
    float msgT_ = 0;
    std::string msg_;
    Note notes_[8]{};
    static constexpr int kNotes = 8;
    static constexpr float kLine = 92.f;
    static constexpr float kSpeed = 72.f;
};

}  // namespace score
