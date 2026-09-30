// S3 FOUNDRY DOOR — you have the foundry. Hold the furnace door for three minutes.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace foundrydoor {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FOUNDRY DOOR"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seam() const { return seam_; }
    // 0 title, 1 the door, 2 held, 3 opened
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Won, Lost };
    enum class Kind { None, Push, Hook };

    struct Beat {
        int frame;
        int side;  // -1 left slag, +1 right latch, 0 hoist
    };
    struct Spark {
        float x = 0, y = 0, vy = 0, life = 0;
    };

    struct Wish {
        int lean = 0;
        bool dog = false;
        bool quench = false;
        bool plant = false;
    };

    void begin();
    void toTitle();
    void finish(bool held);
    void layBeats();
    void update();
    Wish wish() const;
    bool notchOpen() const;
    void droneOn();
    void fanfare();
    void serviceAudio();
    void blip(float freq);
    void sparkAt(float x, float y);
    void tickSparks();
    void backdrop();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool dogged_ = false;
    bool bracing_ = false;
    bool planting_ = false;
    int age_ = 0;
    int gate_ = 0;
    int side_ = 0;
    int lean_ = 0;
    int fan_ = -1;
    int beatCount_ = 0;
    int beatNext_ = 0;
    Kind kind_ = Kind::None;
    float seam_ = 0;
    float grip_ = 1;
    float slip_ = 0;
    float warn_ = 0;
    float shove_ = 0;
    float hook_ = 0;
    float quenchCd_ = 0;
    float shake_ = 0;
    float blip_ = 0;
    float tick_ = 0;
    float fanT_ = 0;
    float steam_ = 0;
    Beat beats_[64]{};
    Spark sparks_[16]{};
};

}  // namespace foundrydoor
