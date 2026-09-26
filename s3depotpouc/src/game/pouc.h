// S3 DEPOT POUC — one depot. Carry the pouch across. Then it is done.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace pouc {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 DEPOT POUC"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    // 0 title, 1 crossing without the pouch, 2 carrying, 3 done, 4 not done
    int marker() const;
    const char* reason() const { return reason_; }
    float heroX() const { return px_; }
    bool carrying() const { return held_; }
    void dump(const char* where) const;

private:
    enum class Mode { Title, Play, Pause, Won, Lost };

    void begin();
    void finish(bool crossed, const char* why);
    void tickGates();
    void syncLifts();
    bool spanOn(float period, float openFor, float phase) const;
    float openRemain(float period, float openFor, float phase, float lift) const;
    bool hole(float x) const;
    bool doorBlocks(float foot) const;
    float bodyH() const { return duck_ ? 22.f : 50.f; }
    bool runPit(float a, float b, bool& right, bool& jump) const;
    bool cross(float stopX, float clearX, float remain, bool& right) const;
    void bot(bool& left, bool& right, bool& jump, bool& duck);
    void stepPlay(bool left, bool right, bool jump, bool duck);
    void hit();
    const char* hint() const;
    int hintPal() const;
    void draw();
    void drawWorld(float cam);
    void drawTitle();
    void backdrop(float cam);
    void audio();
    void blip(float freq);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal, int align = 0);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet, bool shadow = false);
    const gs::Mipped& heroSprite() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool held_ = false;
    bool onGround_ = true;
    bool duck_ = false;
    bool jumped_ = false;
    bool pouchOnDesk_ = true;
    int face_ = 1;
    int fan_ = -1;
    const char* reason_ = "";
    float px_ = 0, py_ = 0, vx_ = 0, vy_ = 0;
    float pouchX_ = 0;
    float cam_ = 0, t_ = 0, stepT_ = 0;
    float coyote_ = 0, stun_ = 0, inv_ = 0, shake_ = 0;
    float beep_ = 0, fanT_ = 0;
    float bridgeLift_ = 0, doorLift_ = 0, hookLift_ = 0;
};

}  // namespace pouc
