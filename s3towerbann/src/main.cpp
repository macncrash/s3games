// S3 TOWER BANN
//   s3towerbann                 play
//   s3towerbann --sim           autopilot brings the banner back
//   s3towerbann --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/tower.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        towerbann::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    towerbann::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool climb = false, carry = false, down = false;
    int frames = 0;
    const int limit = 60 * 120;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 1 && !climb && cart.floor() >= 3) {
            save(sys, "climb.png");
            climb = true;
        } else if (m == 2 && !carry) {
            save(sys, "banner.png");
            carry = true;
        } else if (m == 3 && !down && cart.floor() <= 3) {
            save(sys, "down.png");
            down = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        std::printf("S3 TOWER BANN  still in the tower  lives %d  carry %d  floor %d  (%.1f s)\n", cart.lives(),
                    cart.carrying() ? 1 : 0, cart.floor() + 1, frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 TOWER BANN  the banner is back  then it is done\n");
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
            std::printf("S3 TOWER BANN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3towerbann [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<towerbann::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
