// S3 RAIL SLIP — berth in each slip before the tide turns.
// Passing the far end of a slip without a full stop fails the leg.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace railslip {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 RAIL SLIP"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return time_; }
    float tideLeft() const { return tide_; }
    int slips() const { return berthed_; }
    const char* why() const { return why_; }
    // 0 title, 1 running a leg, 2 just berthed, 3 ended
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    void begin();
    void logic();
    void pilot(float& throttle, float& brake);
    void fail(const char* why);
    void berth();
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool feet = false, bool flip = false);
    void text(const char* s, float x, float y, float scale, int pal, int align = -1);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* why_ = "";
    float time_ = 0;
    float tide_ = 0;
    float carX_ = 0;
    float speed_ = 0;
    float cam_ = 0;
    float hold_ = 0;
    float flash_ = 0;
    int berthed_ = 0;
    int leg_ = 0;
};

}  // namespace railslip
