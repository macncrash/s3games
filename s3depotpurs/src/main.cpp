// S3 DEPOT PURSUIT
//   s3depotpurs                 play
//   s3depotpurs --sim           autopilot stays the last machine running
//   s3depotpurs --sim --shots D also writes PNGs into D
#include "console/system.h"
#include "game/depot.h"
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
        depotpurs::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    depotpurs::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool sawYard = false, sawStop = false, sawEnd = false;
    int frames = 0;
    const int limit = 60 * 80;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 1 && !sawYard && frames > 40) {
            save(sys, "yard.png");
            sawYard = true;
        } else if (m == 2 && !sawStop) {
            save(sys, "stopped.png");
            sawStop = true;
        } else if (m == 3 && !sawEnd) {
            save(sys, "end.png");
            sawEnd = true;
        }
    }
    if (shotDir && !sawEnd) save(sys, "end.png");
    const char* word = cart.won() ? "LAST MACHINE STILL RUNNING" : cart.reason();
    std::printf("S3 DEPOT PURSUIT  %s  hull %d  stopped %d  (%.1f s)\n", word, cart.hull(), cart.stopped(),
                frames / 60.0);
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
            std::printf("S3 DEPOT PURSUIT %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3depotpurs [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<depotpurs::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
