// S3 FERRY
//   s3ferry                 dock before the tide clock
//   s3ferry --sim           autopilot must make the slip in time
//   s3ferry --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/ferry.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    ferry::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool mid = false;
    int frames = 0;
    const int limit = 60 * 110;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (shotDir && frames == 40) save(sys, "cast.png");
        if (shotDir && !mid && cart.y() > 28.f) {
            save(sys, "slip.png");
            mid = true;
        }
    }
    if (shotDir) save(sys, "end.png");
    if (!cart.won()) {
        std::printf("S3 FERRY  FAIL  %s  x %.1f  y %.1f  hdg %.2f  spd %.2f  tide %.1f  hold %.2f  phase %d  (%.1f s)\n",
                    cart.over() ? "tide out" : "timeout", cart.x(), cart.y(), cart.heading(), cart.speed(), cart.tideLeft(),
                    cart.hold(), cart.phase(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    int left = int(cart.tideLeft());
    if (left < 0) left = 0;
    std::printf("S3 FERRY  DOCKED  in the slip before the tide  %02d:%02d left\n", left / 60, left % 60);
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
            std::printf("S3 FERRY %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3ferry [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<ferry::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
