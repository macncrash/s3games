// S3 RIDGE BANN
//   s3ridgebann                 play
//   s3ridgebann --sim           autopilot brings the banner back
//   s3ridgebann --sim --shots D also writes PNGs into D
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
        rbann::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; ++i) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    rbann::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool out = false, hand = false, back = false, end = false;
    int frames = 0;
    const int limit = 60 * 50;
    while (!cart.over() && frames < limit) {
        sys.step();
        ++frames;
        int m = cart.marker();
        if (m == 1 && !out && frames > 50) {
            save(sys, "out.png");
            out = true;
        } else if (m == 2 && !hand) {
            save(sys, "banner.png");
            hand = true;
        } else if (m == 3 && !back && cart.heroZ() < 14.f) {
            save(sys, "back.png");
            back = true;
        } else if (m == 4 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    if (!cart.won()) {
        std::printf("S3 RIDGE BANN  the watch is over  %s  carry %d  z %.1f  banner %.1f  (%.1f s)\n", cart.reason(),
                    cart.carrying() ? 1 : 0, cart.heroZ(), cart.bannerZ(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 RIDGE BANN  the banner is back on the ridge\n");
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
            std::printf("S3 RIDGE BANN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3ridgebann [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<rbann::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
