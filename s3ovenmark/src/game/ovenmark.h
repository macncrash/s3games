// S3 OVENMARK — one loaf. The gold coin is the baker's mark.
// Heat the crust into the gold band and press the coin on. Then lift it.
// That finished mark ends the cartridge. The rest of a morning is not the job.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace ovenmark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 OVENMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool finished() const { return finished_; }
    bool lifted() const { return lifted_; }
    bool marked() const { return marked_; }
    bool onGold() const { return onGold_; }

private:
    enum class Mode { Title, Bake, Lift, Pause, Over };

    struct Input {
        bool action = false;
        bool start = false;
        bool back = false;
        float x = 0;
        float y = 0;
    };

    void begin();
    void toTitle();
    void stamp();
    void lift();
    void finish();
    void fail();
    void blip(float freq);
    void chord(float a, float b, float c);
    Input readPad(const gs::Pad& pad) const;
    Input botInput() const;
    int loafPal() const;
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void backdrop();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Bake;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool marked_ = false;
    bool onGold_ = false;
    bool lifted_ = false;
    bool finished_ = false;
    float heat_ = 0;
    float clock_ = 0;
    float beep_ = 0;
    float lock_ = 0;
    float coinY_ = 0;
    float handX_ = 0;
    float handY_ = 0;
    char note_[24] = {};
};

}  // namespace ovenmark
