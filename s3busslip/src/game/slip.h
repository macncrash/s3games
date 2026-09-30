// S3 BUS SLIP — berth the bus in the slip before the tide turns.
#pragma once
#include "console/system.h"
#include "art.h"

namespace slip {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BUS SLIP"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* why() const { return why_ ? why_ : ""; }
    float x() const { return x_; }
    float speed() const { return vel_; }
    float tide() const { return tide_; }

private:
    enum class Mode { Title, Play, Win, Fail };

    void begin();
    void toTitle();
    void step();
    void pilot(float& thrust) const;
    void controls(float& thrust);
    void audio();
    void draw();
    void sky();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void text(const char* s, float x, float y, float scale, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* why_ = "";
    float t_ = 0;
    float playT_ = 0;
    float x_ = 0;
    float vel_ = 0;
    float tide_ = 0;
    float hold_ = 0;
    float cam_ = 0;
    float wheel_ = 0;
    float shake_ = 0;
    float blip_ = 0;
    bool onSlip_ = false;
};

}  // namespace slip
