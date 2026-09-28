// S3 TOWER DAWN — keep the signal flares burning until the sky breaks.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace tower {

class Game : public gs::Cart {
public:
    const char* title() const override { return "TOWER DAWN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int relights() const { return relights_; }
    int flaresLit() const;
    // 0 title, 1 the watch, 2 ended
    int marker() const;

private:
    enum class Mode { Title, Watch, Lost, Won };

    void beginWatch();
    void update(float dt);
    void draw();
    void sky();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void lightAt(int i);
    void finish(bool win);
    float rnd();
    int nearest() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int relights_ = 0;
    float t_ = 0;
    float clock_ = 0;
    float px_ = 160;
    float face_ = 1;
    float lightCd_ = 0;
    float gustT_ = 2.2f;
    float gustLeft_ = 0;
    int gust_ = -1;
    float fuel_[4] = {1, 1, 1, 1};
    float dark_[4] = {};
    uint32_t rng_ = 0x7A11u;
    int frame_ = 0;
};

}  // namespace tower
