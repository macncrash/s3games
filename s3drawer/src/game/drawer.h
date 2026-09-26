// S3 DRAWER — close the shop. The drawer matches the tape.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace drawer {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 DRAWER"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int tape(int i) const { return (i >= 0 && i < 3) ? tape_[i] : 0; }
    int night() const { return night_; }

private:
    enum class Mode { Title, Play, Shut, Between, Victory };

    struct Piece {
        Kind kind = Kind::Penny;
        bool belongs = false;
        bool inDrawer = false;
        bool startIn = false;
        float x = 0, y = 0;
    };

    void cacheTapes();
    void deal();
    void layout(bool snap);
    void toggle();
    void tryClose();
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
    bool junkInside() const;
    bool matched() const;
    std::string money(int cents) const;
    std::string statusText() const;
    int statusPal() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int night_ = 0;
    int tape_[3] = {};
    int tapeCents_ = 0;
    int tries_ = 3;
    int sel_ = 0;
    int n_ = 0;
    int t_ = 0;
    int shutT_ = 0;
    int betweenT_ = 0;
    int shake_ = 0;
    int wrong_ = 0;
    int beep_ = 0;
    int cool_ = 0;
    int hold_[4] = {};
    std::string reason_;
    Piece pieces_[16]{};
};

}  // namespace drawer
