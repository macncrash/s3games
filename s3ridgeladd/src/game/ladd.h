// S3 RIDGE LADD — one ridge. Reach the far ladder. Then it is done.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <vector>

namespace rladd {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 RIDGE LADD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float heroZ() const { return pz_; }
    float heroU() const { return u_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the spine, 2 a break, 3 the ladder, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Won, Lost };
    enum { PROP_POST = 0, PROP_TOOTH = 1, PROP_CLIFF = 2, PROP_LADDER = 3 };
    enum { BAND_ROAD = 0, BAND_GAP = 1, BAND_VOID = 2 };

    struct Prop {
        float z = 0, u = 0, h = 32;
        int kind = 0;
    };
    struct Stone {
        float z = 0, phase = 0, rate = 0.5f, amp = 0.7f;
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
    void bot(float& axisU, float& axisZ, bool& jump);
    void readPad(float& axisU, float& axisZ, bool& jump) const;
    void bandAt(float z, float& u0, float& u1, int& kind) const;
    bool feetSafe(const char*& why) const;
    bool tryGrab(float axisZ);
    void startClimb();
    float windAt(float z) const;
    float stoneU(int i) const;
    int nextStone(float& dz) const;
    void win();
    void lose(const char* why);
    void blip(float freq);
    void fanfare(bool good);
    void puffAt(float z, float u);
    void serviceAudio();
    void draw();
    void layRoad();
    float viewZ(float worldZ) const;
    float worldAt(float row) const;
    float bendAt(float row) const;
    float halfAt(float row) const;
    int horizon() const;
    Spot spot(float u, float vz, float base) const;
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet);
    void shadow(float cx, float cy, float w);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool climbing_ = false;
    bool jumping_ = false;
    bool grounded_ = true;
    bool moving_ = false;
    const char* reason_ = "UNFINISHED";
    int face_ = 1;
    int fanStep_ = -1;
    bool fanGood_ = true;
    int holdStone_ = -1;
    int holdSide_ = 1;
    float t_ = 0;
    float hold_ = 0;
    float pz_ = 0;
    float u_ = 0;
    float vu_ = 0;
    float stun_ = 0;
    float hurt_ = 0;
    float leave_ = 0;
    float jumpT_ = 0;
    float jumpBuf_ = 0;
    float climb_ = 0;
    float wait_ = 0;
    float shake_ = 0;
    float scroll_ = 0;
    float beep_ = 0;
    float stepSnd_ = 0;
    float fanT_ = 0;
    float shx_ = 0, shy_ = 0;
    Stone stones_[3]{};
    std::vector<Prop> props_;
    std::vector<Puff> puffs_;
};

}  // namespace rladd
