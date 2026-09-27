// S3 FERRY PASS
//   s3ferrypass                 clear the pass before the storm clock
//   s3ferrypass --sim           autopilot must reach the end tape in time
//   s3ferrypass --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/ferry.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        ferrypass::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    ferrypass::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool mid = false;
    int frames = 0;
    const int limit = 60 * 80;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (shotDir && !mid && cart.y() > 90.f) {
            save(sys, "notch.png");
            mid = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf("S3 FERRY PASS  FAIL  %s  x %.1f  y %.1f  hdg %.2f  spd %.2f  (%.1f s)\n", why, cart.x(), cart.y(),
                    cart.heading(), cart.speed(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    int left = int(cart.stormLeft() + 0.5f);
    if (left < 0) left = 0;
    std::printf("S3 FERRY PASS  CLEAR  cleared the pass before the storm clock  %02d:%02d left\n", left / 60, left % 60);
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
            std::printf("S3 FERRY PASS %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3ferrypass [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<ferrypass::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
