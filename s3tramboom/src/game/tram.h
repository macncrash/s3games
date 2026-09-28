// S3 TRAM BOOM — in the tram, deliver the drive onto the boom.
// Missing the end of the leg fails it.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace tramboom {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TRAM BOOM"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return frames_ / 60.0f; }
    const char* why() const { return why_; }
    // 0 title, 1 on the leg, 2 the boom is in sight, 3 the drive is moving, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Run, Deliver, Won, Lost };

    void beginLeg();
    void update(float dt);
    void draw();
    void sky();
    void bot(bool& accel, bool& brake) const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void text(const std::string& s, float x, float y, float h, int pal, int align = 0);
    float screenX(float world) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* why_ = "";
    float x_ = 36.f;
    float v_ = 0.f;
    float hold_ = 0.f;
    float still_ = 0.f;
    float slide_ = 0.f;
    float t_ = 0.f;
    int frames_ = 0;
    int blink_ = 0;
};

}  // namespace tramboom
