// S3 THERM
//   One envelope. The mark, not the trees.
//   s3therm                 fly the bag
//   s3therm --sim           autopilot sets the basket on the mark
//   s3therm --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/therm.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        therm::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 24; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    therm::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool fly = false, mark = false, end = false;
    int frames = 0;
    const int limit = 60 * 150;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 2 && !fly) {
            save(sys, "fly.png");
            fly = true;
        } else if (m == 3 && !mark) {
            save(sys, "mark.png");
            mark = true;
        } else if (m == 4 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    if (!cart.over()) {
        std::printf("S3 THERM  FAIL  STILL IN THE AIR\n");
        std::fflush(stdout);
        return 1;
    }
    std::printf("%s\n", cart.line());
    std::fflush(stdout);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 THERM %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3therm [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<therm::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
