// S3 THERM — one envelope. The mark, not the trees.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace therm {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 THERM"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* line() const { return report_[0] ? report_ : "S3 THERM  FAIL  NO REPORT"; }
    // 0 title, 1 climb, 2 the tail wind, 3 the mark, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Win, Fail };

    void begin();
    void human(bool& burn, bool& vent);
    void pilot(bool& burn, bool& vent);
    void physics(bool burn, bool vent);
    void succeed();
    void fail(const char* why);
    bool hitTree() const;
    const char* band() const;
    const char* hint() const;
    void audio();
    void camera();
    void draw();
    void toScreen(float wx, float wy, float& sx, float& sy) const;
    void stamp(const gs::Mipped& m, float wx, float wy, float worldW, float worldH, int pal, float ax, float ay,
               int fog = 0, bool shadow = false);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool burn_ = false;
    bool vent_ = false;
    bool drop_ = false;
    const char* why_ = "";
    char report_[160] = {};
    float t_ = 0, time_ = 0, left_ = 0;
    float x_ = 0, y_ = 0, vx_ = 0, vy_ = 0, temp_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 1;
    float shake_ = 0, fanT_ = -1, burst_ = 0;
    int fanStep_ = -1;
};

}  // namespace therm
