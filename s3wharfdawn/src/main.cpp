// S3 WHARF DAWN
//   s3wharfdawn                 keep the flares lit
//   s3wharfdawn --sim           autopilot holds the wharf until dawn
//   s3wharfdawn --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/wharf.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    {
        gs::System sys(true);
        wharfdawn::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 18; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    wharfdawn::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool watch = false, end = false;
    int frames = 0;
    const int limit = 60 * 70;
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
        std::printf("S3 WHARF DAWN  THE FLARES HELD UNTIL DAWN  lit %d  fed %d\n", cart.lit(), cart.fed());
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 WHARF DAWN  THE WATCH IS OVER  lit %d  fed %d\n", cart.lit(), cart.fed());
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
            std::printf("S3 WHARF DAWN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3wharfdawn [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<wharfdawn::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
