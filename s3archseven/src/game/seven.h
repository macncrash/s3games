// S3 ARCH SEVEN — one arrow apiece. First to seven.
// A count that steps past seven still stands.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace archseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 ARCH SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    int you() const { return you_; }
    int them() const { return them_; }

private:
    enum class Mode { Title, Aim, Nock, Flight, Call, Win, Lose };

    struct Mark {
        float x = 0, y = 0;
        int pts = 0;
        bool yours = false;
    };

    bool audit();
    void begin();
    void steer();
    bool drawHeld() const;
    Hit predict(float& x, float& y) const;
    void loose();
    void arrive();
    void nextTurn();
    void blip(int ch, float hz, float vol, int frames);
    void pump();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void stamp(const gs::Image& img, float x, float y, float w, float h, int pal);
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mark marks_[12]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool yours_ = true;
    bool skipHold_ = false;
    int you_ = 0;
    int them_ = 0;
    int gain_ = 0;
    int markN_ = 0;
    int drawTick_ = 0;
    int steady_ = 0;
    int flightT_ = 0;
    int callT_ = 0;
    int t_ = 0;
    int blipCh_ = 0;
    int blipLeft_ = 0;
    float aimX_ = kCx;
    float aimY_ = kCy;
    float landX_ = kCx;
    float landY_ = kCy;
    char last_[16] = {};
};

}  // namespace archseven
