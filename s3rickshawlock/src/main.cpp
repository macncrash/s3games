// S3 RICKSHAW LOCK
//   s3rickshawlock                 pedal the rickshaw through the lock
//   s3rickshawlock --sim           autopilot must clear both gates without a scrape
//   s3rickshawlock --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/lock.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        ricklock::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    ricklock::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool throat = false, rise = false, out = false;
    int frames = 0;
    const int limit = 60 * 80;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!throat && m == 2) {
            save(sys, "throat.png");
            throat = true;
        } else if (!rise && m == 2 && cart.speed() < 2.f && frames > 90) {
            save(sys, "rise.png");
            rise = true;
        } else if (!out && m == 3) {
            save(sys, "out.png");
            out = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf("S3 RICKSHAW LOCK  FAIL  %s  x %.1f  y %.1f  hdg %.0f  spd %.1f  (%.1f s)\n", why, cart.x(),
                    cart.y(), cart.heading() * 57.2958f, cart.speed(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 RICKSHAW LOCK  CLEAR  passed the lock without scraping a gate  (%.1f s)\n", cart.seconds());
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
            std::printf("S3 RICKSHAW LOCK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3rickshawlock [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<ricklock::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
