// S3 SIEGE
//   s3siege                 you are the wall
//   s3siege --sim           autopilot holds until every ram is stopped
//   s3siege --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/siege.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        siege::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    siege::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool wall = false, end = false;
    int frames = 0;
    const int limit = 60 * 50;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!wall && m == 1 && frames > 80) {
            save(sys, "wall.png");
            wall = true;
        } else if (!end && (m == 2 || m == 3)) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (shotDir && !end) save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 SIEGE  THE GATE HOLDS  stopped %d  gate %d  score %d\n", cart.stopped(), cart.gate(),
                    cart.score());
        return 0;
    }
    std::printf("S3 SIEGE  THE GATE BREAKS  stopped %d  gate %d  score %d\n", cart.stopped(), cart.gate(),
                cart.score());
    std::fprintf(stderr, "fail breaches %d over %d frames %d\n", cart.breaches(), cart.over() ? 1 : 0, frames);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 SIEGE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3siege [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<siege::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
