// S3 CLAY
//   s3clay                 shoot twenty-five birds
//   s3clay --sim           autopilot breaks the match
//   s3clay --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/clay.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        clay::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 12; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    clay::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool fly = false, match = false;
    int frames = 0;
    const int limit = 60 * 50;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int phase = cart.phase();
        if (!fly && phase == 1) {
            save(sys, "fly.png");
            fly = true;
        } else if (!match && phase == 2) {
            save(sys, "match.png");
            match = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won() || cart.matchHits() != 5) {
        std::printf("S3 CLAY  LOST  last five %d/5  broken %d/25  bird %d  (%.1f s)\n", cart.matchHits(), cart.broken(),
                    cart.birdNo(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 CLAY  MATCH  last five %d/5  broken %d/25  (%.1f s)\n", cart.matchHits(), cart.broken(),
                frames / 60.0);
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
            std::printf("S3 CLAY %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3clay [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<clay::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
