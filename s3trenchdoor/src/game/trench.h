// S3 TRENCH DOOR — one trench, one timber gate, three minutes.
// A shove you miss walks the gate open. The night ends if it gives.
#pragma once
#include <vector>

#include "art.h"
#include "console/system.h"

namespace trench {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TRENCH DOOR"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int gatePct() const { return gatePct_; }
    int secondsLeft() const { return secondsLeft_; }

private:
    enum class Mode { Title, Play, Won, Lost };

    struct Shove {
        int at = 0;
        int dir = 1;
    };

    void bootPictures();
    void begin();
    void buildShoves();
    void act();
    void playTick();
    void endTick();
    void draw();
    void sprite(const gs::Mipped& m, float x, float y, float h, int pal, bool flip);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void tone(int ch, float freq, float vol);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool bracing_ = false;
    int age_ = 0;
    int play_ = 0;
    int lean_ = 0;
    int shoveIx_ = 0;
    int flash_ = 0;
    int shake_ = 0;
    int toneT_ = 0;
    int gatePct_ = 0;
    int secondsLeft_ = 180;
    float open_ = 0;
    std::vector<Shove> shoves_;
};

}  // namespace trench
