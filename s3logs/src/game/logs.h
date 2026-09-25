// S3 LOGS — herd a river drive of sticks into the sorting boom.
#pragma once
#include <array>
#include <vector>

#include "console/system.h"
#include "pictures.h"

namespace logs {

constexpr int LOG_N = 9;
constexpr int NEED = 6;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LOGS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* report() const { return report_; }
    int marker() const;
    int meters() const;

private:
    enum class Mode { Title, Play, Pause, Won, Lost };
    enum class St { Afloat, In, Lost };
    enum class Why { None, Beach, Deadhead, Snag, Jam, Stove };

    struct Stick {
        float x = 0, z = 0, vx = 0;
        float knock = 0;
        St st = St::Afloat;
        int kind = 0;
    };
    struct Hazard {
        float z = 0, off = 0;
        int kind = 0;  // 0 rock, 1 sweeper
    };
    struct Pop {
        float x = 0, z = 0, life = 0;
        int kind = 0;
    };
    struct Proj {
        float x = 0, y = 0, ppm = 0;
        bool ok = false;
    };

    void toTitle();
    void begin();
    void update(float dt);
    void assist();
    void finish(bool win, Why why);
    void draw();
    void river();
    void skyDress();
    void boatAt(float wx, float wz, bool pole, bool flip);
    void world();
    void hud();
    void audio();
    void text(int col, int row, const char* s, int pal);
    void textC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0, bool shadow = false);
    Proj project(float wx, float wz) const;
    int fogFor(float wz) const;
    void splash(float x, float z, int kind);
    int afloat() const;
    int delivered() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool pole_ = false;
    bool ease_ = false;
    bool boost_ = false;
    int hull_ = 3;
    Why why_ = Why::None;
    float steer_ = 0;
    float t_ = 0;
    float race_ = 0;
    float scenery_ = 24;
    float cam_ = 0;
    float boatX_ = 0, boatZ_ = 0, vx_ = 0;
    float hit_ = 0;
    float flashT_ = 0;
    float knock_ = 0;
    float chime_ = 0;
    float chimeF_ = 660;
    float failTone_ = 0;
    int fan_ = -2;
    float fanT_ = 0;
    const char* flash_ = nullptr;
    char report_[160] = "";
    std::array<Stick, LOG_N> logs_{};
    std::vector<Hazard> hazards_;
    std::array<Pop, 12> pops_{};
};

}  // namespace logs
