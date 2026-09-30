// S3 CULVERT RELIEF — hold the culvert mouth until the relief bell, then answer it.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace culvert {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CULVERT RELIEF"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the watch, 2 the bell, 3 the watch has ended
    int marker() const;

private:
    enum class Mode { Title, Watch, Bell, Victory, Over };

    struct Foe {
        int kind = 0;
        int lane = 0;
        float z = 1;
        float speed = 0.15f;
        float cool = 0.8f;
        int hp = 1;
        bool alive = true;
    };
    struct Bolt {
        int lane = 0;
        float z = 0;
    };
    struct Shot {
        int lane = 0;
        float z = 0.5f;
    };
    struct Nade {
        int lane = 0;
        float fuse = 0.8f;
        float z = 0.4f;
    };

    void beginWatch();
    void update(float dt);
    void botThink();
    void draw();
    void project(int lane, float z, float& sx, float& sy, float& s) const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0);
    void text(const std::string& s, float x, float y, float scale, int pal, int align = 0);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void lose(const char* why);
    void answer();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* reason_ = "WATCH OVER";
    int score_ = 0;
    int hp_ = 6;
    int lane_ = 1;
    int spawnAt_ = 0;
    float t_ = 0;
    float watch_ = 0;
    float duck_ = 0;
    float fireCd_ = 0;
    float laneCd_ = 0;
    float shake_ = 0;
    float endT_ = 0;
    float bellSwing_ = 0;
    float scroll_ = 0;
    bool drone_ = false;
    std::vector<Foe> foes_;
    std::vector<Bolt> bolts_;
    std::vector<Shot> shots_;
    std::vector<Nade> nades_;
};

}  // namespace culvert
