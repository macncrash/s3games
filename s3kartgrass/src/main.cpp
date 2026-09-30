// S3 KART GRASS
//   s3kartgrass                 take the kart, land, full stop
//   s3kartgrass --sim           autopilot beats the other crew
//   s3kartgrass --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/kart.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        kartgrass::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 10; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    kartgrass::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool way = false, grass = false, held = false;
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!way && m >= 1 && frames > 40) {
            save(sys, "kart.png");
            way = true;
        } else if (!grass && m == 2) {
            save(sys, "grass.png");
            grass = true;
        } else if (!held && m == 3) {
            save(sys, "hold.png");
            held = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() && cart.why()[0] ? cart.why() : "unfinished";
        std::printf("S3 KART GRASS  FAIL  %s  x %.1f y %.2f spd %.2f phase %d  (%.1f s)\n", why, cart.x(),
                    cart.altitude(), cart.speed(), cart.phase(), cart.seconds());
        std::fflush(stdout);
        return 1;
    }
    std::printf(
        "S3 KART GRASS  PASS  landed on the grass and came to a full stop ahead of the other crew  (%.1f s, %.1f s left)\n",
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
            std::printf("S3 KART GRASS %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3kartgrass [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<kartgrass::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
