// S3 OVEN
//   s3oven                 bake the morning
//   s3oven --sim           autopilot draws six loaves
//   s3oven --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/oven.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        oven::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 24; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    oven::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    bool mid = false;
    const int limit = 60 * 50;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && cart.drawn() >= 1) {
            save(sys, "bake.png");
            mid = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.drawn() == 6) {
        std::printf("S3 OVEN  PASS  the morning holds  six loaves drawn  none burned  (%.1f s)\n", frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    const char* why = cart.why()[0] ? cart.why() : (cart.over() ? "failed" : "timed out");
    std::printf("S3 OVEN  FAIL  %s  drawn %d of 6  (%.1f s)\n", why, cart.drawn(), frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 OVEN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3oven [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<oven::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
