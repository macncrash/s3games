// S3 YARD POUC — at the yard, carry the pouch across. Miss that and the watch is over.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace yardpouc {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 YARD POUC"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    // 0 title, 1 on the yard, 2 the pouch crossed, 3 the watch is over
    int marker() const;
    float heroX() const { return px_; }
    bool carrying() const { return held_; }
    const char* cause() const { return cause_; }
    void dump(const char* where) const;

private:
    enum class Mode { Title, Play, Pause, Won, Lost };

    void begin();
    void finish(bool crossed, const char* why);
    void hit(float fromX);
    bool overPit(float x) const;
    bool cutOn(int i, float t) const;
    float cutOpen(int i, float t) const;
    bool shutterOpen(float* remain) const;
    bool hookDown(float t) const;
    float safeDrop(float prefer) const;
    void bot(bool& left, bool& right, bool& jump, bool& duck);
    void stepPlay(bool left, bool right, bool jump, bool duck);
    void draw();
    void drawWorld(float cam);
    void drawTitle();
    void sky(float cam);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal, int align = 0);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip = false, int fog = 0,
               bool shadow = false);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet, int fog = 0,
             bool shadow = false);
    void blip(float freq, float vol, float hold);
    void serviceAudio();
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
    bool cutWas_[2] = {false, false};
    int face_ = 1;
    int fan_ = -1;
    int lastSec_ = -1;
    const char* cause_ = "";
    float px_ = 0, py_ = 0, vx_ = 0, vy_ = 0;
    float pouchX_ = 0;
    float cam_ = 0, t_ = 0, watchT_ = 0, stepT_ = 0;
    float jumpBuf_ = 0, stun_ = 0, inv_ = 0, lock_ = 0, shake_ = 0;
    float beep_ = 0, fanT_ = 0;
};

}  // namespace yardpouc
