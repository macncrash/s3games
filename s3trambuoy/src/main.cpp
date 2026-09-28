// S3 TRAMBUOY
//   s3trambuoy                 round the buoys, same dock
//   s3trambuoy --sim           autopilot runs the course
//   s3trambuoy --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/tram.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    tram::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool mid = false;
    int frames = 0;
    const int limit = 60 * 110;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (shotDir && frames == 30) save(sys, "cast.png");
        if (shotDir && !mid && cart.buoys() >= 2) {
            save(sys, "buoys.png");
            mid = true;
        }
    }
    if (shotDir) save(sys, "end.png");
    int left = int(cart.clock());
    if (left < 0) left = 0;
    if (!cart.won()) {
        std::printf("S3 TRAMBUOY  FAIL  buoys %d  time %02d:%02d  (%.1f s)\n", cart.buoys(), left / 60, left % 60,
                    frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 TRAMBUOY  ROUNDED  back on the same dock  buoys 3  %02d:%02d left\n", left / 60, left % 60);
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
            std::printf("S3 TRAMBUOY %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3trambuoy [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<tram::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
