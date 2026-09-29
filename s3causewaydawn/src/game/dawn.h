// S3 CAUSEWAY DAWN — one stone causeway over the water.
// Keep the four flares lit until dawn. A dark pot before then is a loss.
#pragma once

#include "art.h"
#include "console/system.h"

namespace cdawn {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CAUSEWAY DAWN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int lit() const;
    int fed() const { return fed_; }
    int braced() const { return braced_; }
    // 0 title, 1 the watch, 2 dawn, 3 a flare went out
    int marker() const;

    static constexpr int kFlares = 4;

private:
    enum class Mode { Title, Watch, Pause, Won, Lost };

    void bootWatch();
    void tickWatch(float dt);
    void readMove(float& dir, bool& pour, bool& brace) const;
    void think(float& dir, bool& pour, bool& brace);
    int choose() const;
    void skyAndRoad();
    void draw();
    void placeZ(float z, float& x, float& y, float& depth) const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal);
    void blip(int ch, float freq);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int fed_ = 0;
    int braced_ = 0;
    int dead_ = -1;
    int age_ = 0;
    int endAge_ = 0;
    int face_ = 1;
    int gustIx_ = -1;
    int gustSeq_ = 0;
    float clock_ = 0;
    float pz_ = 0.22f;
    float pourCd_ = 0;
    float gustT_ = 0;
    float nextGust_ = 1.6f;
    float fuel_[kFlares] = {};
    float bob_ = 0;
};

}  // namespace cdawn
