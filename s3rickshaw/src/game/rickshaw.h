// S3 RICKSHAW — one fare, three turns. Don't tip the cab.
#pragma once
#include <string>
#include <vector>

#include "console/system.h"
#include "game/cabart.h"

namespace rickshaw {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 RICKSHAW"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return job_; }
    const char* fail() const { return fail_; }
    float travel() const { return s_; }
    float offset() const { return x_; }
    float heading() const { return psi_; }
    float lean() const { return lean_; }
    float speed() const { return v_; }

private:
    enum class Mode { Title, Play, Pause, Win, Fail };
    enum Kind { Shop, House, Temple, Shed, Lamp, Stall, SignL, SignR, Post, Flower, Cow, Dog };

    struct Prop {
        float s, lat, wx, wy;
        int kind;
    };
    struct Node {
        float s, x, y, h;
    };

    void begin();
    void buildMesh();
    void layout();
    void centerAt(float s, float& x, float& y, float& h) const;
    float bendAt(float s) const;
    void pilot(float& u, bool& pedal, bool& brake) const;
    void physics(float u, bool pedal, bool brake);
    void tip(const char* why);
    void deliver();
    void audio(float dt);
    void blip(float freq);
    void draw(float s, float lat, float psi, float lean);
    void drawSky(float s);
    void drawRoad(float s, float lat, float psi);
    void drawProps(float s, float lat, float psi);
    void drawCab(float lean, bool spilled);
    void drawHud();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, int fog = 0, bool flip = false, bool shadow = false);
    void hud(int col, int row, const char* text, int pal);
    void hudC(int row, const char* text, int pal);
    const char* hint() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool spilled_ = false;
    const char* fail_ = "";
    float job_ = 0, settle_ = 0, t_ = 0, shake_ = 0;
    float s_ = 0, x_ = 0, psi_ = 0, v_ = 0, lean_ = 0, leanV_ = 0;
    float bellT_ = 0, toneT_ = 0;
    int chime_ = 0, chimeStep_ = 0;
    float chimeT_ = 0;
    std::vector<Node> road_;
    std::vector<Prop> props_;
};

}  // namespace rickshaw
