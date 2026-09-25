// S3 TORPEDO
//   s3torpedo                 play
//   s3torpedo --sim           autopilot puts one of two shots in the belly
//   s3torpedo --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/torpedo.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        torpedo::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; ++i) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    torpedo::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool run = false, flood = false, end = false;
    int frames = 0;
    const int limit = 60 * 25;
    while (!cart.over() && frames < limit) {
        sys.step();
        ++frames;
        const int m = cart.marker();
        if (m == 1 && !run && frames > 20) {
            save(sys, "run.png");
            run = true;
        } else if (m == 2 && !flood) {
            save(sys, "flood.png");
            flood = true;
        } else if (m >= 3 && !end) {
            save(sys, "sunk.png");
            end = true;
        }
    }
    if (!end) save(sys, "sunk.png");
    if (cart.won()) {
        std::printf("S3 TORPEDO  SUNK  the target is down  shot %d of 2  (%.1f s)\n", cart.killingShot(), frames / 60.0);
        return 0;
    }
    std::printf("S3 TORPEDO  FAIL  she got away  (%.1f s)\n", frames / 60.0);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 TORPEDO %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3torpedo [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<torpedo::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
