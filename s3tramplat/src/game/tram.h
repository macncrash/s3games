// S3 TRAM PLAT — take the tram and stop level with the platform.
// The doors sit on the stripe and the floor kneels flush with the deck.
// The clock is the other crew: they take the stand when it runs out.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace tramplat {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TRAM PLAT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return time_; }
    const char* why() const { return why_ ? why_ : ""; }
    float door() const { return door_; }
    float floor() const { return floor_; }
    float speed() const { return speed_; }
    // 0 title, 1 the approach, 2 alongside the platform, 3 holding level, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    void showTitle();
    void startRun();
    void pilot(float& power, float& brake, float& kneel) const;
    void physics(float power, float brake, float kneel);
    void win();
    void fail(const char* why);
    void blip(float freq);
    void draw();
    void sky();
    void spr(const gs::Mipped& m, float cx, float feet, float ht, int pal, bool flip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* why_ = "";
    float time_ = 0;
    float door_ = 0;    // metres, centre of the doors
    float floor_ = 0;   // floor height above the rail
    float speed_ = 0;
    float power_ = 0, brake_ = 0, kneel_ = 0;
    float hold_ = 0;
    float dwell_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    int chime_ = -1;
    float chimeT_ = 0;
};

}  // namespace tramplat
