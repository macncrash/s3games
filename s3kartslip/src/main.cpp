// S3 KARTSLIP
//   s3kartslip                 play
//   s3kartslip --sim           autopilot berths the kart
//   s3kartslip --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/slip.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    slip::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool title = false, race = false, end = false;
    int frames = 0;
    const int limit = 60 * 80;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 0 && frames == 8 && !title) {
            save(sys, "title.png");
            title = true;
        } else if (m == 1 && frames == 180 && !race) {
            save(sys, "quay.png");
            race = true;
        } else if ((m == 2 || m == 3) && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    std::printf("S3 KARTSLIP  %s  tide %.1fs left  crew %.0f behind  (%.1f s)\n",
                cart.won() ? "berthed before the tide" : "FAIL", cart.tideLeft(), cart.crewBehind(), frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 KARTSLIP %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3kartslip [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<slip::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
