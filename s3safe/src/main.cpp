// S3 SAFE
//   s3safe                 play
//   s3safe --sim           set the dials and open the safe
//   s3safe --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/safe.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System titleSys(true);
        vault::Game title;
        titleSys.bootCart(title);
        for (int i = 0; i < 5; i++) titleSys.step();
        save(titleSys, "title.png");
    }

    gs::System sys(true);
    vault::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    for (int i = 0; i < 8; i++) sys.step();
    save(sys, "room.png");

    int frames = 8;
    const int limit = 60 * 20;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    save(sys, "open.png");
    if (!cart.won() || !cart.solved()) {
        std::printf("S3 SAFE  FAIL\n");
        return 1;
    }
    std::printf("the safe opens on %d-%d-%d\n", cart.dial(0), cart.dial(1), cart.dial(2));
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 SAFE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3safe [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<vault::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
