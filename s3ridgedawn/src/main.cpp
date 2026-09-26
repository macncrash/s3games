// S3 RIDGE DAWN
//   s3ridgedawn                 keep the flares lit
//   s3ridgedawn --sim           autopilot holds the ridge until dawn
//   s3ridgedawn --sim --shots D also writes PNGs into D
#include "console/system.h"
#include "game/dawn.h"
#include "version.h"

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        rdawn::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; ++i) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    rdawn::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool watch = false, end = false;
    int frames = 0;
    const int limit = 60 * 50;
    while (!cart.over() && frames < limit) {
        sys.step();
        ++frames;
        int m = cart.marker();
        if (!watch && m == 1 && frames == 220) {
            save(sys, "watch.png");
            watch = true;
        } else if (!end && (m == 2 || m == 3)) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    const bool pass = cart.won() && cart.lit() == rdawn::Game::kFlares;
    if (pass) {
        std::printf("S3 RIDGE DAWN  THE FLARES HELD UNTIL DAWN  lit %d  fed %d  shielded %d\n", cart.lit(), cart.fed(),
                    cart.shielded());
    } else {
        std::printf("S3 RIDGE DAWN  %s  lit %d  fed %d  shielded %d  fuel %.0f %.0f %.0f %.0f  u %.2f  t %.1f\n",
                    cart.reason(), cart.lit(), cart.fed(), cart.shielded(), cart.fuelAt(0), cart.fuelAt(1),
                    cart.fuelAt(2), cart.fuelAt(3), cart.along(), cart.clock());
    }
    std::fflush(stdout);
    return pass ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 RIDGE DAWN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3ridgedawn [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<rdawn::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
