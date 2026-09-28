// S3 KEYSMARK — one phrase under a pencil mark.
// The last note closes the mark. That finished mark ends the cartridge.
#pragma once

#include <string>

#include "console/system.h"
#include "game/art.h"

namespace keysmark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 KEYSMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finished() const { return finished_; }
    int notes() const { return index_; }
    int slips() const { return slips_; }

private:
    enum class Mode { Title, Play, Pause, Over };

    void begin();
    void strike(int lane);
    void finish();
    void fail();
    void toneOff();
    void blip(float freq, float vol);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float w, float h, int pal);
    void word(const gs::Image& img, float cx, float y, int pal);
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Play;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finished_ = false;
    int index_ = 0;
    int slips_ = 0;
    float clock_ = 0;
    float botWait_ = 0;
    float toneT_ = 0;
    float flash_ = 0;
};

}  // namespace keysmark
