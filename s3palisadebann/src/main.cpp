// S3 PALISADE BANN
//   s3palisadebann                 play
//   s3palisadebann --sim           autopilot brings the banner back
//   s3palisadebann --sim --shots D also writes PNGs into D
#include "console/system.h"
#include "game/bann.h"
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
        palbann::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    palbann::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool field = false, cloth = false, home = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 1 && !field && frames > 80) {
            save(sys, "field.png");
            field = true;
        } else if (m == 2 && !cloth) {
            save(sys, "banner.png");
            cloth = true;
        } else if (m == 3 && !home && cart.heroX() < 700.f) {
            save(sys, "home.png");
            home = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        std::printf("S3 PALISADE BANN  the watch is over  lives %d  carry %d  x %.0f  banner %.0f  watch %.1f  (%.1f s)\n",
                    cart.lives(), cart.carrying() ? 1 : 0, cart.heroX(), cart.bannerX(), cart.watchLeft(),
                    frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 PALISADE BANN  the banner is back at the palisade\n");
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
            std::printf("S3 PALISADE BANN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3palisadebann [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<palbann::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
