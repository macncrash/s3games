// S3 MAIL VAN
//   s3mailvankilo                 finish the kilometer without touching wheels
//   s3mailvankilo --sim           autopilot must beat the other crew's clock
//   s3mailvankilo --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/kilo.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        mailkilo::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    mailkilo::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool route = false, wheels = false, stretch = false;
    int frames = 0;
    const int limit = 60 * 100;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!route && m >= 1 && frames > 40) {
            save(sys, "route.png");
            route = true;
        } else if (!wheels && m == 2) {
            save(sys, "wheels.png");
            wheels = true;
        } else if (!stretch && m == 3) {
            save(sys, "stretch.png");
            stretch = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() && cart.why()[0] ? cart.why() : "timed out";
        std::printf("S3 MAIL VAN  FAIL  %s  %.0f m  (%.1f s)\n", why, cart.meters(), cart.seconds());
        std::fflush(stdout);
        return 1;
    }
    std::printf(
        "S3 MAIL VAN  KILO  finished the kilometer without touching wheels before the other crew  (%.1f s)\n",
        cart.seconds());
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
            std::printf("S3 MAIL VAN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3mailvankilo [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<mailkilo::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
