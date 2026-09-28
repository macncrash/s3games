// S3 MILL POUC — you have the mill. Carry the pouch across.
// Reaching the far door without it, or losing it on the way, is a loss.
#pragma once
#include "console/system.h"
#include "game/art.h"

#include <string>

namespace mpouc {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MILL POUC"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 on the near walk, 2 out on the race, 3 the mill is decided
    int marker() const;

private:
    enum class Mode { Title, Carry, Pause, Victory, Fail };

    void begin();
    void finish(bool win, const char* why);
    void update(float dt);
    void bot(bool& right, bool& left, bool& jump);
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet);
    void text(const std::string& s, float x, float y, float scale, int pal);
    bool gateOpen() const;
    float gatePhase() const;
    bool chuteHits(float x0) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool carry_ = true;
    bool onGround_ = true;
    const char* reason_ = "";
    float t_ = 0;
    float playT_ = 0;
    float px_ = 28;
    float py_ = 168;
    float vy_ = 0;
    float sail_ = 0;
    int face_ = 1;
    int fan_ = -1;
    float fanT_ = 0;
};

}  // namespace mpouc
