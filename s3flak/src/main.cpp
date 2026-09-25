// S3 FLAK
//   s3flak                 play
//   s3flak --sim           autopilot fights the raid
//   s3flak --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/flak.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    flak::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool title = false, fight = false;
    int frames = 0;
    const int limit = 60 * 45;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 0 && frames == 16 && !title) {
            save(sys, "title.png");
            title = true;
        } else if (m == 1 && cart.splashed() == 1 && !fight) {
            save(sys, "raid.png");
            fight = true;
        }
    }
    save(sys, "end.png");
    std::printf("S3 FLAK  %s  splashed %d  shells %d  deck %d\n", cart.won() ? "WIN" : "FAIL", cart.splashed(),
                cart.shells(), cart.deck());
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 FLAK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3flak [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<flak::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
