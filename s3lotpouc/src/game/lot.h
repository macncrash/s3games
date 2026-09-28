// S3 LOT POUC — one lot. Carry the pouch across. Then it is done.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace lot {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LOT POUC"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int marker() const;
    bool carrying() const { return held_; }
    float heroX() const { return px_; }

private:
    enum class Mode { Title, Play, Won, Lost };

    struct Car {
        float x = 0, speed = 0, w = 70;
        int kind = 0;
    };

    void begin();
    void finish(bool crossed);
    void bot(bool& left, bool& right, bool& jump);
    void stepPlay(bool left, bool right, bool jump);
    void draw();
    void hud(int col, int row, const char* s);
    void spr(const gs::Mipped& m, float cx, float footY, float h, int pal, bool flip);
    const gs::Mipped& heroSprite() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Car cars_[4]{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool held_ = false;
    bool onGround_ = true;
    int face_ = 1;
    int lives_ = 3;
    int runTick_ = 0;
    float px_ = 48, py_ = 0, vx_ = 0, vy_ = 0;
    float pouchX_ = 180;
    float cam_ = 0, t_ = 0, clock_ = 0;
    float inv_ = 0;
    int titleWait_ = 0;
};

}  // namespace lot
