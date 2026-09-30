// S3 BUSTURN — three city bends on the S3-16. Tip the bus and the other crew keeps the clock.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace busturn {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BUSTURN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int turns() const { return turns_; }
    float crewLeft() const { return crewLeft_; }
    float raceTime() const { return race_; }
    // 0 title, 1 rolling, 2 a bend is under the wheels, 3 depot, 4 tipped or beaten
    int marker() const;

private:
    enum class Mode { Title, Race, Win, Tip, Late };

    struct Corner {
        float center;
        float half;
        int dir;
        bool cleared;
    };

    void startRace();
    void update(float dt);
    void draw();
    void sky();
    void road();
    void props();
    void busSprite();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal, int align = 0);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog = 0);
    float bendAt(float s) const;
    float influence(float s, const Corner& c) const;
    void bot(float& steer, bool& go, bool& stop) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int turns_ = 0;
    float crewLeft_ = 0;
    float race_ = 0;
    float s_ = 0;
    float speed_ = 0;
    float x_ = 0;
    float lean_ = 0;
    float steer_ = 0;
    float t_ = 0;
    float banner_ = 0;
    int next_ = 0;
    Corner corners_[3]{};
    static constexpr float kFinish = 980.f;
    static constexpr float kCrew = 66.f;
    static constexpr float kSafe = 16.5f;
};

}  // namespace busturn
