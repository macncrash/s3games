// S3 WHARF DOOR — hold the freight door through a three-minute tide watch.
// Lean into the slip and pin the hoist. Miss either and the door walks.
#pragma once
#include <vector>

#include "art.h"
#include "console/system.h"

namespace wharf {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 WHARF DOOR"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int gapPct() const { return gapPct_; }
    int secondsLeft() const { return secondsLeft_; }

private:
    enum class Mode { Title, Play, Won, Lost };

    struct Tide {
        int at = 0;
        int dir = 1;
    };
    struct Hoist {
        int at = 0;
    };

    void begin();
    void schedule();
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
    int pin_ = 0;
    int tideIx_ = 0;
    int hoistIx_ = 0;
    int flash_ = 0;
    int shake_ = 0;
    int toneT_ = 0;
    int gapPct_ = 0;
    int secondsLeft_ = 180;
    float gap_ = 0;
    float crateY_ = -40;
    std::vector<Tide> tides_;
    std::vector<Hoist> hoists_;
};

}  // namespace wharf
