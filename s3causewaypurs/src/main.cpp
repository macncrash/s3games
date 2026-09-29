// S3 CAUSEWAY PURSUIT
//   s3causewaypurs                 play
//   s3causewaypurs --sim           autopilot is the last machine still running
//   s3causewaypurs --sim --shots D also writes PNGs into D
#include "console/system.h"
#include "game/purs.h"
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
        cwpurs::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; ++i) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    cwpurs::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool road = false, seize = false, end = false;
    int frames = 0;
    int playFrames = 0;
    const int limit = 60 * 50;
    while (!cart.over() && frames < limit) {
        sys.step();
        ++frames;
        int m = cart.marker();
        if (m == 1) {
            if (++playFrames == 70 && !road) {
                save(sys, "causeway.png");
                road = true;
            }
        } else if (m == 2 && !seize) {
            save(sys, "seize.png");
            seize = true;
        } else if (m == 3 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (shotDir && !end) save(sys, "end.png");
    if (!cart.over()) {
        std::printf("S3 CAUSEWAY PURSUIT  FAIL  THE CAUSEWAY RAN OUT  stalled %d/%d  boiler %d  (%.1f s)\n",
                    cart.stalled(), cart.fleet(), cart.boiler(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    if (cart.won() && cart.stalled() == cart.fleet() && cart.boiler() > 0) {
        std::printf("S3 CAUSEWAY PURSUIT  THE LAST MACHINE STILL RUNNING  stalled %d/%d  boiler %d  (%.1f s)\n",
                    cart.stalled(), cart.fleet(), cart.boiler(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 CAUSEWAY PURSUIT  FAIL  %s  stalled %d/%d  boiler %d  (%.1f s)\n", cart.reason(), cart.stalled(),
                cart.fleet(), cart.boiler(), frames / 60.0);
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
            std::printf("S3 CAUSEWAY PURSUIT %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3causewaypurs [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<cwpurs::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
