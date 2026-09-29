// S3 CAUSEWAY POUC — you have the causeway. Carry the pouch across.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace cwpouc {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CAUSEWAY POUC"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool carrying() const { return held_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 on the causeway, 2 carrying, 3 the pouch crossed, 4 a loss
    int marker() const;

private:
    enum class Mode { Title, Play, Won, Lost };
    void begin();
    void finish(bool crossed, const char* why);
    bool solidAt(float x) const;
    bool jumpEdge(bool right) const;
    void botIntent(bool& left, bool& right, bool& jump, bool& go);
    void stepPlay(bool left, bool right, bool jump);
    void draw();
    void backdrop();
    void text(const char* s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool held_ = false;
    bool onGround_ = true;
    int face_ = 1;
    const char* reason_ = "THE POUCH DID NOT CROSS";
    float px_ = 48.f;
    float py_ = 0.f;
    float vx_ = 0.f;
    float vy_ = 0.f;
    float pouchX_ = 220.f;
    float cam_ = 0.f;
    float t_ = 0.f;
    float playT_ = 0.f;
};

}  // namespace cwpouc
