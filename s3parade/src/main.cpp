// S3 PARADE
//   s3parade            play
//   s3parade --sim      autopilot marches to the square
//   s3parade --sim --shots D   also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/parade.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    parade::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    save(sys, "title.png");

    int frames = 0;
    bool mid = false;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && frames == 150) {
            save(sys, "march.png");
            mid = true;
        }
    }
    save(sys, cart.won() ? "win.png" : "end.png");
    if (cart.won()) {
        std::printf("S3 PARADE  WON  reached the square  rows %d  score %d  lives %d  (%.1f s)\n", cart.rows(),
                    cart.score(), cart.lives(), frames / 60.0);
    } else {
        std::printf("S3 PARADE  FAIL  did not reach the square  rows %d  score %d  lives %d  (%.1f s)\n", cart.rows(),
                    cart.score(), cart.lives(), frames / 60.0);
    }
    std::fflush(stdout);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 PARADE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3parade [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<parade::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
