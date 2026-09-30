// S3 GRANARY BANN — one granary. Bring the banner back. Then it is done.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace granary {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 GRANARY"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    // 0 title, 1 yard, 2 banner in hand, 3 banner home
    int marker() const;

private:
    enum class Mode { Title, Yard, Victory };

    void layYard();
    void begin();
    void update();
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet);
    bool barnBlocks(float x, float y) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool have_ = false;
    float t_ = 0;
    float px_ = 168, py_ = 156;
    float face_ = 1;
    float step_ = 0;
    float hold_ = 0;
    int frameN_ = 0;
};

}  // namespace granary
