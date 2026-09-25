// S3 SKATE
//   s3skate                 skate the line
//   s3skate --sim           autopilot lands the line
//   s3skate --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/skate.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    skate::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    bool shotRun = false;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!shotRun && frames == 16) {
            save(sys, "title.png");
            shotRun = true;
        }
        if (shotDir && frames == 180) save(sys, "line.png");
    }
    save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 SKATE  WIN  line landed  run %d  falls %d  (%.1f s)\n", cart.run(), cart.falls(),
                    frames / 60.0);
        return 0;
    }
    std::printf("S3 SKATE  FAIL  %s  run %d  line %d/%d  falls %d  (%.1f s)\n", cart.fail(), cart.run(), cart.line(),
                skate::Game::kTricks, cart.falls(), frames / 60.0);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 SKATE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3skate [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<skate::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
