// S3 FERRY TURN — make the three turns without tipping.
// Each bend wants a heel through the sector. Past the tip, the ferry rolls.
// Missing the slip at the end fails the leg, turns or not.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace ferryturn {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FERRY TURN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return legT_; }
    const char* why() const { return why_ ? why_ : ""; }
    float x() const { return x_; }
    float lane() const { return lane_; }
    float speed() const { return speed_; }
    float list() const { return list_; }
    int turns() const { return next_; }
    float arc() const { return arc_; }
    // 0 title, 1 the crossing, 2 heeling a turn, 3 running to the slip, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Cross, Pause, Fail, Win };

    struct Wake {
        float x = 0, y = 0, life = 0;
    };

    void showTitle();
    void startRun();
    void pilot(float& rudder, float& throttle) const;
    void physics(float rudder, float throttle);
    void win();
    void fail(const char* why, const char* banner);
    void blip(float freq);
    void audio();
    void draw();
    void water();
    void spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip = false, int fog = 0);
    void text(const char* s, float x, float y, float scale, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    int hullFrame() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool snap_ = true;
    const char* why_ = "";
    const char* banner_ = "";
    const char* toast_ = "";
    float legT_ = 0, anim_ = 0;
    float x_ = 0, lane_ = 0, speed_ = 0, laneV_ = 0, list_ = 0;
    float rudder_ = 0, throttle_ = 0;
    float overL_ = 0, pegT_ = 0, arc_ = 0;
    float camX_ = 0, camY_ = 0, camS_ = 4.f;
    float shx_ = 0, shy_ = 0, shake_ = 0;
    float beep_ = 0, toastT_ = 0, chimeT_ = 0;
    int next_ = 0;
    int chime_ = -1;
    int wakeN_ = 0;
    Wake wakes_[10]{};
};

}  // namespace ferryturn
