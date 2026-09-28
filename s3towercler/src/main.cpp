// S3 TOWERCLER
//   s3towercler                 play
//   s3towercler --sim           the keeper clears the ground
//   s3towercler --sim --shots D also writes PNGs into D
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
        tower::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 30; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    tower::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool play = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!play && frames == 40) {
            save(sys, "yard.png");
            play = true;
        }
    }
    save(sys, cart.won() ? "clear.png" : "fail.png");
    std::printf("S3 TOWERCLER  %s  ground %s  left %d  clock %.1fs  (%.1f s)\n", cart.won() ? "WIN" : "FAIL",
                cart.won() ? "clear" : "foul", cart.left(), cart.clockLeft(), frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 TOWERCLER %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3towercler [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<tower::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
