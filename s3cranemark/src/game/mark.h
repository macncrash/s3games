// S3 CRANE MARK — set the load down on the mark. Missing the end fails the leg.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"
#include "game/world.h"

namespace cranemark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CRANE MARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return race_; }
    const char* why() const { return why_; }
    float x() const { return loadX_; }
    float swing() const { return th_; }
    int phase() const { return phase_; }
    // 0 title, 1 on the leg, 2 over the mark, 3 holding the set, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Leg, Fail, Win };

    void begin();
    void titlePose();
    void update(float dt);
    void pilot(float hx, float& ax, float& hoist);
    void hookAt(float& x, float& y) const;
    void loadAt(float& x, float& y) const;
    void failLeg(const char* why);
    void winLeg();
    bool startPressed() const;
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool shadow = false);
    void blip(float freq);
    void chime(float dt);
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool resting_ = false;
    bool onMark_ = false;
    bool onDeck_ = false;
    bool onPier_ = false;
    bool lowering_ = false;
    int phase_ = 0;
    int melody_ = -1;
    float tx_ = HOME_X, vx_ = 0.f;
    float len_ = TRAVEL_L;
    float th_ = 0.f, om_ = 0.f;
    float t_ = 0.f, race_ = 0.f, hold_ = 0.f, dropT_ = 0.f;
    float melodyT_ = 0.f;
    float loadX_ = 0.f, loadY_ = 0.f;
    const char* why_ = "missed the end";
    const char* banner_ = "";
};

}  // namespace cranemark
