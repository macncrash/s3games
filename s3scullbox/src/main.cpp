// S3 SCULL BOX
//   s3scullbox                 stop the scull inside the box
//   s3scullbox --sim           autopilot stops inside the box
//   s3scullbox --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/scull.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    scullbox::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 60;
    bool mid = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (shotDir && frames == 40) save(sys, "row.png");
        if (shotDir && !mid && frames > 90) {
            save(sys, "box.png");
            mid = true;
        }
    }
    if (shotDir) save(sys, "end.png");
    if (!cart.won()) {
        std::printf("S3 SCULL BOX  FAIL  (%.1f s)\n", frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 SCULL BOX  STOPPED  inside the box  the leg is made  (%.1f s)\n", cart.seconds());
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
            std::printf("S3 SCULL BOX %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3scullbox [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<scullbox::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
