#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace spanpouc {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SPAN POUC"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* reason() const { return reason_.c_str(); }
    // 0 title, 1 on the span, 2 ended
    int marker() const;

private:
    enum class Mode { Title, Run, Fall, Won };

    void resetRun();
    void tick(float dt);
    void draw();
    void sky();
    void deck();
    float curve(float z) const;
    float half(float z) const;
    void blit(const gs::Image& img, float x, float y, float h, int pal, bool feet, int fog = 0);
    void textAt(const std::string& s, float x, float y, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    std::string reason_;
    float z_ = 0;
    float x_ = 0;
    float vx_ = 0;
    float speed_ = 0;
    float t_ = 0;
    float modeT_ = 0;
    float fallSide_ = 1;
    int hor_ = 78;
};

}  // namespace spanpouc
