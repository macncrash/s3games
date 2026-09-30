// S3 HELIKILO — finish one kilometer without touching a wheel.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace heli {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HELIKILO"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int meters() const { return meters_; }
    const char* result() const { return result_; }

private:
    enum class Mode { Title, Fly, Fail, Win };
    enum class Kind { Low, High, Tall, Drop };

    struct Wheel {
        float z;
        Kind kind;
    };

    void buildCourse();
    void resetRun();
    void flyLogic(float dt);
    void collide();
    void finishWin();
    void finishFail(const char* why);
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void text(const std::string& s, float x, float y, float scale, int pal);
    float screenY(float alt) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    std::vector<Wheel> wheels_;
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int meters_ = 0;
    int rotor_ = 0;
    char result_[200] = {};

    float t_ = 0;
    float raceT_ = 0;
    float odo_ = 0;
    float speed_ = 0;
    float alt_ = 0;
    float valt_ = 0;
};

}  // namespace heli
