// S3 TILEMARK — one short floor.
// Four sockets take a tile. Match the painted mark, then press the stamp.
// That finished mark ends the cartridge. A row still open is not the job.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace tilemark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TILEMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finished() const { return finished_; }
    bool marked() const { return marked_; }
    int laid() const { return laid_; }

    static constexpr int kN = 4;

private:
    enum class Mode { Title, Lay, Stamp, Pause, Win };

    struct Input {
        int dx = 0;
        bool action = false;
        bool start = false;
        bool back = false;
    };

    void toTitle();
    void begin();
    void turn();
    void finish();
    void countLaid();
    void blip(float freq);
    void chord(float a, float b, float c);
    void decay();
    Input readPad(const gs::Pad& pad) const;
    Input botInput() const;
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void backdrop();
    void draw();

    static constexpr int kMark[kN] = {1, 2, 0, 3};

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Lay;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool marked_ = false;
    bool finished_ = false;
    int face_[kN] = {};
    int cursor_ = 0;
    int laid_ = 0;
    int pace_ = 0;
    int hold_ = 0;
    float clock_ = 0;
    float beep_ = 0;
    char note_[32] = {};
};

}  // namespace tilemark
