// S3 SKI
//   s3ski                 ski the downhill
//   s3ski --sim           autopilot runs the gates
//   s3ski --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/ski.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    ski::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    bool shotRun = false, shotGate = false;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!shotRun && frames == 24) {
            save(sys, "title.png");
            shotRun = true;
        }
        if (shotDir && frames == 160) save(sys, "hill.png");
        if (!shotGate && cart.clean() >= 3) {
            save(sys, "gate.png");
            shotGate = true;
        }
    }
    save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 SKI  WIN  cleared the downhill  gates %d  misses %d  (%.1f s)\n", cart.clean(), cart.misses(),
                    frames / 60.0);
        return 0;
    }
    std::printf("S3 SKI  FAIL  misses %d  gates %d/%d  (%.1f s)\n", cart.misses(), cart.clean(), cart.gateCount(),
                frames / 60.0);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 SKI %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3ski [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<ski::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
