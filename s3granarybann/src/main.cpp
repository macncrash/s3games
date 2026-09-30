// S3 GRANARY BANN
//   s3granarybann                 play
//   s3granarybann --sim           walk the banner home
//   s3granarybann --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/granary.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        granary::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 30; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    granary::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool yard = false, carry = false, home = false;
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 1 && !yard) {
            save(sys, "yard.png");
            yard = true;
        } else if (m == 2 && !carry) {
            save(sys, "carry.png");
            carry = true;
        } else if (m == 3 && !home) {
            save(sys, "home.png");
            home = true;
        }
    }
    if (!home) save(sys, "home.png");
    std::printf("S3 GRANARY BANN  %s  the banner is %s  (%.1f s)\n", cart.won() ? "PASS" : "FAIL",
                cart.won() ? "home" : "still out", frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 GRANARY BANN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3granarybann [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<granary::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
