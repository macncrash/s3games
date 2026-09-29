// S3 MAIL VAN
//   s3mailvanplat                 stop level with the platform
//   s3mailvanplat --sim           autopilot must stop level with the dock
//   s3mailvanplat --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/plat.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        mailplat::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    mailplat::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool street = false, dock = false, held = false;
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!street && m >= 1 && frames > 20) {
            save(sys, "street.png");
            street = true;
        } else if (!dock && m == 2) {
            save(sys, "platform.png");
            dock = true;
        } else if (!held && m == 3) {
            save(sys, "level.png");
            held = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() && cart.why()[0] ? cart.why() : "timed out";
        std::printf("S3 MAIL VAN  FAIL  %s  door %.2f  bed %.2f  spd %.2f  (%.1f s)\n", why, cart.x(), cart.deck(),
                    cart.speed(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 MAIL VAN  LEVEL  stopped level with the platform  (%.1f s)\n", cart.seconds());
    std::fflush(stdout);
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 MAIL VAN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3mailvanplat [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<mailplat::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
