// S3 CABLE — a hill tram. Stop the door level with the mark.
#pragma once
#include <vector>

#include "console/system.h"
#include "art.h"

namespace cable {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CABLE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* report() const { return report_; }
    int cleared() const { return cleared_; }
    // 0 title, 1 hauling, 2 on the mark, 3 doors open, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Run, Result };

    struct Stop {
        const char* name;
        float at;
        float tol;
        int face;
    };
    static constexpr int kStopN = 3;
    static constexpr Stop kStops[3] = {
        {"WHARF", 32.f, 0.50f, 0},
        {"TERRACE", 70.f, 0.36f, 1},
        {"CROWN", 112.f, 0.26f, 2},
    };
    struct Prop {
        float s;
        float lateral;
        float h;
        int kind;
        bool flip;
    };

    void begin();
    void scatter();
    void control(bool& grip, bool& brake);
    void physics(bool grip, bool brake);
    void arrive();
    void win();
    void fail(const char* why);
    void audio();
    void draw();
    void sky();
    void project(float s, float lateral, float& x, float& y) const;
    void stamp(const gs::Mipped& m, float ax, float ay, float sx, float sy, float scale, int pal, int fog = 0, bool flip = false);
    void ui(const gs::Image& img, float x, float y, int pal = PAL_HUD);
    void hudText(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    const Stop& stop() const;
    float gradeAt(float s) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool grip_ = false;
    bool brake_ = false;
    bool gripped_ = false;
    int cleared_ = 0;
    int bell_ = 0;
    float s_ = 0, v_ = 0, cam_ = 0;
    float held_ = 0, dwell_ = 0, time_ = 0, t_ = 0;
    float viewCam_ = 0;
    char made_[16] = {};
    char banner_[48] = {};
    char report_[180] = {};
    std::vector<Prop> props_;
};

}  // namespace cable
