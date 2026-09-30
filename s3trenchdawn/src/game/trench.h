// S3 TRENCH DAWN — the trench is yours. Keep the flares lit until dawn.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace trench {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TRENCH DAWN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int lit() const;
    int fed() const { return fed_; }
    // 0 title, 1 the watch, 2 dawn, 3 a flare died
    int marker() const;

private:
    enum class Mode { Title, Watch, Pause, Won, Lost };

    void beginWatch();
    void readPad(float& dir, bool& feed) const;
    void think(float& dir, bool& feed);
    void tickWatch();
    void draw();
    void sky();
    void sprite(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool feeding_ = false;
    int fed_ = 0;
    int age_ = 0;
    int watch_ = 0;
    int face_ = 1;
    int step_ = 0;
    int dead_ = -1;
    int focus_ = -1;
    float px_ = 160.f;
    float fuel_[kFlares] = {};
    float shell_ = 0;
    int shellIx_ = 0;
};

}  // namespace trench
