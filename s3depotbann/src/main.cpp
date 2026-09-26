// S3 DEPOT BANN
//   s3depotbann                 play
//   s3depotbann --sim           autopilot brings the banner back
//   s3depotbann --sim --shots D also writes PNGs into D
#include "console/system.h"
#include "game/bann.h"
#include "version.h"

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        dbann::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 70; ++i) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    dbann::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool out = false, hand = false, back = false, end = false;
    int frames = 0;
    const int limit = 60 * 140;
    while (!cart.over() && frames < limit) {
        sys.step();
        ++frames;
        int m = cart.marker();
        if (m == 1 && !out && frames > 50) {
            save(sys, "yard.png");
            out = true;
        } else if (m == 2 && !hand) {
            save(sys, "banner.png");
            hand = true;
        } else if (m == 3 && !back && cart.heroX() < 900.f) {
            save(sys, "back.png");
            back = true;
        } else if (m == 4 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    if (!cart.won()) {
        std::printf(
            "S3 DEPOT BANN  the yard kept the banner  %s  lives %d  carry %d  x %.0f  banner %.0f  track %d  phase %d  (%.1f s)\n",
            cart.reason(), cart.lives(), cart.carrying() ? 1 : 0, cart.heroX(), cart.bannerX(), cart.track(),
            cart.phase(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 DEPOT BANN  the banner is back at the depot\n");
    std::fflush(stdout);
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 DEPOT BANN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3depotbann [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<dbann::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
