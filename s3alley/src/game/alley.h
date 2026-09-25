// S3 ALLEY — the narrow street, and the door at the end of it.
#pragma once
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace alley {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 ALLEY"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int stumbles() const { return hits_; }
    float behind() const { return gap_; }
    float along() const { return z_; }
    float opened() const { return doorOpen_; }
    // 0 title, 1 the street, 2 the door, 3 caught
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Door, Caught };
    enum class Kind : uint8_t { Crate, Barrel, Sign, Line, Vent };

    struct Block {
        float z, x, half;
        Kind kind;
        bool duck;
        bool spent;
    };
    struct Bill {
        float depth;
        float x, foot, h, w;
        const gs::Mipped* img;
        int pal;
        bool flip;
        int fog;
        bool shadow;
    };

    void tune();
    void loadCourse();
    void enterTitle();
    void begin();
    void bot(bool& left, bool& right, bool& duck);
    void play(float dt);
    void draw();
    void backdrop();
    void queueWorld();
    void flushBills();
    void bill(const gs::Mipped& m, float wx, float wz, float worldH, float worldW, float lift, int pal, bool flip);
    void screenSpr(const gs::Mipped& m, float cx, float cy, float h, float w, int pal, bool flip, bool shadow);
    void glyphText(const std::string& s, float x, float y, float scale, int pal, int align);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    int fogOf(float depth) const;
    void hit(Block& b);
    void footstep();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool audio_ = false;
    int hits_ = 0;
    float z_ = 0, px_ = 0, camX_ = 0;
    float gap_ = 8;
    float t_ = 0, bob_ = 0, stun_ = 0, shake_ = 0, stepAcc_ = 0;
    float doorOpen_ = 0, fanT_ = 0, inv_ = 0;
    int fanStep_ = -1;
    std::vector<Block> blocks_;
    std::vector<Bill> bills_;
};

}  // namespace alley
