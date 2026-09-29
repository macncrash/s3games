// PALISADE WELL
//   s3palisadewell            keep the well
//   s3palisadewell --sim      the watch is fought through
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/well.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        palisade::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }
    gs::System sys(true);
    palisade::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool fight = false, mid = false, end = false;
    int frames = 0;
    const int limit = 60 * 120;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 1 && !fight && frames > 100) {
            save(sys, "watch.png");
            fight = true;
        } else if (m == 2 && cart.wave() == 2 && !mid) {
            save(sys, "last.png");
            mid = true;
        } else if (m == 3 && !end) {
            save(sys, "stood.png");
            end = true;
        }
    }
    if (!end) save(sys, "stood.png");
    std::printf("S3 PALISADE WELL  %s  score %d  wave %d  well %d  (%.1f s)\n", cart.won() ? "PASS" : "FAIL",
                cart.score(), cart.wave() + 1, cart.wellHp(), frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 PALISADE WELL %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3palisadewell [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<palisade::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
