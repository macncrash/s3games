// S3 TRENCH DOOR
//   s3trenchdoor                 hold the gate
//   s3trenchdoor --sim           autopilot keeps the trench
//   s3trenchdoor --sim --shots D also writes PNGs into D
//
//   Hold Z, X, C, or Space to brace. Left and Right meet the shove.
//   Enter starts. Miss the shove and the gate walks open.
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/trench.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    trench::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    if (shotDir) {
        for (int i = 0; i < 12; i++) sys.step();
        save(sys, "title.png");
    }
    int frames = 0;
    const int limit = 60 * 220;
    bool mid = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (shotDir && !mid && cart.secondsLeft() < 150 && cart.secondsLeft() > 0) {
            save(sys, "trench.png");
            mid = true;
        }
    }
    if (shotDir) save(sys, cart.won() ? "held.png" : "end.png");
    std::printf("S3 TRENCH DOOR  %s  watch %ds left  gate %d%%  (%.1f s)\n", cart.won() ? "PASS" : "FAIL",
                cart.secondsLeft(), cart.gatePct(), frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 TRENCH DOOR %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3trenchdoor [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<trench::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
