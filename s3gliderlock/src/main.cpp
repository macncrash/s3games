// S3 GLIDER LOCK
//   s3gliderlock                 pass the lock
//   s3gliderlock --sim           autopilot must clear both gates and the end
//   s3gliderlock --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/glider.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        gliderlock::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    gliderlock::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool approach = false, throat = false, chamber = false, out = false;
    int frames = 0;
    const int limit = 60 * 70;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!approach && m == 1 && frames > 20) {
            save(sys, "approach.png");
            approach = true;
        } else if (!throat && m == 2) {
            save(sys, "throat.png");
            throat = true;
        } else if (!chamber && m == 3) {
            save(sys, "chamber.png");
            chamber = true;
        } else if (!out && m == 4) {
            save(sys, "out.png");
            out = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf("S3 GLIDER LOCK  FAIL  %s  x %.1f  alt %.2f  spd %.1f  (%.1f s)\n", why, cart.x(), cart.alt(),
                    cart.speed(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 GLIDER LOCK  CLEAR  passed the lock without scraping a gate  (%.1f s)\n", cart.seconds());
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
            std::printf("S3 GLIDER LOCK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3gliderlock [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<gliderlock::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
