// S3 DRAWERMARK — one till. The gold coin is the mark.
// Silver that sums to the same figure only fills the drawer.
// Seating the gold alone and shutting the till finishes the mark.
// That finished mark ends the cartridge. The rest of the night is not the job.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace drawermark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 DRAWERMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finished() const { return finished_; }
    bool onMark() const { return onMark_; }
    bool shut() const { return shut_; }
    int cents() const { return cents_; }
    int mark() const { return kMarkCents; }

private:
    enum class Mode { Title, Play, Shut, Win };

    struct Piece {
        Kind kind = Kind::Gold;
        bool inDrawer = false;
        float x = 0, y = 0;
    };

    void deal();
    void layout(bool snap);
    void toggle();
    void tryShut();
    void botAct();
    void human();
    void nudge(int d);
    void blip(float freq, float vol = 0.04f);
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void box(const gs::Mipped& m, float x, float y, float w, float h, int pal);
    int drawerCents() const;
    bool goldIn() const;
    bool otherIn() const;
    bool junkIn() const;
    bool seated() const;
    std::string money(int cents) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Piece pieces_[8]{};
    int n_ = 0;
    int sel_ = 0;
    int t_ = 0;
    int shutT_ = 0;
    int tries_ = 3;
    int wrong_ = 0;
    int shake_ = 0;
    int beep_ = 0;
    int cool_ = 0;
    int hold_[4] = {};
    int cents_ = 0;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finished_ = false;
    bool onMark_ = false;
    bool shut_ = false;
    std::string reason_;
};

}  // namespace drawermark
