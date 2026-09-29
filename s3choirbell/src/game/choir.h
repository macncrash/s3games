// S3 CHOIRBELL — a short choir under one bell.
// Five singers. Hit each note in the phrase. The bell rings if you finish
// before the third try dies.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace choir {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CHOIRBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int tries() const { return tries_; }
    int marker() const;

private:
    enum class Mode { Title, Phrase, Ring, Fail, Over };

    void beginPhrase();
    void update();
    void draw();
    void text(const char* s, float x, float y, float scale, int pal, int align = 0);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void chime(float freq);
    void bellVoice();
    void missVoice();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int tries_ = 3;
    int age_ = 0;
    int phrase_ = 0;
    int next_ = 0;
    int hold_ = 0;
    bool struck_[6] = {};
    float swing_ = 0;
    int mouth_ = 0;
};

}  // namespace choir
