// S3 SCULL
//   s3scullbuoy                 round the buoys and beat the other crew home
//   s3scullbuoy --sim           the scull must finish inside the other crew's time
//   s3scullbuoy --sim --shots D also writes PNGs into D
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
    scull::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool mid = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (shotDir && frames == 30) save(sys, "dock.png");
        if (shotDir && !mid && cart.buoy() >= 2) {
            save(sys, "buoys.png");
            mid = true;
        }
    }
    if (shotDir) save(sys, "end.png");
    if (!cart.won()) {
        std::printf("S3 SCULL  FAIL  buoy %d  x %.1f  y %.1f  you %.2f  crew %.2f  (%.1f s)\n", cart.buoy(), cart.x(),
                    cart.y(), cart.you(), cart.crew(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    int you = int(cart.you() + 0.5f);
    int crew = int(cart.crew() + 0.5f);
    std::printf("S3 SCULL  HOME  beat the other crew  %d:%02d against %d:%02d\n", you / 60, you % 60, crew / 60,
                crew % 60);
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
            std::printf("S3 SCULL %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3scullbuoy [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<scull::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
