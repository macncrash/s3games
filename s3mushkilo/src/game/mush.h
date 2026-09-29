// S3 MUSHKILO — finish one kilometer. Do not touch a wheel.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace mush {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MUSHKILO"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int meters() const { return meters_; }

private:
    enum class Mode { Title, Run, Fail, Victory };

    struct Wheel {
        int lane;  // 0 left, 1 mid, 2 right
        float z;
        int kind;  // 0 loose wheel, 1 cart, 2 bike
    };

    void beginRun();
    void layCourse();
    void update(float dt);
    void steerBot();
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet);
    float rnd();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int meters_ = 0;
    float dist_ = 0;
    float lane_ = 1;
    int want_ = 1;
    float t_ = 0;
    float speed_ = 14;
    uint32_t rng_ = 0x4D5348u;
    std::vector<Wheel> wheels_;
    uint16_t skyTop_ = 0, skyHor_ = 0;
};

}  // namespace mush
