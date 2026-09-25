// S3 GAUNTLET — one stone corridor, then the door at the end.
// Clear the hall, take the warden's key, and step into the door.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace gauntlet {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 GAUNTLET"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int lives() const { return lives_; }
    int hearts() const { return hearts_; }
    int left() const;
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Dead, Over, Victory };
    enum class Kind { Grunt, Archer, Warden };

    struct Enemy {
        Kind kind = Kind::Grunt;
        float x = 0, y = 0, hx = 0, hy = 0;
        int hp = 1, maxHp = 1;
        float shoot = 0.9f, stun = 0, flash = 0;
        int struck = -1;
        bool alive = true;
        bool alert = false;
    };
    struct Bolt {
        float x = 0, y = 0, vx = 0, vy = 0, life = 0, rad = 5;
        bool live = true;
    };
    struct Spark {
        float x = 0, y = 0, vx = 0, vy = 0, t = 0;
    };
    struct Popup {
        float x = 0, y = 0, t = 0;
        int pts = 0;
    };
    struct Food {
        float x = 0, y = 0;
        bool taken = false;
    };

    void resetLevel();
    void begin();
    void revive();
    void update(float dt);
    void tickFx(float dt);
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void blit(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow = false);
    void intent(float& mx, float& my, bool& swing, float& faceX, float& faceY);
    void hurt(int dmg);
    void swingHits();
    void killEnemy(Enemy& e);
    void spawnBolt(float x, float y, float tx, float ty, float speed, float rad);
    void spawnSparks(float x, float y, int n, float speed);
    void popup(float x, float y, int pts);
    void blip(float freq);
    void startFan();
    void fanStep(float dt);
    int prize(const Enemy& e) const;
    bool hallClear() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool hasKey_ = false;
    bool keyOn_ = false;
    int score_ = 0;
    int lives_ = 3;
    int hearts_ = 6;
    int facing_ = 0;
    int swingId_ = 0;
    int camI_ = 0;
    int shx_ = 0;
    int shy_ = 0;
    float px_ = 160, py_ = 1580;
    float keyX_ = 0, keyY_ = 0;
    float cp_ = 1580;
    float t_ = 0, playT_ = 0, deadT_ = 0;
    float swingT_ = 0, swingCd_ = 0, inv_ = 0;
    float shake_ = 0, hurtFlash_ = 0, beep_ = 0;
    float stall_ = 0, bash_ = 0, anchorX_ = 160, anchorY_ = 1580;
    float fanT_ = 0;
    int fan_ = -1;
    bool moving_ = false;
    std::vector<Enemy> enemies_;
    std::vector<Bolt> bolts_;
    std::vector<Spark> sparks_;
    std::vector<Popup> pops_;
    std::vector<Food> foods_;
};

}  // namespace gauntlet
