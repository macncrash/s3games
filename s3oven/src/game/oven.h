// S3 OVEN — six loaves. Burn one and the morning fails.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace oven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 OVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int drawn() const { return drawn_; }
    const char* why() const { return why_; }

private:
    enum class Mode { Title, Play, Pause, Clear, Dead };

    struct Loaf {
        float near = 0;
        float far = 0;
        bool turned = false;
        bool pulled = false;
        bool toldTurn = false;
        bool toldPull = false;
        float fly = 0;
        float spark = 0;
    };

    void toTitle();
    void begin();
    void step(float dt);
    void human();
    void bot();
    bool tryTurn(int i);
    bool tryPull(int i);
    void win();
    void die(int i);
    void say(const char* s);
    void blip(float freq);
    void bed(float dt);
    float heat(int i) const;
    int look(int i) const;
    void hint(char* out, int n, int& pal) const;
    void draw();
    void backdrop();
    void meter(float x, float y, float value, float burn, float z0, float z1);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void solid(float x, float y, float w, float h, int pal);
    float slotX(int i) const { return 32.f + float(i) * 48.f; }

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int drawn_ = 0;
    int sel_ = 0;
    int lock_ = 0;
    int burned_ = -1;
    int fan_ = -1;
    int hold_ = 0;
    float t_ = 0;
    float anim_ = 0;
    float msgT_ = 0;
    float blipT_ = 0;
    float hurryT_ = 0.2f;
    float shake_ = 0;
    float fanT_ = 0;
    float flash_ = 0;
    char msg_[32] = {};
    char why_[48] = {};
    Loaf loaf_[6]{};
};

}  // namespace oven
