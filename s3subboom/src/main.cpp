// S3 SUB BOOM
//   s3subboom                 dive the leg
//   s3subboom --sim           autopilot sets the drive on the boom
//   s3subboom --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/sub.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        subboom::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    subboom::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool leg = false, boom = false, held = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!leg && m >= 1 && frames > 50) {
            save(sys, "leg.png");
            leg = true;
        } else if (!boom && m == 2) {
            save(sys, "boom.png");
            boom = true;
        } else if (!held && m == 3) {
            save(sys, "hold.png");
            held = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf("S3 SUB BOOM  FAIL  %s  x %.1f y %.1f spd %.2f phase %d  (%.1f s)\n", why, cart.x(), cart.y(),
                    cart.speed(), cart.phase(), cart.seconds());
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 SUB BOOM  DELIVERED  the drive is on the boom  (%.1f s)\n", cart.seconds());
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
            std::printf("S3 SUB BOOM %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3subboom [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<subboom::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
