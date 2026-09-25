// S3 DRIFT
//   s3drift                 play the one pass
//   s3drift --sim           autopilot must score the slide without a wall
//   s3drift --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/drift.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    {
        gs::System sys(true);
        drift::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 24; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    drift::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);

    bool count = false, run = false, end = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 1 && !count && frames > 8) {
            save(sys, "count.png");
            count = true;
        } else if (m == 2 && !run && cart.score() > 3500) {
            save(sys, "slide.png");
            run = true;
        } else if (m == 3 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    if (!cart.over()) {
        std::printf("S3 DRIFT  FAIL  the pass did not finish\n");
        return 1;
    }
    std::printf("%s\n", cart.report());
    std::fflush(stdout);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 DRIFT %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3drift [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<drift::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
