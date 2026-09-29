// S3 FISH MARK
//   s3fishmark                 fish until the mark is finished
//   s3fishmark --sim           autopilot must cover the mark
//   s3fishmark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/fish.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        fishmark::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    fishmark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool cast = false, fight = false, board = false;
    int frames = 0;
    const int limit = 60 * 45;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!cast && m == 1) {
            save(sys, "cast.png");
            cast = true;
        } else if (!fight && m == 2) {
            save(sys, "fight.png");
            fight = true;
        } else if (!board && m == 3) {
            save(sys, "mark.png");
            board = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        std::printf("S3 FISH MARK  FAIL  the mark was not finished  (%.1f s)\n", frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("%s\n", cart.report());
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
            std::printf("S3 FISH MARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3fishmark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<fishmark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
