// S3 CULVERTWELL — the culvert is yours. Keep the well standing through three waves.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace culvertwell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CULVERTWELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int wave() const { return wave_; }
    int well() const { return hp_; }
    // 0 title, 1 the culvert, 2 a wave held, 3 the well fell, 4 it stands
    int marker() const;

private:
    enum class Mode { Title, Play, Banner, Fell, Stood };
    enum class Kind : uint8_t { Sapper, Barrel, Ram };

    struct Spawn {
        float t;
        Kind kind;
        int lane;
        int hp;
    };
    struct Threat {
        Kind kind;
        int lane;
        int hp;
        float z;
        bool gone;
    };

    void begin();
    void armWave();
    void update(float dt);
    void botAct(int& lane, bool& swing);
    void draw();
    void project(int lane, float z, float& sx, float& sy, float& scale) const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int wave_ = 0;
    int hp_ = 5;
    int lane_ = 1;
    int spawnAt_ = 0;
    float t_ = 0;
    float waveT_ = 0;
    float scroll_ = 0;
    float cool_ = 0;
    float swing_ = 0;
    float shake_ = 0;
    float banner_ = 0;
    std::vector<Spawn> spawns_;
    std::vector<Threat> threats_;
};

}  // namespace culvertwell
