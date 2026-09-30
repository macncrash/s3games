// S3 BUSTURN
//   s3busturn                 play
//   s3busturn --sim           autopilot makes the three turns
//   s3busturn --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/busturn.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        busturn::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 30; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    busturn::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool bend = false, depot = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 2 && !bend) {
            save(sys, "bend.png");
            bend = true;
        } else if (m == 3 && !depot) {
            save(sys, "depot.png");
            depot = true;
        }
    }
    if (!depot) save(sys, "depot.png");
    std::printf("S3 BUSTURN  %s  turns %d  crew %.1fs left  (%.1f s)\n", cart.won() ? "WIN" : "FAIL", cart.turns(),
                cart.crewLeft(), frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 BUSTURN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3busturn [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<busturn::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
