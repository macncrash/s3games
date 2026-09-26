// S3 SLED PASS
//   s3sledpass                 clear the pass
//   s3sledpass --sim           autopilot must beat the storm clock
//   s3sledpass --sim --shots D also writes PNGs into D
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

    gs::System sys(true);
    sledpass::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);

    bool title = false, run = false, closing = false, end = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 0 && !title && frames > 8) {
            save(sys, "title.png");
            title = true;
        } else if (m == 2 && !run && cart.meters() > 220.f) {
            save(sys, "pass.png");
            run = true;
        } else if (m == 3 && !closing) {
            save(sys, "storm.png");
            closing = true;
        } else if (m == 4 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf("S3 SLED PASS  FAIL  %s  z %.0f  x %.2f  spd %.1f  clock %.1f  (%.1f s)\n", why, cart.meters(),
                    cart.x(), cart.speed(), cart.clockLeft(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 SLED PASS  CLEAR  cleared the pass before the storm clock  (%.1f s, %.1f s left)\n",
                cart.seconds(), cart.clockLeft());
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
            std::printf("S3 SLED PASS %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3sledpass [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<sledpass::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
