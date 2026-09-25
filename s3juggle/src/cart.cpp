// S3 JUGGLE
//   s3juggle                 play
//   s3juggle --sim           autopilot keeps three balls up for a minute
//   s3juggle --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/juggle.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        juggle::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 150; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    juggle::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool mid = false;
    int frames = 0;
    const int limit = 60 * 80;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (shotDir && !mid && cart.airFrames() > 150) {
            save(sys, "air.png");
            mid = true;
        }
    }
    if (shotDir) save(sys, cart.won() ? "win.png" : "end.png");
    std::printf("S3 JUGGLE  %s  drops %d  air %.0fs  (%.1fs)\n", cart.won() ? "PASS" : "FAIL", cart.drops(),
                cart.airFrames() / 60.0, frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 JUGGLE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3juggle [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<juggle::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
