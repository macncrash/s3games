#pragma once
#include "console/system.h"

namespace granary {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 GRANARY DOOR"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int door() const { return door_; }
    int held() const { return held_; }

private:
    enum class Mode { Title, Play, End };

    struct Img {
        gs::Image im;
        int w = 0, h = 0;
        uint8_t* p = nullptr;
        void px(int x, int y, int c);
        void rect(int x, int y, int w, int h, int c);
        void frame(int x, int y, int w, int h, int c);
    };

    struct Foe {
        float x, y, spd;
        int kind;
        bool alive;
    };
    struct Pellet {
        float x, y, vx, vy;
        bool alive;
    };

    void buildArt(gs::VDP& vdp);
    void glyphTile(gs::VDP& vdp, int index, char ch);
    void paintSky(gs::VDP& vdp);
    void paintYard(gs::VDP& vdp);
    void clearHud(gs::VDP& vdp);
    void hudText(gs::VDP& vdp, int col, int row, const char* s, int pal);
    void drawWorld(gs::VDP& vdp);
    void blip(gs::APU& apu, float freq);
    void spawn(float x, float spd, int kind);
    void tickPlay(gs::Pad& pad, gs::APU& apu);
    float rnd();

    gs::System* sys_ = nullptr;
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int door_ = 12;
    int held_ = 0;
    int cool_ = 0;
    int shove_ = 0;
    int flash_ = 0;
    float px_ = 160;
    uint32_t rng_ = 0x6d2b79f5u;

    Img barn_, doorImg_, guard_, foe_[3], shot_, crack_, lamp_;
    Foe foes_[8] = {};
    Pellet shots_[10] = {};
    int tileSolid_ = 1;
    int tileWheat_ = 2;
    int tileDirt_ = 3;
    int fontBase_ = 16;
};

}  // namespace granary
