// S3 CULVERT DOOR — one culvert. Hold the door for three minutes.
// A surge you miss walks the leaf and raises the water. Either one ends the watch.
#pragma once
#include <vector>

#include "art.h"
#include "console/system.h"

namespace culvertdoor {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CULVERT DOOR"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int sealPct() const { return sealPct_; }
    int waterPct() const { return waterPct_; }
    int secondsLeft() const { return secondsLeft_; }

private:
    enum class Mode { Title, Play, Won, Lost };

    struct Surge {
        int at = 0;
        int dir = 1;
    };

    void bootPictures();
    void begin();
    void buildSurges();
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
    int surgeIx_ = 0;
    int flash_ = 0;
    int shake_ = 0;
    int toneT_ = 0;
    int sealPct_ = 0;
    int waterPct_ = 0;
    int secondsLeft_ = 180;
    float seal_ = 0;
    float water_ = 0;
    std::vector<Surge> surges_;
};

}  // namespace culvertdoor
