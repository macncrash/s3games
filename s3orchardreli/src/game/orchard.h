// S3 ORCHARD RELIEF — hold the rows until the relief bell, then answer it.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace orchard {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 ORCHARD RELIEF"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int baskets() const { return baskets_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the watch, 2 the bell is ringing, 3 the watch has ended
    int marker() const;

private:
    enum class Mode { Title, Watch, Victory, Over };

    struct Foe {
        int kind = 0;
        int lane = 0;
        float z = 0;
        float speed = 0;
        bool alive = true;
    };

    void beginWatch();
    void update(float dt);
    void botThink();
    void draw();
    void project(float worldX, float z, float& sx, float& sy, float& s) const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0, bool feet = true);
    void text(const std::string& s, float x, float y, float scale, int pal, int align = 0);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void winWatch();
    void loseWatch(const char* why);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool bell_ = false;
    const char* reason_ = "WATCH OVER";
    int score_ = 0;
    int baskets_ = 3;
    int spawnAt_ = 0;
    float t_ = 0;
    float watch_ = 0;
    float px_ = 0;
    float swing_ = 0;
    float rope_ = 0;
    float shake_ = 0;
    float endT_ = 0;
    bool wantSwing_ = false;
    bool wantRope_ = false;
    float move_ = 0;
    std::vector<Foe> foes_;
};

}  // namespace orchard
