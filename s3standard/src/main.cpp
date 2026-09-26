// S3 STANDARD
//   s3standard                 play
//   s3standard --sim           autopilot brings the flag back
//   s3standard --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/standard.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        standard::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 48; ++i) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    standard::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool road = false, taken = false, back = false, end = false;
    int frames = 0;
    const int limit = 60 * 45;
    while (!cart.over() && frames < limit) {
        sys.step();
        ++frames;
        int m = cart.marker();
        if (m == 1 && !road && frames > 50) {
            save(sys, "road.png");
            road = true;
        } else if (m == 2 && !taken) {
            save(sys, "flag.png");
            taken = true;
        } else if (m == 3 && !back && cart.heroZ() < 70.f) {
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
            "S3 STANDARD  the flag is still on the road  lives %d  carry %d  z %.0f  flag %.0f  face %d  speed %.0f  (%.1f s)\n",
            cart.lives(), cart.carrying() ? 1 : 0, cart.heroZ(), cart.flagZ(), cart.face(), cart.speed(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 STANDARD  the flag is off the road and back with the lines\n");
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
            std::printf("S3 STANDARD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3standard [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<standard::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
