// S3 MASKSEVEN — gild a half-mask against the other bench.
// First mask to carry seven foils wins. Then hang it and leave.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace maskseven {

constexpr int kGoal = 7;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MASKSEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int you() const { return you_; }
    int rival() const { return rival_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 gilding, 2 left
    int marker() const;

private:
    enum class Mode { Title, Gild, Lead, Leave, Lost, Over };

    struct Input {
        bool lay = false;
        bool spoil = false;
        bool start = false;
    };

    static constexpr int kPeriod = 36;
    static constexpr int kSweet0 = 14;
    static constexpr int kSweet1 = 22;

    void begin();
    void startGild();
    void lay(bool sweet);
    void missBar();
    void toLead();
    void toLeave();
    void toLost(const char* why);
    void finishLeave();
    void stepPlay(const Input& in);
    Input readPad(const gs::Pad& pad) const;
    Input botInput() const;
    bool inSweet() const;
    float needle() const;
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void backdrop();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    const char* reason_ = "";
    int you_ = 0;
    int rival_ = 0;
    int clock_ = 0;
    int titleWait_ = 0;
    int show_ = 0;
    int flash_ = 0;
    bool barHit_ = false;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool released_ = true;
    float beep_ = 0;
};

}  // namespace maskseven
