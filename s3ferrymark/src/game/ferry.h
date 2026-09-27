// S3 FERRY MARK — the ferry has one job: set down on the mark.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace fmark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FERRY MARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return time_; }
    const char* why() const { return why_; }
    float x() const { return x_; }
    float y() const { return y_; }
    float heading() const { return hdg_; }
    float speed() const { return spd_; }
    // 0 title, 1 on the way, 2 over the paint, 3 settling, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    void showTitle();
    void startRun();
    void pilot(float& thrust, float& rudder) const;
    void physics(float thrust, float rudder);
    void win();
    void fail(const char* why, const char* banner);
    bool onMark() const;
    void blip(float freq);
    void draw();
    void sky();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip = false);
    void sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* why_ = "";
    const char* banner_ = "";
    int chime_ = -1;
    float time_ = 0;
    float x_ = 0, y_ = 0, hdg_ = 0;
    float surge_ = 0, sway_ = 0, yaw_ = 0;
    float spd_ = 0;
    float hold_ = 0;
    float idle_ = 0;
    float camX_ = 0, camY_ = 0, camS_ = 6;
    float chimeT_ = 0;
    float beep_ = 0;
    float thrustIn_ = 0;
};

}  // namespace fmark
