// S3 CRANEPASS
//   s3cranepass                 play
//   s3cranepass --sim           autopilot clears the pass
//   s3cranepass --sim --shots D also writes PNGs into D
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
        pass::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 30; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    pass::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool mid = false;
    int frames = 0;
    const int limit = 60 * 120;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && cart.cleared() >= 2) {
            save(sys, "pass.png");
            mid = true;
        }
    }
    save(sys, "end.png");
    std::printf("S3 CRANEPASS  %s  cleared %d  crew clock %.1fs  (%.1f s)\n", cart.won() ? "PASS" : "FAIL",
                cart.cleared(), cart.clock(), frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 CRANEPASS %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3cranepass [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<pass::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
