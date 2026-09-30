// S3 METROBOX — stop the train inside the box before the other crew.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace metro {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 METROBOX"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* report() const { return line_; }

private:
    enum class Mode { Title, Drive, Pause, Win, Fail };

    struct In {
        float throttle = 0;
        float brake = 0;
        float reverse = 0;
    };
    struct St {
        const char* name;
        float left;
        float width;
    };

    void showTitle();
    void begin();
    In readInput();
    In human();
    In pilot() const;
    void physics(const In& in, float dt);
    void judge();
    void finish(bool win, const char* why);
    void audio(float dt);
    void lights();
    void draw();
    void hudText(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void blit(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip);
    float cam() const;
    bool insideBox() const;
    float crewLeft() const;
    const St& stop() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int next_ = 0;
    int score_ = 0;
    int song_ = -1;
    float songT_ = 0;
    float time_ = 0;
    float dwell_ = 0;
    float anim_ = 0;
    float nose_ = 120;
    float speed_ = 0;
    float rival_ = 20;
    float msgT_ = 0;
    const char* why_ = nullptr;
    char msg_[48] = {};
    char line_[180] = {};
};

}  // namespace metro
