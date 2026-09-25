// S3 TABLE
//   s3table                 play first to seven
//   s3table --sim           autopilot plays until the puck has crossed seven times
//   s3table --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/table.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    table::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false;
    int frames = 0;
    const int limit = 60 * 75;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 18) {
            save(sys, "title.png");
            titled = true;
        }
    }
    save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 TABLE  SEVEN  you %d  them %d  the puck crossed  (%.1f s)\n", cart.you(), cart.them(),
                    frames / 60.0);
        return 0;
    }
    std::printf("S3 TABLE  SHORT  you %d  them %d  (%.1f s)\n", cart.you(), cart.them(), frames / 60.0);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 TABLE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3table [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<table::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
