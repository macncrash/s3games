// S3 HELI BOOM
//   s3heliboom            play
//   s3heliboom --sim      autopilot delivers the drive to the boom
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/heli.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        heliboom::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    heliboom::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool flying = false, on = false;
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!flying && m >= 1 && frames > 20) {
            save(sys, "fly.png");
            flying = true;
        } else if (!on && m >= 2) {
            save(sys, "boom.png");
            on = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() && cart.why()[0] ? cart.why() : "timed out";
        std::printf("S3 HELI BOOM  FAIL  %s  (%.1f s)\n", why, frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 HELI BOOM  DELIVERED  the drive is on the boom  (%.1f s)\n", cart.seconds());
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
            std::printf("S3 HELI BOOM %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3heliboom [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<heliboom::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
