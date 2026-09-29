// S3 CRANEPLAT
//   s3craneplat                 take the crane
//   s3craneplat --sim           autopilot stops level with the platform
//   s3craneplat --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/plat.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        craneplat::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 30; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    craneplat::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool mid = false;
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && frames == 70) {
            save(sys, "shift.png");
            mid = true;
        }
    }
    save(sys, "end.png");
    if (cart.won()) std::printf("S3 CRANEPLAT  WIN  stopped level with the platform ahead of the other crew\n");
    else if (!cart.over()) std::printf("S3 CRANEPLAT  FAIL  the shift ran out\n");
    else std::printf("S3 CRANEPLAT  FAIL  %s\n", cart.reason());
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 CRANEPLAT %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3craneplat [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<craneplat::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
