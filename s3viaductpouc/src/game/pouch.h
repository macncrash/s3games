#pragma once
#include "console/system.h"
#include "game/art.h"

namespace pouch {

// Carry one dispatch pouch the length of a single viaduct.
class Game : public gs::Cart {
public:
    const char* title() const override { return "VIADUCT POUCH"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }

private:
    enum class Mode { Title, Play, Pause, Dead, Victory };

    void drawSpan(float x0, float x1);
    void spr(const gs::Image& img, float x, float y, float w, float h, int pal, bool flip = false, bool shadow = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    bool deckAt(float x) const;
    void dropPouch(float kick);
    void respawn();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool held_ = true;
    bool faceR_ = true;
    int lives_ = 3;
    int frameN_ = 0;
    float x_ = 22;
    float y_ = 112;
    float vy_ = 0;
    float px_ = 22;
    float py_ = 100;
    float pvx_ = 0;
    float pvy_ = 0;
    float inv_ = 0;
    float windT_ = 0;
    int stepSnd_ = 0;
};

}  // namespace pouch
