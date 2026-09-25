// S3 HARBOR
//   s3harbor                 steam the channel
//   s3harbor --sim           autopilot runs the forts
//   s3harbor --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/harbor.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        harbor::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    harbor::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    bool mid = false;
    const int limit = 60 * 50;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && frames == 180) {
            save(sys, "channel.png");
            mid = true;
        }
    }
    save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 HARBOR  CHANNEL CLEAR  forts %d/%d  magazine %d  hull %d  (%.1f s)\n", cart.silenced(),
                    cart.batteries(), cart.magazine(), cart.hull(), frames / 60.0);
    } else {
        std::printf("S3 HARBOR  FAIL  %s  forts %d/%d  magazine %d  hull %d  (%.1f s)\n", cart.cause(), cart.silenced(),
                    cart.batteries(), cart.magazine(), cart.hull(), frames / 60.0);
    }
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
            std::printf("S3 HARBOR %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3harbor [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<harbor::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
