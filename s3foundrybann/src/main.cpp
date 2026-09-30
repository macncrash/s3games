// S3 FOUNDRY BANN
//   s3foundrybann                 play
//   s3foundrybann --sim           autopilot brings the banner back
//   s3foundrybann --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/foundry.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        foundry::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    foundry::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool floor = false, hand = false, back = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 1 && !floor && frames > 50) {
            save(sys, "floor.png");
            floor = true;
        } else if (m == 2 && !hand) {
            save(sys, "banner.png");
            hand = true;
        } else if (m == 3 && !back) {
            save(sys, "back.png");
            back = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        std::printf("S3 FOUNDRY BANN  %s  lives %d  carry %d  x %.0f  banner %.0f  (%.1f s)\n", cart.reason(),
                    cart.lives(), cart.carrying() ? 1 : 0, cart.heroX(), cart.bannerX(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 FOUNDRY BANN  the banner is back in the foundry\n");
    std::fflush(stdout);
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 FOUNDRY BANN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3foundrybann [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<foundry::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
