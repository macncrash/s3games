// S3 MOWER
//   s3mower                 play
//   s3mower --sim           autopilot stripes the field before the rain
//   s3mower --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/mower.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        mower::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    mower::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool mid = false;
    int frames = 0;
    const int limit = 60 * 150;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && cart.stripes() >= 4) {
            save(sys, "stripes.png");
            mid = true;
        }
    }
    if (shotDir) save(sys, "end.png");
    int sec = cart.rainSeconds();
    if (!cart.won()) {
        std::printf("S3 MOWER  FAIL  rained out  stripes %d/%d  left %d  %d:%02d on the clock  (%.1f s)\n",
                    cart.stripes(), cart.goal(), cart.left(), sec / 60, sec % 60, frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 MOWER  PASS  cut the field before the rain  stripes %d/%d  %d:%02d left  (%.1f s)\n",
                cart.stripes(), cart.goal(), sec / 60, sec % 60, frames / 60.0);
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
            std::printf("S3 MOWER %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3mower [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<mower::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
