// S3 DEPOT DAWN
//   s3depotdawn                 keep the flares lit
//   s3depotdawn --sim           autopilot holds the depot until dawn
//   s3depotdawn --sim --shots D also writes PNGs into D
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
        depotdawn::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 24; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    depotdawn::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool watch = false, end = false;
    int frames = 0;
    const int limit = 60 * 50;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!watch && m == 1 && frames > 90) {
            save(sys, "watch.png");
            watch = true;
        } else if (!end && (m == 2 || m == 3)) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (shotDir && !end) save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 DEPOT DAWN  THE FLARES HELD UNTIL DAWN  lit %d  poured %d  hooded %d\n", cart.lit(),
                    cart.poured(), cart.hooded());
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 DEPOT DAWN  THE WATCH IS OVER  lit %d  poured %d  hooded %d\n", cart.lit(), cart.poured(),
                cart.hooded());
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
            std::printf("S3 DEPOT DAWN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3depotdawn [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<depotdawn::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
