// S3 ROW — five hundred meters. Stay in the lane.
#pragma once
#include "console/system.h"
#include "pictures.h"

namespace row {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 ROW"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int meters() const { return int(dist_); }
    const char* report() const { return report_; }
    // 0 title, 1 racing, 2 finished in the lane, 3 out of the lane
    int marker() const;

private:
    enum class Mode { Title, Race, Pause, Won, Foul };

    struct Wake {
        float course = 0, x = 0, life = 0;
    };
    struct Proj {
        float x = 0, y = 0, ppm = 0;
        bool ok = false;
    };

    void toTitle();
    void begin();
    void update(float dt);
    void finish(bool win);
    void draw();
    void water(float view);
    void bank(float view);
    void field(float along);
    void shellAt(float phase);
    void hud();
    void audio();
    void blip();
    void fanfare(bool win);
    float steerOf() const;
    Proj project(float wx, float ahead) const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0, bool shadow = false);
    void text(int col, int row, const char* s, int pal);
    void textC(int row, const char* s, int pal);
    int shellFrame(float phase) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    float t_ = 0;
    float demo_ = 0;
    float scenery_ = 0;
    float dist_ = 0;
    float x_ = 0;
    float vx_ = 0;
    float v_ = 0;
    float phase_ = 0;
    float race_ = 0;
    float thunk_ = 0;
    Wake wakes_[16]{};
    char report_[160] = {};
};

}  // namespace row
