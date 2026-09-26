// S3 FUNICULAR — two counterbalanced cars. Stop the red floor level with the platform.
#pragma once
#include <string>

#include "art.h"
#include "console/system.h"

namespace funicular {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FUNICULAR"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* report() const { return report_; }
    int cleared() const { return cleared_; }
    // 0 title, 1 hauling, 2 on the platform, 3 doors, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Run, Result };

    struct Stop {
        const char* name;
        float at;
        float tol;
    };
    static constexpr int kStopsN = 3;
    static constexpr Stop kStops[3] = {
        {"ORCHARD", 26.f, 0.22f},
        {"PASS", 52.f, 0.16f},
        {"CREST", 90.f, 0.12f},
    };
    struct Load {
        float red, green;
    };
    static constexpr Load kLoad[3] = {
        {18.f, 10.f},
        {10.f, 18.f},
        {15.f, 12.5f},
    };

    const Stop& stop() const;
    float coastA() const;
    void begin();
    void applyLoad();
    void bot(bool& wind, bool& brake, bool& ease);
    void physics(bool wind, bool brake, bool ease);
    void arrive();
    void win();
    void fail(const char* why);
    void audio(bool wind, bool brake);
    void draw(bool wind, bool brake, bool ease);
    void stamp(const gs::Image& img, float ax, float ay, float x, float y, float scale, int pal, int fog = 0, bool shadow = false);
    void stamp(const gs::Mipped& img, float ax, float ay, float x, float y, float scale, int pal, int fog = 0);
    void hudText(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool pendingWin_ = false;
    int cleared_ = 0;
    int bell_ = 0;
    int post_ = 0;
    float s_ = 8.f;
    float v_ = 0.f;
    float massA_ = 18.f;
    float massB_ = 10.f;
    float held_ = 0.f;
    float dwell_ = 0.f;
    float time_ = 0.f;
    float t_ = 0.f;
    float cable_ = 0.f;
    char made_[16] = {};
    char banner_[48] = {};
    char report_[180] = {};
};

}  // namespace funicular
