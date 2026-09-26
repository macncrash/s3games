// S3 DEPOT DOOR
//   s3depotdoor                 hold the depot door
//   s3depotdoor --sim           autopilot holds it for three minutes
//   s3depotdoor --sim --shots D also writes PNGs into D
#include "console/system.h"
#include "game/door.h"
#include "version.h"

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        depotdoor::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; ++i) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    depotdoor::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool door = false, end = false;
    int frames = 0;
    const int limit = 60 * 200;
    while (!cart.over() && frames < limit) {
        sys.step();
        ++frames;
        int m = cart.marker();
        if (m == 1 && !door && frames == 420) {
            save(sys, "door.png");
            door = true;
        } else if ((m == 2 || m == 3) && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    if (cart.won() && cart.watch() >= 179.5f) {
        std::printf("S3 DEPOT DOOR  THE DOOR HELD FOR THREE MINUTES\n");
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 DEPOT DOOR  THE DOOR OPENED  %s  open %.2f  arms %.2f  misses %d  (%.1f s)\n", cart.reason(),
                cart.openAmt(), cart.arms(), cart.misses(), frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 DEPOT DOOR %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3depotdoor [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<depotdoor::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
