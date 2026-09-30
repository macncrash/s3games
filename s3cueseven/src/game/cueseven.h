// S3 CUE SEVEN — play cue until first to seven. Leave when that is true.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace cueseven {

constexpr int kLine = 7;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CUE SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return you_ >= kLine && them_ < kLine && won_; }
    int you() const { return you_; }
    int them() const { return them_; }
    bool live() const { return mode_ == Mode::Aim || mode_ == Mode::Roll; }

private:
    enum class Mode { Title, Aim, Roll, Call, Win, Lose };

    struct Input {
        bool action = false;
        bool start = false;
    };

    void beginMatch();
    void beginVisit();
    void stroke();
    void stepRoll();
    void settle();
    void checkLine();
    Input readPad(const gs::Pad& pad) const;
    Input botInput() const;

    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void blob(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool yours_ = true;
    bool willPot_ = false;
    bool potted_ = false;
    int you_ = 0;
    int them_ = 0;
    int visits_ = 0;
    int themShots_ = 0;
    float meter_ = 0;
    float meterDir_ = 1;
    float clock_ = 0;
    float rollT_ = 0;
    float callT_ = 0;
    float cueX_ = 0;
    float objX_ = 0;
    float whiteX_ = 0;
    const char* say_ = "";
};

}  // namespace cueseven
