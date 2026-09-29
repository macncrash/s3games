// S3 QUARRY DAWN
//   s3quarrydawn                 keep the flares lit
//   s3quarrydawn --sim           autopilot holds the quarry until dawn
//   s3quarrydawn --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/dawn.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    {
        gs::System sys(true);
        qdawn::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 18; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    qdawn::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool watch = false, end = false;
    int frames = 0;
    const int limit = 60 * 80;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!watch && m == 1 && frames > 80) {
            save(sys, "watch.png");
            watch = true;
        } else if (!end && (m == 2 || m == 3)) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (shotDir && !end) save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 QUARRY DAWN  THE FLARES HELD UNTIL DAWN  lit %d  fed %d\n", cart.lit(), cart.fed());
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 QUARRY DAWN  THE WATCH IS OVER  lit %d  fed %d\n", cart.lit(), cart.fed());
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
            std::printf("S3 QUARRY DAWN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3quarrydawn [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<qdawn::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
