// S3 PLOW GRASS — land the plow on the grass and come to a full stop.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace plowgrass {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PLOW GRASS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return you_; }

private:
    enum class Mode { Title, Drive, Pause, Win, Fail };

    struct Puff {
        float x, y, life;
    };

    void begin();
    void controls(float& gas, float& steer);
    void pilot(float& gas, float& steer);
    void physics(float dt, float gas, float steer);
    bool onGrass() const;
    void audio(float dt, float gas);
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal);
    void worldToScreen(float wx, float wy, float& sx, float& sy) const;
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    int plowFrame() const;
    void blip(float freq);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int fanStep_ = -1;
    float you_ = 0;
    float crew_ = 0;
    float hold_ = 0;
    float t_ = 0;
    float x_ = 0, y_ = 0, heading_ = 0, speed_ = 0;
    float wheel_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 2.1f;
    float tone_ = 0;
    Puff puff_[12]{};
    int puffN_ = 0;
};

}  // namespace plowgrass
