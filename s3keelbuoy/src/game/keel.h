// S3 KEELBUOY — round three buoys and return to the same dock.
#pragma once
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace keelbuoy {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 KEELBUOY"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int buoys() const { return buoys_; }
    float x() const { return x_; }
    float y() const { return y_; }
    float heading() const { return heading_; }
    float speed() const { return speed_; }

private:
    enum class Mode { Title, Sail, Pause, Victory };

    struct Wake {
        float x, y, life;
    };

    void begin();
    void update(float dt);
    void controls(float& steer, float& trim);
    void pilot(float& steer, float& trim);
    void physics(float dt, float steer, float trim);
    void marks();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float sx, float sy, float h, int pal);
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal);
    bool worldToScreen(float wx, float wy, float& sx, float& sy) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool leftDock_ = false;
    bool brake_ = false;
    int buoys_ = 0;
    int tack_ = 0;
    int wp_ = 0;
    float t_ = 0;
    float race_ = 0;
    float tackT_ = 10;
    float x_ = 0, y_ = 0, heading_ = 0, speed_ = 0, yaw_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 1.6f;
    float stuckT_ = 0, stuckX_ = 0, stuckY_ = 0;
    std::vector<Wake> wakes_;
};

}  // namespace keelbuoy
