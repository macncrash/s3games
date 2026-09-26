// S3 TUGBOAT LOCK
//   s3tugboatlock                 take the tug through the lock
//   s3tugboatlock --sim           autopilot must clear both gates ahead of the other crew
//   s3tugboatlock --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/tug.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        tuglock::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    tuglock::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool throat = false, rise = false, out = false;
    int frames = 0;
    const int limit = 60 * 140;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!throat && m == 2) {
            save(sys, "throat.png");
            throat = true;
        } else if (!rise && m == 3) {
            save(sys, "rise.png");
            rise = true;
        } else if (!out && m == 4) {
            save(sys, "out.png");
            out = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf(
            "S3 TUGBOAT LOCK  FAIL  %s  x %.2f y %.1f hdg %.0f spd %.2f lo %.2f hi %.2f phase %d crew %.1f (%.1fs)\n",
            why, cart.x(), cart.y(), cart.heading() * 57.2958f, cart.speed(), cart.lowerOpen(), cart.upperOpen(),
            cart.phase(), cart.crewLeft(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf(
        "S3 TUGBOAT LOCK  CLEAR  passed the lock without scraping a gate ahead of the other crew (%.1fs, %.1fs left)\n",
        cart.seconds(), cart.crewLeft());
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
            std::printf("S3 TUGBOAT LOCK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3tugboatlock [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<tuglock::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
