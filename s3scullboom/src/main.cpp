// S3 SCULL BOOM
//   s3scullboom                 play
//   s3scullboom --sim           autopilot delivers the drive
//   s3scullboom --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/scull.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        scullboom::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    scullboom::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool row = false, air = false, bed = false;
    int frames = 0;
    const int limit = 60 * 45;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!row && m >= 1 && frames > 30) {
            save(sys, "row.png");
            row = true;
        } else if (!air && m == 2) {
            save(sys, "heave.png");
            air = true;
        } else if (!bed && m == 3) {
            save(sys, "boom.png");
            bed = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf("S3 SCULL BOOM  FAIL  %s  x %.1f  (%.1f s)\n", why, cart.x(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 SCULL BOOM  DELIVERED  the drive is on the boom  (%.1f s)\n", cart.seconds());
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
            std::printf("S3 SCULL BOOM %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3scullboom [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<scullboom::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
