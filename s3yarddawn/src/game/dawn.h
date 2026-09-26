// S3 YARD DAWN — you have the yard. Keep the flares lit until dawn.
// Anything else is a loss.
#pragma once

#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace yarddawn {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 YARD DAWN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int lit() const;
    int fed() const { return fed_; }
    int hauled() const { return hauled_; }
    int braced() const { return braced_; }
    // 0 title, 1 the watch, 2 dawn, 3 a flare went out
    int marker() const;

    static constexpr int kFlares = 5;

private:
    enum class Mode { Title, Watch, Pause, Won, Lost };

    struct Mote {
        float x, y, vx, vy, life;
        int pal;
    };

    void bootTitle();
    void beginWatch();
    void beginWin();
    void beginLoss(int flare);
    void updateTitle();
    void updateWatch(float dt);
    void updatePause();
    void updateEnd(bool dawn);
    void readPad(float& dir, bool& feed, bool& haul) const;
    void think(float& dir);
    int choose();
    bool mustHold(int i) const;
    float lowestOther(int i) const;
    float score(int i) const;
    float drain() const;
    float dawnEase() const;
    void spawn();
    void advanceThreats(float dt);
    int nearest() const;
    float drumX(int i) const;
    void canvasAt(int i, float& x, float& y) const;
    void stepMotes(float dt);
    void burst(float x, float y, int n, float speed, int pal);
    void blip(float freq, float vol, int frames);
    void sky();
    void lamp();
    void draw();
    void drawWorld();
    void drawHud();
    const char* hint() const;
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
    int fed_ = 0;
    int hauled_ = 0;
    int braced_ = 0;
    int dead_ = -1;
    int focus_ = -1;
    int hold_ = 0;
    int toneT_ = 0;
    int fanI_ = 0;
    int fanT_ = 0;
    int canvasIx_ = 0;
    int drumIx_ = 0;
    int face_ = 1;
    float t_ = 0;
    float px_ = 160;
    float move_ = 0;
    float feedCd_ = 0;
    float shake_ = 0;
    float fuel_[kFlares] = {};
    float pop_[kFlares] = {};
    float hurt_[kFlares] = {};
    bool canvasOn_[kFlares] = {};
    float canvasEta_[kFlares] = {};
    bool drumOn_[kFlares] = {};
    float drumEta_[kFlares] = {};
    bool lowPing_[kFlares] = {};
    std::vector<Mote> motes_;
};

}  // namespace yarddawn
