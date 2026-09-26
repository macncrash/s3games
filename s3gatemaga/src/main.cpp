// S3 GATE MAGA
//   s3gatemaga                 play the watch
//   s3gatemaga --sim           autopilot holds the gate
//   s3gatemaga --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/gate.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        maga::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 30; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    maga::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool mid = false, hold = false, end = false;
    int frames = 0;
    const int limit = 60 * 45;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && cart.raidTime() > 7.f) {
            save(sys, "raid.png");
            mid = true;
        } else if (!hold && cart.raidTime() > 23.f) {
            save(sys, "hold.png");
            hold = true;
        }
        if (cart.over() && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    std::printf("S3 GATE MAGA  %s  rounds %d  stopped %d\n", cart.result(), cart.rounds(), cart.stopped());
    if (!cart.won()) std::fputs(cart.trace().c_str(), stdout);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 GATE MAGA %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3gatemaga [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<maga::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
