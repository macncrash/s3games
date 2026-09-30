// S3 KEEL KILO — finish one kilometer in the keel. A wheel touch fails the leg.
// Missing the finish gate fails it too. The other crew holds the clock.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace keelkilo {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 KEEL KILO"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return time_; }
    const char* why() const { return why_; }

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    void begin();
    void pilot(float& thrust, float& steer);
    void stepBoat(float thrust, float steer);
    void fail(const char* why);
    void finish();
    void blip(float freq);
    float aimX() const;
    bool wheelHit(float cx, float wz, float rad) const;
    void draw();
    bool project(float wx, float wz, float& sx, float& sy, float& sc) const;
    void stamp(const gs::Mipped& m, float x, float y, float w, float h, int pal);
    void worldQuad(const gs::Mipped& m, float x0, float z0, float x1, float z1, float lift, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* why_ = "";
    float time_ = 0;
    float z_ = 0, x_ = 0, heading_ = 0, speed_ = 0;
    float camZ_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    int chime_ = -1;
    float chimeT_ = 0;
};

}  // namespace keelkilo
