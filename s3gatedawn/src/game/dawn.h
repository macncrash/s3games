// S3 GATE DAWN — you have the gate. Keep the flares lit until dawn.
// Anything else is a loss.
#pragma once

#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace gatedawn {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 GATE DAWN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int lit() const;
    int tended() const { return tended_; }
    int struck() const { return struck_; }
    // 0 title, 1 the watch, 2 dawn, 3 a flare went out
    int marker() const;

    static constexpr int kFlares = 5;

private:
    enum class Mode { Title, Watch, Pause, Won, Lost };

    struct Mote {
        float x, y, vx, vy, life;
    };

    void bootTitle();
    void beginWatch();
    void beginWin();
    void beginLoss(int flare);
    void updateTitle();
    void updateWatch(float dt);
    void updatePause();
    void updateEnd(bool dawn);
    void readPad(float& dir, bool& feed, bool& strike) const;
    void think(float& dir, bool& feed, bool& strike);
    int choose();
    float flareScore(int i) const;
    float drain() const;
    void spawn();
    void advanceThreats(float dt);
    int nearest() const;
    void stepMotes(float dt);
    void burst(float x, float y, int n, float speed);
    void blip(float freq, float vol, int frames);
    void draw();
    void sky();
    void lamp();
    void drawWorld();
    void drawHud();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void word(const std::string& s, float cx, float y, float h, int pal);
    float lineWidth(const std::string& s, float h) const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0,
             bool feet = false, bool shadow = false, int clip = gs::SCREEN_H);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int tended_ = 0;
    int struck_ = 0;
    int shielded_ = 0;
    int dead_ = -1;
    int focus_ = -1;
    int hold_ = 0;
    int toneT_ = 0;
    int fanI_ = 0;
    int fanT_ = 0;
    int gustIx_ = 0;
    int climbIx_ = 0;
    float t_ = 0;
    float px_ = 160;
    float face_ = 1;
    float move_ = 0;
    float feedCd_ = 0;
    float fuel_[kFlares] = {};
    float idle_[kFlares] = {};
    float pop_[kFlares] = {};
    float hurt_[kFlares] = {};
    bool gustOn_[kFlares] = {};
    float gustEta_[kFlares] = {};
    bool climbOn_[kFlares] = {};
    float climbEta_[kFlares] = {};
    std::vector<Mote> motes_;
};

}  // namespace gatedawn
