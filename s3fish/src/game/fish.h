// S3 FISH — daylight is the clock. Five keepers.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace fish {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FISH"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool creelFull() const;
    bool fighting() const { return hooked_ >= 0; }
    int keepers() const { return keepers_; }
    bool kept(int species) const { return species >= 0 && species < 5 && have_[species]; }
    float seconds() const { return clock_; }
    float day() const;

    static constexpr int NFISH = 9;

private:
    enum class Mode { Title, Help, Play, Pause, Win, Lose };

    struct Fish {
        int sp = 0;
        bool keeper = false;
        float x = 0, y = 0, vx = 0;
        float phase = 0;
        float yBase = 120;
        int runDir = 1;
        float runT = 1;
        float shy = 0;
        bool out = false;
    };
    struct Puff {
        float x = 0, y = 0, t = 0;
    };

    void seed();
    void startDay();
    void updatePlay();
    void swim();
    void showcase();
    void approach();
    void fightFish();
    bool tryHook(bool allowShort);
    void hook(int i);
    void land();
    void snap();
    void cutLine();
    void tickFx();
    void audio();
    void draw();
    void drawOneFish(int i, int fogBias);
    void backdrop();
    void hudText(int col, int row, const char* s, int pal);
    void hudCenter(int row, const char* s, int pal);
    void pipRow(int col, int row, int on, int total, int onPal, int offPal);
    void text(const char* s, float x, float y, float scale, int pal, int align = 0);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0, bool feet = false,
             bool shadow = false);
    void line(float x0, float y0, float x1, float y1, int pal);
    void mouthOf(int i, float& mx, float& my) const;
    int faceOf(int i) const;
    void puffAt(float x, float y);
    void say(const char* s);
    void blip(bool high);
    void chime(bool big);
    int goal() const;
    float skyDay() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool have_[5] = {};
    int keepers_ = 0;
    float clock_ = 0;
    float age_ = 0;
    float boatX_ = 160;
    float lureX_ = 160;
    float lureY_ = 140;
    float tension_ = 0;
    float progress_ = 0;
    float bite_ = 0;
    float lock_ = 0;
    float hookBuf_ = 0;
    float shake_ = 0;
    float shakeX_ = 0, shakeY_ = 0;
    float msgT_ = 0;
    float showT_ = 0;
    float blipT_ = 0;
    float reelAcc_ = 0;
    float reelHold_ = 0;
    float chimeT_ = 0;
    int chimeStep_ = -1;
    bool chimeBig_ = false;
    int hooked_ = -1;
    int showSp_ = 0;
    bool showKeep_ = false;
    bool padOn_ = false;
    bool stung_ = false;
    char msg_[40] = {};
    Fish fish_[NFISH]{};
    Puff puff_[6]{};
    int puffN_ = 0;
};

}  // namespace fish
