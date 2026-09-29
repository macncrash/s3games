// S3 CLIFF PLAT
//   s3cliffplat                 play
//   s3cliffplat --sim           autopilot must stop level with the platform
//   s3cliffplat --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/plat.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    {
        gs::System sys(true);
        cliffplat::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    cliffplat::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 50;
    bool mid = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (shotDir && frames == 70) save(sys, "cliff.png");
        if (shotDir && !mid && frames > 200) {
            save(sys, "platform.png");
            mid = true;
        }
    }
    if (shotDir) save(sys, "end.png");
    if (!cart.won()) {
        std::printf("S3 CLIFF PLAT  FAIL  (%.1f s)\n", frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 CLIFF PLAT  WIN  stopped level with the platform ahead of the other crew  (%.1f s)\n",
                cart.seconds());
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
            std::printf("S3 CLIFF PLAT %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3cliffplat [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<cliffplat::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
