// S3 TILE GOLD
//   s3tilegold                 set tiles until only the gold counts double
//   s3tilegold --sim           autopilot leaves cream and sets the gold
//   s3tilegold --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/tilegold.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    tilegold::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, tiled = false;
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!titled && m == 0 && frames >= 8) {
            save(sys, "title.png");
            titled = true;
        }
        if (!tiled && m == 1) {
            save(sys, "tile.png");
            tiled = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.goldOut() && cart.score() >= cart.line() && cart.bare() < cart.line() && cart.golds() > 0 &&
        cart.cream() == 0) {
        std::printf(
            "S3 TILE GOLD  DOUBLE  only the gold counts double  score %d  bare %d  golds %d  cream %d  line %d  "
            "(%.1f s)\n",
            cart.score(), cart.bare(), cart.golds(), cart.cream(), cart.line(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 TILE GOLD  SHORT  score %d  bare %d  golds %d  cream %d  line %d  (%.1f s)\n", cart.score(),
                cart.bare(), cart.golds(), cart.cream(), cart.line(), frames / 60.0);
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
            std::printf("S3 TILE GOLD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3tilegold [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<tilegold::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
