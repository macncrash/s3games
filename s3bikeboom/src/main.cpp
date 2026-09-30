// S3 BIKE BOOM
//   s3bikeboom                 take the bike and deliver the drive
//   s3bikeboom --sim           autopilot must beat the other crew
//   s3bikeboom --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/boom.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        bikeboom::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    bikeboom::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool way = false, under = false, held = false;
    int frames = 0;
    const int limit = 60 * 70;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!way && m >= 1 && frames > 30) {
            save(sys, "bike.png");
            way = true;
        } else if (!under && m == 2) {
            save(sys, "boom.png");
            under = true;
        } else if (!held && m == 3) {
            save(sys, "handoff.png");
            held = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() && cart.why()[0] ? cart.why() : "timed out";
        std::printf("S3 BIKE BOOM  FAIL  %s  x %.1f spd %.2f left %.1f phase %d  (%.1f s)\n", why, cart.x(),
                    cart.speed(), cart.left(), cart.phase(), cart.seconds());
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 BIKE BOOM  DELIVERED  drive on the boom before the other crew  (%.1f s)\n", cart.seconds());
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
            std::printf("S3 BIKE BOOM %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3bikeboom [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<bikeboom::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
