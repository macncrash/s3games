// S3 RAID
//   s3raid                 ride into the yard and clear it
//   s3raid --sim           autopilot rides in and clears the yard
//   s3raid --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/raid.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        raid::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    raid::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool shotRide = false, shotYard = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (shotDir && !shotRide && cart.phase() == 1 && frames == 70) {
            save(sys, "ride.png");
            shotRide = true;
        }
        if (shotDir && !shotYard && cart.phase() == 3) {
            save(sys, "yard.png");
            shotYard = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        std::printf("S3 RAID  FAIL  phase %d  cleared %d  hull %d  s %.0f  x %.2f  bike %.1f %.1f  mask %d  (%.1fs)\n",
                    cart.phase(), cart.cleared(), cart.hull(), cart.travel(), cart.lateral(), cart.bikeX(), cart.bikeY(),
                    cart.markMask(), cart.seconds());
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 RAID  YARD CLEAR  score %d  cleared %d  hull %d  (%.1fs)\n", cart.score(), cart.cleared(), cart.hull(),
                cart.seconds());
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
            std::printf("S3 RAID %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3raid [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<raid::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
