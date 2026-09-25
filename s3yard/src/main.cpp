// S3 YARD
//   s3yard                 drive the junkyard
//   s3yard --sim           autopilot is the last machine and takes the purse
//   s3yard --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/yard.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        yard::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    yard::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool mid = false;
    int frames = 0;
    const int limit = 60 * 60;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && cart.marker() == 1 && frames == 70) {
            save(sys, "yard.png");
            mid = true;
        }
    }
    save(sys, "purse.png");
    if (cart.won()) {
        std::printf("S3 YARD  LAST MACHINE TAKES THE PURSE\n");
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 YARD  ENGINE DEAD\n");
    std::fprintf(stderr, "fail %s hp %d live %d x %.1f y %.1f (%.1fs)\n", cart.why(), cart.youHp(), cart.live(),
                 cart.px(), cart.py(), frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 YARD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3yard [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<yard::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
