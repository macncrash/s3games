// PALISADE MAGA
//   s3palisademaga            hold the magazine through the raid
//   s3palisademaga --sim      the magazine outlasts the raid
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
    {
        gs::System sys(true);
        palisade::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }
    gs::System sys(true);
    palisade::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool raid = false, end = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 1 && !raid && frames > 90) {
            save(sys, "raid.png");
            raid = true;
        } else if ((m == 2 || m == 3) && !end) {
            save(sys, "held.png");
            end = true;
        }
    }
    if (!end) save(sys, "held.png");
    std::printf("S3 PALISADE MAGA  %s  score %d  rounds %d  wall %d  (%.1f s)\n", cart.won() ? "PASS" : "FAIL",
                cart.score(), cart.rounds(), cart.wall(), frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 PALISADE MAGA %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3palisademaga [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<palisade::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
