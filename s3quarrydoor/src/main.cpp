// S3 QUARRY DOOR
//   s3quarrydoor                 hold the quarry gate
//   s3quarrydoor --sim           autopilot keeps the gate shut
//   s3quarrydoor --sim --shots D also writes PNGs into D
//
//   Hold Z, X, C, or Space to plant the shore. Left and Right meet the haul.
//   Enter starts. Miss the truck and the gate walks open.
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/door.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    quarry::Game cart;
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
            save(sys, "gate.png");
            mid = true;
        }
    }
    if (shotDir) save(sys, cart.won() ? "held.png" : "end.png");
    std::printf("S3 QUARRY DOOR  %s  watch %ds left  gate %d%% open  (%.1f s)\n", cart.won() ? "PASS" : "FAIL",
                cart.secondsLeft(), cart.openPct(), frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 QUARRY DOOR %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3quarrydoor [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<quarry::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
