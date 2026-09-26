// S3 DEPOT DAWN — at the depot, keep the flares lit until dawn.
// Miss that and the watch is over.
#pragma once

#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace depotdawn {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 DEPOT DAWN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int lit() const;
    int poured() const { return poured_; }
    int hooded() const { return hooded_; }
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
    void beginLoss(int pot);
    void updateTitle();
    void updateWatch(float dt);
    void updatePause();
    void updateEnd();
    void readPad(float& dir, bool& feed) const;
    void think(float& dir, bool& feed);
    int choose();
    int nearest() const;
    void openDrip();
    void openShunt();
    void dripStep(float dt);
    void resolveShunt();
    void tryFeed();
    void embers();
    void stepMotes(float dt);
    void burst(float x, float y, int n, float speed, int pal);
    void blip(float freq, float vol, int frames);
    void serviceAudio();
    float drain() const;
    float dawnEase() const;
    const char* hint() const;
    void lamp();
    void sky();
    void yard();
    void draw();
    void drawWorld();
    void drawHud();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void word(const std::string& s, float cx, float y, float h, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0,
             bool feet = false, bool shadow = false, int clip = gs::SCREEN_H);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int poured_ = 0;
    int hooded_ = 0;
    int dead_ = -1;
    int focus_ = 0;
    int hold_ = 0;
    int toneT_ = 0;
    int fanI_ = 0;
    int fanT_ = 0;
    int tick_ = 0;
    int shuntIx_ = 0;
    int shuntImpact_ = 0;
    int dripIx_ = 0;
    int dripPot_ = -1;
    int dripEnd_ = 0;
    int face_ = 1;
    bool shuntOn_ = false;
    bool dripOn_ = false;
    float seat_ = 0;
    float px_ = 160;
    float move_ = 0;
    float feedCd_ = 0;
    float shake_ = 0;
    float ox_ = 0, oy_ = 0;
    float yardV_ = 0;
    float fuel_[kFlares] = {};
    float pop_[kFlares] = {};
    float hurt_[kFlares] = {};
    std::vector<Mote> motes_;
};

}  // namespace depotdawn
