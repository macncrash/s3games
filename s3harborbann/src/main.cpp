// S3 HARBOR BANN
//   s3harborbann                 play
//   s3harborbann --sim           autopilot brings the banner back
//   s3harborbann --sim --shots D also writes PNGs into D
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
        hbann::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; ++i) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    hbann::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool out = false, hand = false, back = false, end = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        ++frames;
        int m = cart.marker();
        if (m == 1 && !out && frames > 40) {
            save(sys, "channel.png");
            out = true;
        } else if (m == 2 && !hand) {
            save(sys, "banner.png");
            hand = true;
        } else if (m == 3 && !back && cart.boatZ() < 280.f) {
            save(sys, "return.png");
            back = true;
        } else if (m == 4 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    if (!cart.won()) {
        std::printf("S3 HARBOR BANN  the harbor is empty  %s  hull %d  carry %d  z %.0f  (%.1f s)\n", cart.reason(),
                    cart.hull(), cart.carrying() ? 1 : 0, cart.boatZ(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 HARBOR BANN  the banner is back at the harbor\n");
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
            std::printf("S3 HARBOR BANN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3harborbann [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<hbann::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
