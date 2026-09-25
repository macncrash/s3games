// S3 FLAK — a deck gun, six bombers, ten shells.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace flak {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FLAK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int splashed() const { return splashed_; }
    int shells() const { return rounds_; }
    int deck() const { return hits_; }
    // 0 title, 1 the raid, 2 the deck held or lost
    int marker() const { return mode_ == Mode::Title ? 0 : mode_ == Mode::Play ? 1 : 2; }

private:
    enum class Mode { Title, Play, End };
    enum class St { Fly, Run, Dead, Gone };

    struct Plane {
        float x = 0, y = 0, vx = 0, fuse = -1;
        St st = St::Fly;
    };
    struct Shell {
        float x0 = 0, y0 = 0, tx = 0, ty = 0, t = 0, dur = 1;
    };
    struct Bomb {
        float x = 0, y = 0, vy = 0;
    };
    struct Puff {
        float x = 0, y = 0, t = 0, life = 0.4f;
        int kind = 0;
    };
    struct Brass {
        float x = 0, y = 0, vx = 0, vy = 0;
    };

    void beginRaid();
    void updateTitle(float dt);
    void updatePlay(float dt);
    void updateFx(float dt);
    void control(float dt);
    void tryFire(bool want);
    void burstAt(float x, float y, bool hit);
    void dropBomb(const Plane& p);
    void finish(bool win);
    void audio(float dt);
    bool predict(const Plane& p, float& ax, float& ay) const;
    int pickPlane() const;
    bool raidClear() const;
    void backdrop();
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float h, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog = 0, bool feet = false,
             bool world = true);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int rounds_ = 10;
    int splashed_ = 0;
    int hits_ = 0;
    int next_ = 0;
    float t_ = 0;
    float endT_ = 0;
    float sightX_ = 160, sightY_ = 70;
    float cool_ = 0;
    float flash_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    float fanT_ = 0;
    float camX_ = 0, camY_ = 0;
    float demoX_ = 70, demoVx_ = 62, demoY_ = 90;
    int fan_ = -1;
    std::vector<Plane> planes_;
    std::vector<Shell> shells_;
    std::vector<Bomb> bombs_;
    std::vector<Puff> puffs_;
    std::vector<Brass> brass_;
};

}  // namespace flak
