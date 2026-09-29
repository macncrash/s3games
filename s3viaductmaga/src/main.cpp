// VIADUCT MAGA
//   s3viaductmaga            play
//   s3viaductmaga --sim      the watch is fought until the magazine outlasts the raid
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/maga.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    maga::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool raid = false, end = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 0 && frames == 8) save(sys, "title.png");
        if (m == 1 && !raid) {
            save(sys, "raid.png");
            raid = true;
        } else if (m == 2 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    std::printf("VIADUCT MAGA  %s  magazine %d outlasted the raid  (%.1f s)\n", cart.won() ? "PASS" : "FAIL",
                cart.rounds(), frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("VIADUCT MAGA %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3viaductmaga [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<maga::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
