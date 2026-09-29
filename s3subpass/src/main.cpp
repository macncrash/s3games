// S3 SUBPASS
//   s3subpass                 play
//   s3subpass --sim           autopilot takes the pass
//   s3subpass --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/pass.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        subpass::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 30; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    subpass::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool run = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!run && frames == 90) {
            save(sys, "pass.png");
            run = true;
        }
    }
    save(sys, "end.png");
    std::printf("S3 SUBPASS  %s  cleared the pass ahead of the other crew  hull %d  crew %d%%  (%.1f s)\n",
                cart.won() ? "WIN" : "FAIL", cart.hull(), cart.crewPct(), frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 SUBPASS %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3subpass [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<subpass::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
