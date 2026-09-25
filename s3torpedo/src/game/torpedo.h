// S3 TORPEDO — two shots. The freighter sinks only if the red belly is holed.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace torpedo {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TORPEDO"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int killingShot() const { return killingShot_; }
    // 0 title, 1 the run, 2 flooding, 3 ended
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Won, Lost };
    enum class Hit { None, Armor, Belly };

    struct Torp {
        float x, y;
        int shot;
        int aim;  // -1 shallow, 0 in the belly, 1 under the keel
    };
    struct Bit {
        float x, y, vx, vy, a;
        int kind;  // 0 bubble, 1 splash, 2 smoke
    };

    void beginTitle();
    void beginRun();
    void play(float dt);
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0);
    void say(const std::string& s, float t);
    void launch();
    void floodFrom(const Torp& t);
    void miss(const Torp& t);
    void clang();
    void fanfare();
    void fail(const char* why);
    float shipY() const;
    float localY(float worldY) const;
    Hit probe(float wx, float wy) const;
    int aimOf(float worldY) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool flooding_ = false;
    bool engineOn_ = false;
    int shotsLeft_ = 2;
    int shotsFired_ = 0;
    int killingShot_ = 0;
    float attackT_ = 0;
    float shipX_ = 200;
    float shipV_ = 16;
    float drop_ = 0;
    float subY_ = 160;
    float cool_ = 0;
    float shake_ = 0;
    float hold_ = 0;
    float doom_ = 0;
    float ping_ = 0.4f;
    float blip_ = 0;
    float msgT_ = 0;
    int fanStep_ = -1;
    float fanT_ = 0;
    std::string msg_;
    std::string end_;
    std::vector<Torp> torps_;
    std::vector<Bit> bits_;
};

}  // namespace torpedo
