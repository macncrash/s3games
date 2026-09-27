// S3 ALLEY DOOR — one alley. Hold the door for three minutes.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace alley {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 ALLEY DOOR"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float gap() const { return gap_; }
    // 0 title, 1 the door, 2 held, 3 opened
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Won, Lost };

    struct Beat {
        int frame;
        int kind;
        int side;
    };
    struct Drop {
        float x = 0, y = 0, vy = 0, life = 0;
    };
    struct Wish {
        int lean = 0;
        bool catchChain = false;
        bool brick = false;
        bool shoulder = false;
    };

    void begin();
    void toTitle();
    void finish(bool held);
    void layBeats();
    void update();
    Wish wish() const;
    void hingeOn();
    void fanfare();
    void serviceAudio();
    void blip(float freq);
    void puff(float x, float y);
    void tickDrops();
    void alleyLight();
    void scene(float shx);
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
    bool chainCaught_ = false;
    bool bracing_ = false;
    bool shouldering_ = false;
    int age_ = 0;
    int gate_ = 0;
    int side_ = 0;
    int kind_ = 0;
    int lean_ = 0;
    int fan_ = -1;
    int beatCount_ = 0;
    int beatNext_ = 0;
    float gap_ = 0;
    float arms_ = 1;
    float slip_ = 0;
    float brickT_ = 0;
    float brickCd_ = 0;
    float yankT_ = 0;
    float yankOut_ = 0;
    float warn_ = 0;
    float shove_ = 0;
    float pending_ = 0;
    float hit_ = 0;
    float shake_ = 0;
    float recoil_ = 0;
    float blip_ = 0;
    float tick_ = 0;
    float fanT_ = 0;
    Beat beats_[96]{};
    Drop drops_[28]{};
};

}  // namespace alley
