// S3 YARD DOOR — one yard. Hold the door for three minutes.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace yarddoor {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 YARD DOOR"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float gap() const { return slide_; }
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
    struct Intent {
        int lean = 0;
        bool slam = false;
        bool chock = false;
        bool shoulder = false;
    };

    void begin();
    void toTitle();
    void finish(bool held);
    void schedule();
    void update();
    Intent intent() const;
    void creakOn();
    void fanfare();
    void serviceAudio();
    void blip(float freq);
    void burst(float x, float y);
    void tickSparks();
    void sky(float shx);
    void lamp();
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
    bool barCaught_ = false;
    bool bracing_ = false;
    bool shouldering_ = false;
    int age_ = 0;
    int gate_ = 0;
    int side_ = 0;
    int kind_ = 0;
    int lean_ = 0;
    int fan_ = -1;
    int markCount_ = 0;
    int markNext_ = 0;
    float slide_ = 0;
    float arms_ = 1;
    float slip_ = 0;
    float chockT_ = 0;
    float chockCd_ = 0;
    float barT_ = 0;
    float barOut_ = 0;
    float tele_ = 0;
    float hit_ = 0;
    float pendingHit_ = 0;
    float shake_ = 0;
    float recoil_ = 0;
    float blip_ = 0;
    float tick_ = 0;
    float fanT_ = 0;
    Mark marks_[80]{};
    Spark sparks_[12]{};
};

}  // namespace yarddoor
