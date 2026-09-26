// S3 CURL GOLD
//   s3curlgold                 one end; only the gold counts double
//   s3curlgold --sim           autopilot, exits 0 only on the double
//   s3curlgold --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/curlgold.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        curlgold::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    curlgold::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool slid = false;
    int frames = 0;
    const int limit = 60 * 50;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!slid && cart.sliding()) {
            save(sys, "slide.png");
            slid = true;
        }
    }
    save(sys, "end.png");
    bool math = cart.red() == cart.gold() * 2 + cart.cream();
    if (cart.won() && cart.delivered() == 8 && cart.gold() >= 1 && cart.red() > cart.yel() && math) {
        std::printf("S3 CURL GOLD  DOUBLE  only the gold counts double  red %d  yel %d  gold %d  cream %d  (%.1f s)\n",
                    cart.red(), cart.yel(), cart.gold(), cart.cream(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 CURL GOLD  SHORT  red %d  yel %d  gold %d  cream %d  lie %.2f %.2f  rocks %d  (%.1f s)\n",
                cart.red(), cart.yel(), cart.gold(), cart.cream(), cart.redLie(), cart.yelLie(), cart.delivered(),
                frames / 60.0);
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
            std::printf("S3 CURL GOLD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3curlgold [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<curlgold::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
