// S3 PLOWSLIP
//   s3plowslip                 play
//   s3plowslip --sim           autopilot berths the plow before the tide
//   s3plowslip --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/plow.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        plow::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 30; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    plow::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool runShot = false;
    int frames = 0;
    const int limit = 60 * 50;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!runShot && frames == 90) {
            save(sys, "slip.png");
            runShot = true;
        }
    }
    if (!runShot) save(sys, "slip.png");
    save(sys, "end.png");
    std::printf("S3 PLOWSLIP  %s  score %d  tide %.1fs  (%.1f s)\n", cart.won() ? "PASS" : "FAIL", cart.score(),
                cart.tideLeft(), frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 PLOWSLIP %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3plowslip [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<plow::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
