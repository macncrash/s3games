// S3 RIDGE POUC — you have the ridge. Carry the pouch across. Anything else is a loss.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace rpouc {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 RIDGE POUC"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool carrying() const { return carrying_; }
    float heroZ() const { return pz_; }
    float heroU() const { return u_; }
    float pouchZ() const { return pouchZ_; }
    float pouchU() const { return pouchU_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the carry, 2 the pouch is loose, 3 the far half, 4 the ridge has ended
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Fail };

    struct Foe {
        int kind = 0;  // 0 marches the spine, 1 hunts a loose pouch
        int dir = 1;
        float z = 0, lane = 0, home = 0, speed = 0, lo = 0, hi = 0, phase = 0, stun = 0;
        bool on = false;
        bool hasPouch = false;
    };
    struct Prop {
        float z = 0, u = 0, h = 32;
        int kind = 0;  // 0 post, 1 cairn, 2 rock
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
    void bot(float& axisU, float& axisZ, bool& plant, bool& toss) const;
    void tickFoes(float dt);
    void dropPouch(float away);
    void grab();
    void plant();
    void win();
    void lose(const char* why);
    bool stolen() const;
    bool loose() const { return !carrying_ && !planted_ && !stolen(); }
    float windNow() const;
    float foeU(const Foe& f) const;
    float viewZ(float worldZ) const;
    float bendAt(float row) const;
    float halfAt(float row) const;
    float worldAt(float row) const;
    int horizon() const;
    Spot spot(float u, float vz, float base) const;
    void blip(float freq);
    void fanfare(bool good);
    void serviceAudio();
    void draw();
    void layRoad();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet);
    void shadow(float cx, float cy, float w);
    void puffAt(float z, float u);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool carrying_ = true;
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
    float pouchZ_ = 0;
    float pouchU_ = 0;
    float pouchVz_ = 0;
    float pouchVu_ = 0;
    float stun_ = 0;
    float slip_ = 0;
    float scroll_ = 0;
    float shake_ = 0;
    float rush_ = 0;
    float beep_ = 0;
    float fanT_ = 0;
    float tossCd_ = 0;
    float shx_ = 0, shy_ = 0;
    std::vector<Foe> foes_;
    std::vector<Prop> props_;
    std::vector<Puff> puffs_;
};

}  // namespace rpouc
