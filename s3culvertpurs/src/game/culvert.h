// S3 CULVERT PURSE — be the last machine still running, or the watch is over.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace cpurs {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CULVERT PURSE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int stalled() const { return stalled_; }
    int fleet() const { return fleet_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the culvert, 2 one other machine left, 3 the watch has ended
    int marker() const;

private:
    enum class Mode { Title, Watch, Pause, Victory, Over };

    struct Row {
        float t;
        int kind;
        int lane;
    };
    struct Mach {
        int kind = 0;
        int lane = 0;
        int hp = 1;
        int points = 0;
        float z = 0;
        float flash = 0;
        bool alive = true;
    };
    struct Puff {
        float x = 0;
        float z = 0;
        float t = 0;
        int kind = 0;
    };

    void beginWatch();
    void update(float dt);
    void spike();
    void hurt(Mach& m);
    void winWatch();
    void loseWatch(const char* why);
    void draw();
    void layCulvert(float shx);
    void project(float worldX, float z, float& sx, float& sy, float& s) const;
    const gs::Mipped& bodyOf(int kind) const;
    int palOf(int kind) const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0);
    void shadow(float cx, float cy, float w);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    int aliveCount() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* reason_ = "THE WATCH IS OVER";
    int score_ = 0;
    int stalled_ = 0;
    int fleet_ = 0;
    int spawnAt_ = 0;
    int hull_ = 3;
    float px_ = 0;
    float watch_ = 0;
    float t_ = 0;
    float endT_ = 0;
    float lunge_ = 0;
    float spikeCd_ = 0;
    float shake_ = 0;
    float scrape_ = 0;
    std::vector<Row> script_;
    std::vector<Mach> machs_;
    std::vector<Puff> puffs_;
};

}  // namespace cpurs
