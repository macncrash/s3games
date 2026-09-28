// S3 MOSAIC TAPE
//   s3mosaictape                 play
//   s3mosaictape --sim           autopilot matches the drawer to the tape
//   s3mosaictape --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/tape.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    tape::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool title = false, play = false, seal = false, end = false;
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 0 && !title && frames > 8) {
            save(sys, "title.png");
            title = true;
        } else if (m == 1 && !play && frames > 24) {
            save(sys, "play.png");
            play = true;
        } else if (m == 2 && !seal) {
            save(sys, "seal.png");
            seal = true;
        } else if (m == 3 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (cart.won() && cart.matched()) {
        std::printf("S3 MOSAIC TAPE  PASS  the drawer matches the tape  swaps %d  (%.1f s)\n", cart.swaps(),
                    frames / 60.0);
        return 0;
    }
    std::printf("S3 MOSAIC TAPE  FAIL  swaps %d  matched %d  (%.1f s)\n", cart.swaps(), cart.matched() ? 1 : 0,
                frames / 60.0);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 MOSAIC TAPE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3mosaictape [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<tape::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
