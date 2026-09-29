// S3 VIADUCT DOOR — one span. Hold the gate for three minutes.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace viaduct {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 VIADUCT DOOR"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float gap() const { return gap_; }
    // 0 title, 1 the span, 2 held, 3 opened
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Won, Lost };

    struct Beat {
        int frame;
        int kind;
        int side;
    };
    struct Mote {
        float x = 0, y = 0, vy = 0, life = 0;
    };
    struct Wish {
        int lean = 0;
        bool catchHitch = false;
        bool chock = false;
        bool set = false;
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
    void tickMotes();
    void spanLight();
    void scene(float shx);
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip = false, int fog = 0);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool hitchCaught_ = false;
    bool bracing_ = false;
    bool setting_ = false;
    int age_ = 0;
    int gate_ = 0;
    int side_ = 0;
    int kind_ = 0;
    int lean_ = 0;
    int fan_ = -1;
    int beatCount_ = 0;
    int beatNext_ = 0;
    float gap_ = 0;
    float grip_ = 1;
    float slip_ = 0;
    float chockT_ = 0;
    float chockCd_ = 0;
    float hitchT_ = 0;
    float hitchOut_ = 0;
    float warn_ = 0;
    float gust_ = 0;
    float pending_ = 0;
    float shake_ = 0;
    float recoil_ = 0;
    float blip_ = 0;
    float tick_ = 0;
    float fanT_ = 0;
    Beat beats_[96]{};
    Mote motes_[24]{};
};

}  // namespace viaduct
