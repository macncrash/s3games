// S3 TANK
//   s3tank                 play the block
//   s3tank --sim           autopilot keeps moving until it takes the block
//   s3tank --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/tank.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        tank::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 30; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    tank::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool street = false, end = false;
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!street && m == 1 && frames > 20) {
            save(sys, "street.png");
            street = true;
        } else if (!end && m == 2) {
            save(sys, "block.png");
            end = true;
        }
    }
    if (!end) save(sys, "block.png");
    if (cart.won()) std::printf("S3 TANK  TAKES THE BLOCK\n");
    else {
        std::printf("S3 TANK  STOPPED\n");
        std::fprintf(stderr, "fail %s hits %d\n", cart.why(), cart.hits());
    }
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 TANK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3tank [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<tank::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
