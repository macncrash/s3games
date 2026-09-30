// S3 FOUNDRY DAWN — at the foundry, keep the flares lit until dawn.
// Miss that and the watch is over.
#pragma once

#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace foundrydawn {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FOUNDRY DAWN"; }
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

    static constexpr int kFlares = 4;

private:
    enum class Mode { Title, Watch, Won, Lost };
    enum class Kind { Slag, Quench };

    struct Threat {
        int flare = 0;
        Kind kind = Kind::Slag;
        float eta = 0;
        bool live = false;
    };
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
    void updateEnd();
    void readPad(float& dir, bool& feed, bool& brace, bool& haul) const;
    void think(float& dir, bool& feed, bool& brace, bool& haul);
    void spawnDue();
    void resolve(Threat& th);
    int lowest() const;
    float flareX(int i) const;
    float dawnEase() const;
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
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool feet = false);

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
    int beat_ = 0;
    int hold_ = 0;
    int toneT_ = 0;
    int face_ = 1;
    float t_ = 0;
    float px_ = 160;
    float move_ = 0;
    float feedCd_ = 0;
    float shake_ = 0;
    float fuel_[kFlares] = {};
    float pop_[kFlares] = {};
    float hurt_[kFlares] = {};
    Threat threat_{};
    std::vector<Mote> motes_;
};

}  // namespace foundrydawn
