// S3 PAWNBELL — a short pawn, three tries, one bell.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace pawnbell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PAWNBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    int dead() const { return dead_; }
    const char* why() const { return why_; }

private:
    enum class Mode { Title, Play, Dead, Ring, Over };

    void resetTry();
    void kill(const char* why);
    void ring();
    void botThink();
    void physics();
    void backdrop();
    void layHall();
    void draw();
    void stamp(const gs::Image& img, float x, float y, float w, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    bool onFloor(float x) const;
    bool grounded() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    int dead_ = 0;
    int t_ = 0;
    int wait_ = 0;
    const char* why_ = "";
    float px_ = 28.f;
    float py_ = 176.f;
    float vx_ = 0.f;
    float vy_ = 0.f;
    bool faceR_ = true;
    float bobPh_ = 0.f;
};

}  // namespace pawnbell
