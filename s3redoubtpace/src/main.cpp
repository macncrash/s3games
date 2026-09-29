// S3 REDOUBT PACE
//   s3redoubtpace                 play
//   s3redoubtpace --sim           autopilot waits for the third pace
//   s3redoubtpace --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/pace.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        redoubtpace::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    redoubtpace::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool watch = false, third = false;
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 1 && !watch && frames > 50) {
            save(sys, "watch.png");
            watch = true;
        } else if (m == 2 && !third) {
            save(sys, "third.png");
            third = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        std::printf("S3 REDOUBT PACE  the watch is over  pace %d  shot %d  (%s)  (%.1f s)\n", cart.pace(),
                    cart.shotPace(), cart.reason(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 REDOUBT PACE  fired on the third pace\n");
    std::fflush(stdout);
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 REDOUBT PACE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3redoubtpace [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<redoubtpace::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
