// S3 RIDGE POUC
//   s3ridgepouc                 play
//   s3ridgepouc --sim           autopilot carries the pouch across
//   s3ridgepouc --sim --shots D also writes PNGs into D
#include "console/system.h"
#include "game/pouc.h"
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
        rpouc::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; ++i) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    rpouc::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool carry = false, far = false, loose = false, end = false;
    int frames = 0;
    const int limit = 60 * 50;
    while (!cart.over() && frames < limit) {
        sys.step();
        ++frames;
        int m = cart.marker();
        if (m == 1 && !carry && frames > 40) {
            save(sys, "carry.png");
            carry = true;
        } else if (m == 2 && !loose) {
            save(sys, "loose.png");
            loose = true;
        } else if (m == 3 && !far) {
            save(sys, "far.png");
            far = true;
        } else if (m == 4 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    if (!cart.won()) {
        std::printf("S3 RIDGE POUC  the ridge is lost  %s  carry %d  z %.1f  u %.2f  pouch %.1f %.2f  (%.1f s)\n",
                    cart.reason(), cart.carrying() ? 1 : 0, cart.heroZ(), cart.heroU(), cart.pouchZ(), cart.pouchU(),
                    frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 RIDGE POUC  the pouch crossed the ridge\n");
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
            std::printf("S3 RIDGE POUC %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3ridgepouc [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<rpouc::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
