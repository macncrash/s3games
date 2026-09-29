// S3 MEMORYMARK — one short order.
// Four cards hide the strokes of a mark. Watch the order, repeat it,
// then lift the stamp. That finished mark ends the cartridge.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace memorymark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MEMORYMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool finished() const { return finished_; }
    bool lifted() const { return lifted_; }
    bool marked() const { return marked_; }
    int got() const { return got_; }

private:
    enum class Mode { Title, Show, Recall, Lift, Pause, Over };

    struct Input {
        int dx = 0;
        bool action = false;
        bool start = false;
        bool back = false;
        float x = 0;
        float y = 0;
    };

    void toTitle();
    void begin();
    void confirm();
    void lift();
    void finish();
    void fail();
    void blip(float freq);
    void chord(float a, float b, float c);
    Input readPad(const gs::Pad& pad) const;
    Input botInput() const;
    bool faceUp(int i) const;
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void backdrop();
    void draw();

    static constexpr int kN = 4;
    static constexpr int kShowOn = 36;
    static constexpr int kShowGap = 14;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Recall;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool marked_ = false;
    bool lifted_ = false;
    bool finished_ = false;
    int seq_[kN] = {2, 0, 3, 1};
    int showI_ = 0;
    int showT_ = 0;
    int got_ = 0;
    int cursor_ = 0;
    int pace_ = 0;
    float clock_ = 0;
    float beep_ = 0;
    float handX_ = 40.f;
    float handY_ = 190.f;
    float stampY_ = 72.f;
    char note_[28] = {};
};

}  // namespace memorymark
