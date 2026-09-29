// S3 PALISADE POUC — one palisade. Carry the pouch across. Then it is done.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace palisadepouc {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PALISADE POUC"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 on the near bank, 2 carrying, 3 the pouch is across, 4 a loss
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Won, Lost };

    void begin();
    void finish(bool crossed, const char* why);
    bool barDown() const;
    bool overDitch(float x) const;
    void bot(bool& left, bool& right, bool& jump);
    void stepPlay(bool left, bool right, bool jump);
    void blip(float freq);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip);
    void text(const char* s, float x, float y, int pal);
    void backdrop();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool held_ = false;
    bool onGround_ = true;
    int face_ = 1;
    const char* reason_ = "";
    float px_ = 48.f, py_ = 0.f, vx_ = 0.f, vy_ = 0.f;
    float pouchX_ = 110.f;
    float cam_ = 0.f, t_ = 0.f, stepT_ = 0.f;
    float coyote_ = 0.f, stun_ = 0.f;
    float beep_ = 0.f;
};

}  // namespace palisadepouc
