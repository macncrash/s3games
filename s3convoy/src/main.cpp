// S3 CONVOY
//   s3convoy                 play
//   s3convoy --sim           autopilot escorts the truck to the depot
//   s3convoy --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/convoy.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    convoy::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0, drive = 0;
    bool sawRoad = false;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (frames == 12) save(sys, "title.png");
        if (cart.phase() == 1 && ++drive == 36) {
            save(sys, "road.png");
            sawRoad = true;
        }
    }
    if (shotDir && !sawRoad) save(sys, "road.png");
    save(sys, "end.png");
    const char* msg = cart.won() ? "THE TRUCK ARRIVED" : "THE ROAD ENDS";
    std::printf("S3 CONVOY  %s  hull %d  score %d  (%.1f s)\n", msg, cart.hull(), cart.score(), cart.seconds());
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 CONVOY %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3convoy [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<convoy::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
