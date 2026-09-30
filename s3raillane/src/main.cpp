// S3 RAILLANE
//   s3raillane                 hold the lane
//   s3raillane --sim           autopilot holds every leg
//   s3raillane --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/lane.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        lane::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 24; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    lane::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    bool rolling = false, mid = false;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!rolling && cart.marker() == 1 && frames > 30) {
            save(sys, "run.png");
            rolling = true;
        }
        if (!mid && cart.legsHeld() == 1) {
            save(sys, "leg.png");
            mid = true;
        }
    }
    save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 RAILLANE  LANE HELD  legs %d  (%.1f s)\n", cart.legsHeld(), frames / 60.0);
        return 0;
    }
    const char* why = cart.endNote()[0] ? cart.endNote() : "UNFINISHED";
    std::printf("S3 RAILLANE  LEG LOST  %s  legs %d  (%.1f s)\n", why, cart.legsHeld(), frames / 60.0);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 RAILLANE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3raillane [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<lane::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
