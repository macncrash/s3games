// S3 GATE POUC — at the gate, carry the pouch across. Miss that and the watch is over.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace pouc {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 GATE POUC"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    // 0 title, 1 on the road, 2 the pouch crossed, 3 the watch is over
    int marker() const;
    const char* cause() const { return cause_; }
    float heroX() const { return px_; }
    bool carrying() const { return held_; }
    float watch() const { return watchT_; }
    void dump(const char* where) const;

private:
    enum class Mode { Title, Play, Pause, Won, Lost };

    struct Patrol {
        float minX = 0, maxX = 0, speed = 0, endWait = 0, hold = 0, done = 0, need = 0;
        float x = 0, dir = 1, wait = 0;
    };

    void begin();
    void finish(bool crossed, const char* why);
    bool overPit(float x) const;
    bool barDown() const;
    bool handsOpen(float* remain) const;
    float bodyH() const { return duck_ ? 24.f : 46.f; }
    void bot(bool& left, bool& right, bool& jump, bool& duck);
    void stepPlay(bool left, bool right, bool jump, bool duck);
    void hit(float fromX);
    void draw();
    void drawWorld(float cam);
    void drawTitle();
    void sky(float cam);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal, int align = 0);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet, bool shadow = false);
    void blip(float freq, float vol, float hold);
    void serviceAudio();
    const gs::Mipped& heroSprite() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Patrol patrols_[3]{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool held_ = false;
    bool onGround_ = true;
    bool duck_ = false;
    int face_ = 1;
    int commit_ = -1;
    int fan_ = -1;
    int lastSec_ = -1;
    const char* cause_ = "";
    float px_ = 0, py_ = 0, vx_ = 0, vy_ = 0;
    float pouchX_ = 0, pouchVx_ = 0;
    float cam_ = 0, t_ = 0, watchT_ = 0, stepT_ = 0;
    float jumpBuf_ = 0, stun_ = 0, inv_ = 0, lock_ = 0, miss_ = 0, shake_ = 0;
    float beep_ = 0, fanT_ = 0;
};

}  // namespace pouc
