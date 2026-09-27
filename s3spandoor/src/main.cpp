// S3 SPANDOOR
//   s3spandoor                 hold the span door
//   s3spandoor --sim           autopilot holds it for three minutes
//   s3spandoor --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/span.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        spandoor::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 12; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    spandoor::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool span = false, end = false;
    int frames = 0;
    const int limit = 60 * 190;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (shotDir && !span && m == 1 && frames == 100) {
            save(sys, "span.png");
            span = true;
        } else if (shotDir && !end && (m == 2 || m == 3)) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (shotDir && !end) save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 SPANDOOR  THE DOOR HELD FOR THREE MINUTES  score %d\n", cart.score());
        return 0;
    }
    std::printf("S3 SPANDOOR  THE WATCH IS OVER  score %d\n", cart.score());
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 SPANDOOR %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3spandoor [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<spandoor::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
