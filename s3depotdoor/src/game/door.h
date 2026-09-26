// S3 DEPOT DOOR — you have the depot. Hold the door for three minutes.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace depotdoor {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 DEPOT DOOR"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float watch() const { return age_ / 60.f; }
    float openAmt() const { return lift_; }
    float arms() const { return arms_; }
    int misses() const { return misses_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the door, 2 held, 3 opened
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Won, Lost };

    struct Mark {
        int frame;
        int kind;
        int side;
    };
    struct Spark {
        float x = 0, y = 0, vx = 0, vy = 0, life = 0;
    };

    void begin();
    void toTitle();
    void finish(bool held);
    void schedule();
    int upcoming(int kind, int* side) const;
    void update();
    void botIntent(bool inTruck, int truckIn, bool& haul, bool& left, bool& right, bool& sack);
    void humanIntent(bool& haul, bool& left, bool& right, bool& sack);
    void pressDog(int side);
    void trySack();
    void burst(float x, float y);
    void blip(float freq);
    void fanfare();
    void serviceAudio();
    void sky(float shx);
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0,
             bool shadow = false);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip = false, int fog = 0,
               bool shadow = false);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool dogLive_ = false;
    bool dogAnswered_ = false;
    bool truckLive_ = false;
    bool hauling_ = false;
    const char* reason_ = "THE DOOR OPENED";
    int age_ = 0;
    int gate_ = 0;
    int misses_ = 0;
    int dogSide_ = 0;
    int missSide_ = 0;
    int fan_ = -1;
    int markCount_ = 0;
    int markNext_ = 0;
    float lift_ = 0.f;
    float arms_ = 1.f;
    float slip_ = 0.f;
    float sack_ = 0.f;
    float sackCd_ = 0.f;
    float dogL_ = 0.f;
    float dogR_ = 0.f;
    float dogVisL_ = 0.f;
    float dogVisR_ = 0.f;
    float shake_ = 0.f;
    float recoil_ = 0.f;
    float missT_ = 0.f;
    float blip_ = 0.f;
    float tick_ = 0.f;
    float fanT_ = 0.f;
    Mark marks_[64]{};
    Spark sparks_[12]{};
};

}  // namespace depotdoor
