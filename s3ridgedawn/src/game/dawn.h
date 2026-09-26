// S3 RIDGE DAWN — one ridge. Keep the flares lit until dawn.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace rdawn {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 RIDGE DAWN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int lit() const;
    int fed() const { return fed_; }
    int shielded() const { return shielded_; }
    float fuelAt(int i) const { return (i >= 0 && i < kFlares) ? fuel_[i] : 0.f; }
    float along() const { return u_; }
    float clock() const { return t_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the watch, 2 dawn, 3 a flare went out
    int marker() const;

    static constexpr int kFlares = 4;

private:
    enum class Mode { Title, Watch, Pause, Won, Lost };

    struct Mote {
        float x, y, vx, vy, life;
        int pal;
    };
    struct Spot {
        float x = 0, y = 0, h = 0;
        int fog = 0;
        bool ok = false;
    };

    void bootTitle();
    void beginWatch();
    void beginWin();
    void beginLoss(int flare);
    void updateTitle(float dt);
    void updateWatch(float dt);
    void updatePause();
    void updateEnd(float dt);
    void readPad(float& dir, bool& feed) const;
    void think(float& dir);
    int choose();
    float drain() const;
    float windVel() const;
    float gustField() const;
    int nearest() const;
    void openGusts();
    void resolveGust(float dt);
    void tryFeed();
    void embers();
    void stepMotes(float dt);
    void burst(float x, float y, int n, float speed, int pal);
    void blip(float freq, float vol, int frames);
    void serviceAudio();
    void lamp();
    void draw();
    void layRoad();
    Spot spot(float u, float z, float base) const;
    float bendAt(float row) const;
    float halfAt(float row) const;
    int horizon() const;
    float dawnEase() const;
    const char* hint() const;
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow = false,
             int clip = gs::SCREEN_H);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* reason_ = "UNFINISHED";
    int fed_ = 0;
    int shielded_ = 0;
    int missed_ = 0;
    int dead_ = -1;
    int focus_ = 0;
    int gustIx_ = 0;
    int gustFlare_ = -1;
    int gustDir_ = 1;
    int hold_ = 0;
    int toneT_ = 0;
    int fanI_ = 0;
    int fanT_ = 0;
    int face_ = 1;
    int hor_ = 78;
    float t_ = 0;
    float anim_ = 0;
    float u_ = 0;
    float feedCd_ = 0;
    float shake_ = 0;
    float shx_ = 0, shy_ = 0;
    float gustEta_ = 0;
    bool gustOn_ = false;
    float fuel_[kFlares] = {};
    float pop_[kFlares] = {};
    float hurt_[kFlares] = {};
    std::vector<Mote> motes_;
};

}  // namespace rdawn
