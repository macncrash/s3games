// S3 GLIDER PASS
//   s3gliderpass                 clear the pass before the storm clock
//   s3gliderpass --sim           autopilot must clear the notch and the end tape
//   s3gliderpass --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/pass.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        gliderpass::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    gliderpass::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool climb = false, notch = false, lee = false, slot = false;
    int frames = 0;
    const int limit = 60 * 50;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!climb && m == 1 && frames > 24) {
            save(sys, "climb.png");
            climb = true;
        } else if (!notch && m == 2) {
            save(sys, "notch.png");
            notch = true;
        } else if (!lee && m == 3) {
            save(sys, "lee.png");
            lee = true;
        } else if (!slot && m == 4) {
            save(sys, "slot.png");
            slot = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf("S3 GLIDER PASS  FAIL  %s  x %.1f  alt %.2f  spd %.1f  (%.1f s)\n", why, cart.x(), cart.alt(),
                    cart.speed(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 GLIDER PASS  CLEAR  cleared the pass before the storm clock  (%.1f s)\n", cart.seconds());
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
            std::printf("S3 GLIDER PASS %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3gliderpass [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<gliderpass::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
