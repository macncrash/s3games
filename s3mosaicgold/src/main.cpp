// S3 MOSAIC GOLD
//   s3mosaicgold                 play the mosaic
//   s3mosaicgold --sim           autopilot leaves when only the gold counts double
//   s3mosaicgold --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/gold.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    mosaicgold::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, play = false;
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!titled && frames == 8) {
            save(sys, "title.png");
            titled = true;
        }
        if (!play && m == 1) {
            save(sys, "play.png");
            play = true;
        }
    }
    save(sys, "end.png");
    const bool math = cart.score() == cart.gold() * 2 + cart.cream() && cart.bare() == cart.gold() + cart.cream();
    if (cart.won() && cart.arranged() && cart.gold() > 0 && cart.bare() < cart.line() && cart.score() >= cart.line() &&
        math) {
        std::printf(
            "S3 MOSAIC GOLD  DOUBLE  only the gold counts double  score %d  bare %d  gold %d  cream %d  moves %d  (%.1f s)\n",
            cart.score(), cart.bare(), cart.gold(), cart.cream(), cart.moves(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 MOSAIC GOLD  OPEN  no gold double  score %d  bare %d  gold %d  cream %d  moves %d  (%.1f s)\n",
                cart.score(), cart.bare(), cart.gold(), cart.cream(), cart.moves(), frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 MOSAIC GOLD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3mosaicgold [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<mosaicgold::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
