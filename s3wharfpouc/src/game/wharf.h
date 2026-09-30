// S3 WHARF POUC — you have the wharf. Carry the pouch across. Anything else is a loss.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace wharf {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 WHARF POUC"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    // 0 title, 1 on the wharf, 2 carrying, 3 the pouch crossed, 4 a loss
    int marker() const;
    const char* reason() const { return reason_; }
    float heroX() const { return px_; }
    bool carrying() const { return held_; }
    void dump(const char* where) const;

private:
    enum class Mode { Title, Play, Pause, Won, Lost };

    void begin();
    void finish(bool crossed, const char* why);
    bool gap(float x) const;
    bool hookLow() const;
    float barrelX() const;
    float bodyH() const { return duck_ ? 24.f : 52.f; }
    void bot(bool& left, bool& right, bool& jump, bool& duck);
    void stepPlay(bool left, bool right, bool jump, bool duck);
    void knock(float fromX);
    const char* hint() const;
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
    int face_ = 1;
    int fan_ = -1;
    const char* reason_ = "";
    float px_ = 0, py_ = 0, vx_ = 0, vy_ = 0;
    float pouchX_ = 0;
    float cam_ = 0, t_ = 0, watch_ = 0, stepT_ = 0;
    float coyote_ = 0, stun_ = 0, inv_ = 0, shake_ = 0;
    float beep_ = 0, fanT_ = 0;
};

}  // namespace wharf
