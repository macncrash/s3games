// S3 BUS LANE — the bus stays in the lane for the whole leg.
#pragma once
#include "art.h"
#include "console/system.h"

namespace buslane {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BUS LANE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* why() const { return why_ ? why_ : ""; }
    float seconds() const { return race_; }
    float lateral() const { return x_; }
    // 0 title, 1 in the lane, 2 on the edge, 3 the depot is ahead, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Win, Fail };

    struct Proj {
        float x = 0, y = 0, ppm = 0;
        int fog = 0;
        bool ok = false;
    };

    void toTitle();
    void begin();
    void update(float dt);
    void finish(bool good);
    void audio();
    void draw();
    void road(float view);
    void street(float along);
    void busAt();
    void text(int col, int row, const char* s, int pal);
    void textC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0,
             bool shadow = false);
    Proj project(float wx, float ahead) const;
    float bendAt(float s) const;
    float shove(float d) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* why_ = "";
    float t_ = 0, race_ = 0, dist_ = 0, x_ = 0, vx_ = 0, scenery_ = 0;
    float hum_ = 0;
};

}  // namespace buslane
