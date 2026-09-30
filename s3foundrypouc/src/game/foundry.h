// S3 FOUNDRY POUC — one foundry. Carry the pouch across. Then it is done.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace foundrypouc {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FOUNDRY POUC"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    // 0 title, 1 on the floor, 2 carrying, 3 the pouch crossed, 4 the watch is over
    int marker() const;
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Play, Pause, Won, Lost };

    void begin();
    void finish(bool crossed, const char* why);
    void bot(bool& left, bool& right, bool& jump, bool& duck);
    void stepPlay(bool left, bool right, bool jump, bool duck);
    void blip(float freq);
    void draw();
    void backdrop();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog = 0);
    void text(const char* s, float x, float y, float scale, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    const gs::Mipped& heroSprite() const;
    bool pipeHits(float x, float top, float feet) const;
    bool ladleDown() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool held_ = false;
    bool onGround_ = true;
    bool duck_ = false;
    int face_ = 1;
    int lives_ = 3;
    int tick_ = 0;
    int stepN_ = 0;
    int inv_ = 0;
    const char* reason_ = "";
    float px_ = 48.f, py_ = 188.f, vx_ = 0.f, vy_ = 0.f;
    float pouchX_ = 150.f;
    float cam_ = 0.f;
    float coyote_ = 0.f;
    float hold_ = 0.f;
    float beep_ = 0.f;
    float ladleY_ = 64.f;
};

}  // namespace foundrypouc
