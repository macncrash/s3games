// S3 FISH
//   s3fish            fish the day
//   s3fish --sim      autopilot lands five keepers before dark
//   s3fish --sim --shots D   also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/fish.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        fish::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    fish::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 90;
    bool didFight = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (shotDir && !didFight && cart.fighting()) {
            save(sys, "fight.png");
            didFight = true;
        }
    }
    if (shotDir) save(sys, cart.won() ? "win.png" : "end.png");

    if (cart.won() && cart.creelFull() && cart.keepers() == 5 && cart.seconds() <= cart.day()) {
        std::printf("S3 FISH  FIVE KEEPERS  daylight held  BASS TROUT PIKE WALLEYE CATFISH  (%.1f s)\n",
                    cart.seconds());
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 FISH  FAIL  keepers %d/5  creel %d  won %d  over %d  kept %d%d%d%d%d  (%.1f s)\n",
                cart.keepers(), cart.creelFull() ? 1 : 0, cart.won() ? 1 : 0, cart.over() ? 1 : 0, cart.kept(0),
                cart.kept(1), cart.kept(2), cart.kept(3), cart.kept(4), cart.seconds());
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 FISH %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3fish [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<fish::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
