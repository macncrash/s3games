// S3 MUSHPLAT
//   s3mushplat                 play
//   s3mushplat --sim           autopilot stops level on every platform
//   s3mushplat --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/mush.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        mush::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 30; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    mush::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool run = false, end = false;
    int frames = 0;
    const int limit = 60 * 180;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 1 && !run && frames > 40) {
            save(sys, "leg.png");
            run = true;
        }
        if ((m == 4 || cart.won()) && !end) {
            save(sys, "win.png");
            end = true;
        }
    }
    if (!end) save(sys, "win.png");
    std::printf("S3 MUSHPLAT  %s  legs %d  team %d  (%.1f s)\n", cart.won() ? "PASS" : "FAIL", cart.made(),
                cart.lives(), frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 MUSHPLAT %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3mushplat [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<mush::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
