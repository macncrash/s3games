// S3 FISHBELL — three strikes at the keeper.
// The bell hangs over the pier. It rings only when the hook meets the
// fish and misses the weed. A short strike, a snag, or a fish that
// swims through: that try dies. The third dead try ends it.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace fishbell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FISHBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    bool rules() const { return rules_; }
    bool swimming() const { return mode_ == Mode::Swim; }
    int deadTries() const { return dead_; }
    int tryNo() const { return tryNo_; }
    const char* reason() const { return why_; }

private:
    enum class Mode { Title, Swim, Dead, Ring, Leave, Over };
    enum class Fate { Ring, Short, Snag, Miss };

    struct Input {
        bool strike = false;
        bool start = false;
        float x = 0;
    };

    void fresh();
    void toTitle();
    void begin();
    void cast();
    void ring();
    void dieTry(Fate why);
    bool prove();
    Input readPad(const gs::Pad& pad) const;
    Input botInput() const;
    static const char* fateName(Fate f);

    void blip(float freq, float vol, float hold);
    void tickAudio(float dt);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void backdrop();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Fate whyFate_ = Fate::Miss;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    bool rules_ = false;
    bool struck_ = false;
    int dead_ = 0;
    int tryNo_ = 0;
    int fanStep_ = -1;
    float hookX_ = kHookHome;
    float fishX_ = kFishX0;
    float weedX_ = kWeedX0;
    float t_ = 0;
    float swimT_ = 0;
    float deadT_ = 0;
    float ringT_ = 0;
    float leaveT_ = 0;
    float bellPh_ = 0;
    float bellAmp_ = 0.16f;
    float bellTick_ = 0;
    float toneT_ = 0;
    float fanT_ = 0;
    const char* why_ = "OPEN";
};

}  // namespace fishbell
