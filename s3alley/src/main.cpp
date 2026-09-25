// S3 ALLEY
//   s3alley                 walk the street
//   s3alley --sim           autopilot reaches the door
//   s3alley --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/alley.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        alley::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    alley::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool street = false, door = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 1 && !street && cart.along() > 90.f) {
            save(sys, "street.png");
            street = true;
        } else if (m == 2 && !door && cart.opened() > 0.55f) {
            save(sys, "door.png");
            door = true;
        }
    }
    if (!door) save(sys, "door.png");
    if (cart.won())
        std::printf("S3 ALLEY  the far door opens  stumbles %d  behind %.0f  (%.1f s)\n", cart.stumbles(), cart.behind(),
                    frames / 60.0);
    else
        std::printf("S3 ALLEY  the street keeps you  stumbles %d  behind %.0f  (%.1f s)\n", cart.stumbles(),
                    cart.behind(), frames / 60.0);
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
            std::printf("S3 ALLEY %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3alley [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<alley::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
