// S3 SUBSLIP — berth the boat in the pocket before the tide turns.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace slip {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SUBSLIP"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int hull() const { return hull_; }
    int tideLeft() const;  // whole seconds until the tide turns (0 if it has)

private:
    enum class Mode { Title, Run, Over };

    void beginRun();
    void stepRun();
    void collide();
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void sprite(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void readHelm(float& thrust, float& plane);
    float currentAt(float depth) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int hull_ = 3;
    int hold_ = 0;
    int flash_ = 0;
    float t_ = 0;
    float x_ = 0, y_ = 0, vx_ = 0, vy_ = 0;
    float thrust_ = 0, plane_ = 0;
    float cam_ = 0;
    float spin_ = 0;
    float ping_ = 0;
};

}  // namespace slip
