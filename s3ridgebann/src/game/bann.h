// S3 RIDGE BANN — at the ridge, bring the banner back. Miss that and the watch is over.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace rbann {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 RIDGE BANN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool carrying() const { return carrying_; }
    float heroZ() const { return pz_; }
    float bannerZ() const { return bz_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the run out, 2 the banner is in hand at the cairn, 3 the return, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Fail };

    struct Foe {
        float z = 0, lo = 0, hi = 0, lane = 0, phase = 0, stun = 0;
        int dir = 1;
        bool on = false;
    };
    struct Prop {
        float z = 0, u = 0, h = 40;
        int kind = 0;
    };
    struct Puff {
        float z = 0, u = 0, t = 0;
    };
    struct Spot {
        float x = 0, y = 0, h = 0;
        int fog = 0;
        bool ok = false;
    };

    void bootTitle();
    void begin();
    void layout();
    void update();
    void bot(float& axisU, float& axisZ, bool& take) const;
    void grab();
    void plant();
    void win();
    void lose(const char* why);
    void tickFoes(float dt);
    void blip(float freq);
    void fanfare(bool good);
    void serviceAudio();
    float windNow() const;
    float foeU(const Foe& f) const;
    float viewZ(float worldZ) const;
    float bendAt(float row) const;
    float halfAt(float row) const;
    int horizon() const;
    Spot spot(float u, float vz, float base) const;
    void draw();
    void layRoad();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet);
    void shadow(float cx, float cy, float w);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool carrying_ = false;
    bool planted_ = false;
    bool fanGood_ = true;
    const char* reason_ = "THE WATCH IS OVER";
    int face_ = 1;
    int lastSec_ = 99;
    int fanStep_ = -1;
    float t_ = 0;
    float watch_ = 0;
    float hold_ = 0;
    float pz_ = 0;
    float u_ = 0;
    float vu_ = 0;
    float bz_ = 0;
    float bu_ = 0;
    float stun_ = 0;
    float slip_ = 0;
    float scroll_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    float fanT_ = 0;
    float shx_ = 0, shy_ = 0;
    float rush_ = 0;
    std::vector<Foe> foes_;
    std::vector<Prop> props_;
    std::vector<Puff> puffs_;
};

}  // namespace rbann
