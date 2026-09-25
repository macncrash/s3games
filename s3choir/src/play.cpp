// S3 CHOIR
//   s3choir                 play
//   s3choir --sim           the three entries land together
//   s3choir --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/choir.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        choir::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    choir::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    bool board = false, land = false, win = false;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!board && frames == 36) {
            save(sys, "board.png");
            board = true;
        }
        if (!land && cart.marker() == 2) {
            save(sys, "land.png");
            land = true;
        }
        if (!win && cart.marker() == 4) {
            save(sys, "win.png");
            win = true;
        }
    }
    if (cart.won()) {
        std::printf("S3 CHOIR  the three land together  anthem sung  score %d\n", cart.score());
        return 0;
    }
    std::printf("S3 CHOIR  the piece stops  phrase %d  %s\n", cart.phrase() + 1,
                cart.reason()[0] ? cart.reason() : "TIME");
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 CHOIR %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3choir [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<choir::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
