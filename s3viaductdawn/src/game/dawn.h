// S3 VIADUCT DAWN — keep the pier flares lit until the gorge goes pale.
#pragma once

#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace viaduct {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 VIADUCT DAWN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int lit() const;
    int fed() const { return fed_; }
    int cupped() const { return cupped_; }
    // 0 title, 1 the watch, 2 dawn, 3 a flare went dark
    int marker() const;

    static constexpr int kFlares = 3;

private:
    enum class Mode { Title, Watch, Pause, Won, Lost };

    struct Mote {
        float x, y, vx, vy, life;
        int pal;
    };

    void bootTitle();
    void beginWatch();
    void beginWin();
    void beginLoss(int flare);
    void updateTitle();
    void updateWatch(float dt);
    void updatePause();
    void updateEnd(bool dawn);
    void readPad(float& dir, bool& feed, bool& cup) const;
    void think(float& dir, bool& feed, bool& cup);
    int nearest() const;
    void stepMotes(float dt);
    void burst(float x, float y, int n, int pal);
    void blip(float freq, float vol, int frames);
    void sky();
    void gorge();
    void lamp();
    void draw();
    void drawWorld();
    void drawHud();
    const char* hint() const;
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void word(const std::string& s, float cx, float y, float h, int pal);
    float lineWidth(const std::string& s, float h) const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0,
             bool feet = false);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int fed_ = 0;
    int cupped_ = 0;
    int dead_ = -1;
    int hold_ = 0;
    int toneT_ = 0;
    int fanI_ = 0;
    int fanT_ = 0;
    int gustIx_ = 0;
    int gustFlare_ = -1;
    int face_ = 1;
    int pitch_ = 0;
    float t_ = 0;
    float px_ = 40;
    float move_ = 0;
    float feedCd_ = 0;
    float caskCd_ = 0;
    float shake_ = 0;
    float gustLeft_ = 0;
    float gustHeld_ = 0;
    bool lowPing_[kFlares] = {};
    float fuel_[kFlares] = {};
    float pop_[kFlares] = {};
    std::vector<Mote> motes_;
};

}  // namespace viaduct
