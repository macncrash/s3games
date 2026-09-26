// S3 DEPOT PACE
//   s3depotpace                 play
//   s3depotpace --sim           the watch fires on the third pace
//   s3depotpace --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/depot.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        depotpace::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    depotpace::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool watch = false, third = false, end = false;
    int frames = 0;
    const int limit = 60 * 20;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 1 && !watch && frames > 30) {
            save(sys, "watch.png");
            watch = true;
        } else if (m == 2 && !third) {
            save(sys, "third.png");
            third = true;
        } else if (m == 3 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    const bool pass = cart.won() && cart.shotPace() == 3;
    if (pass) {
        std::printf("S3 DEPOT PACE  FIRED ON THE THIRD PACE  the depot holds  (%.1f s)\n", frames / 60.0);
    } else {
        std::printf("S3 DEPOT PACE  %s  pace %d  shot %d  (%.1f s)\n", cart.reason(), cart.pace(), cart.shotPace(),
                    frames / 60.0);
    }
    std::fflush(stdout);
    return pass ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 DEPOT PACE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3depotpace [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<depotpace::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
