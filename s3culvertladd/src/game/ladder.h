// S3 CULVERT LADDER — you have the culvert. Reach the far ladder.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>
#include <vector>

namespace culvert {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CULVERT LADDER"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the culvert, 2 the far ladder, 3 the watch failed
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Victory, Fail };

    struct Solid {
        float x, y, w, h;
    };

    void bootWall();
    void begin();
    void update();
    void bot(bool& left, bool& right, bool& jump, bool& up);
    void win();
    void lose(const char* why);
    void blip(float freq);
    void serviceAudio();
    bool hits(float l, float t, float r, float b, const Solid& s) const;
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* reason_ = "THE FAR LADDER IS STILL AHEAD";
    std::vector<Solid> solids_;
    float px_ = 48.f, py_ = 168.f, vx_ = 0.f, vy_ = 0.f;
    float cam_ = 0.f;
    float t_ = 0.f;
    float clock_ = 0.f;
    float hold_ = 0.f;
    float blip_ = 0.f;
    float face_ = 1.f;
    bool grounded_ = false;
    bool climbing_ = false;
    int rungs_ = 0;
};

}  // namespace culvert
