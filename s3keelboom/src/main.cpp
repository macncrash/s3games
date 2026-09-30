// S3 KEEL BOOM
//   s3keelboom                 play
//   s3keelboom --sim           autopilot delivers the drive
//   s3keelboom --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/keel.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        keelboom::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    keelboom::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool leg = false, nest = false, hold = false, end = false;
    int frames = 0;
    const int limit = 60 * 120;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!leg && m >= 1 && frames > 50) {
            save(sys, "leg.png");
            leg = true;
        } else if (!nest && m == 2) {
            save(sys, "boom.png");
            nest = true;
        } else if (!hold && m == 3) {
            save(sys, "hold.png");
            hold = true;
        } else if (!end && m == 4) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf("S3 KEEL BOOM  FAIL  %s  x %.1f  y %.1f  hdg %.0f  spd %.2f  (%.1f s)\n", why, cart.x(), cart.y(),
                    cart.heading() * 57.2958f, cart.speed(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 KEEL BOOM  DELIVERED  the drive is on the boom  (%.1f s)\n", cart.seconds());
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
            std::printf("S3 KEEL BOOM %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3keelboom [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<keelboom::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
