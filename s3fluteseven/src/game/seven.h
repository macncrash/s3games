// S3 FLUTE SEVEN — a short flute. Done when first to seven.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace fluteseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FLUTE SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    int you() const { return you_; }
    int them() const { return them_; }
    const char* why() const { return why_; }

private:
    enum class Mode { Title, Play, Miss, Leave, Over };

    void begin();
    void stepPlay();
    void blow();
    void rivalPoint();
    void miss();
    void winLeave();
    void lose();
    float headX() const;
    void audio();
    void tone(float freq, float vol, int frames);
    void draw();
    void backdrop();
    void stage();
    void image(const gs::Mipped& m, float left, float top, float h, int pal, bool flip = false, int fog = 0);
    void spr(const gs::Mipped& m, float cx, float bottom, float h, int pal, bool flip = false, int fog = 0);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool left_ = false;
    int you_ = 0;
    int them_ = 0;
    int finger_ = 0;
    int rivalTick_ = 0;
    int hold_ = 0;
    int tick_ = 0;
    int missT_ = 0;
    int tone_ = 0;
    int fanT_ = 0;
    int breath_ = 0;
    int shake_ = 0;
    float t_ = 0;
    float phase_ = 0;
    const char* why_ = "";
};

}  // namespace fluteseven
