// S3 EIGHT
//   s3eight                 play one rack
//   s3eight --sim           autopilot calls the pocket until the rack clears
//   s3eight --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/eight.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    eight::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, broke = false;
    int frames = 0;
    const int limit = 60 * 150;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 8) {
            save(sys, "title.png");
            titled = true;
        }
        if (!broke && frames == 50) {
            save(sys, "break.png");
            broke = true;
        }
    }
    save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 EIGHT  RACK CLEARED  8 IN %s  shots %d  score %d  (%.1f s)\n", cart.pocketName(), cart.shots(),
                    cart.score(), frames / 60.0);
        return 0;
    }
    std::printf("S3 EIGHT  OPEN  %s  shots %d  left %d  score %d  (%.1f s)\n", cart.reason(), cart.shots(), cart.left(),
                cart.score(), frames / 60.0);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 EIGHT %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3eight [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<eight::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
