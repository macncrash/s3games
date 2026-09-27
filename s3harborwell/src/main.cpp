// S3 HARBOR WELL
//   s3harborwell                 play
//   s3harborwell --sim           autopilot holds the well through three waves
//   s3harborwell --sim --shots D also writes PNGs into D
#include "console/system.h"
#include "game/well.h"
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
        well::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    well::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool sawWave = false, sawStand = false, sawEnd = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 1 && !sawWave && frames > 40) {
            save(sys, "wave.png");
            sawWave = true;
        } else if (m == 2 && !sawStand) {
            save(sys, "stand.png");
            sawStand = true;
        } else if (m == 3 && !sawEnd) {
            save(sys, "end.png");
            sawEnd = true;
        }
    }
    if (shotDir && !sawEnd) save(sys, "end.png");
    std::printf("S3 HARBOR WELL  %s  stones %d  wave %d  score %d  (%.1f s)\n", cart.won() ? "PASS" : "FAIL",
                cart.stones(), cart.wave() + 1, cart.score(), frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 HARBOR WELL %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3harborwell [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<well::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
