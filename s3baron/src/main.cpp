// S3 BARON
//   s3baron                 play
//   s3baron --sim           autopilot flies the sortie
//   s3baron --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/baron.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        baron::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    baron::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool fly = false, clear = false, ace = false, end = false;
    int frames = 0, flyFrames = 0;
    const int limit = 60 * 200;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 1) {
            if (++flyFrames == 80 && !fly) {
                save(sys, "fight.png");
                fly = true;
            }
        } else if (m == 2 && !clear) {
            save(sys, "clear.png");
            clear = true;
        } else if (m == 3 && !ace) {
            save(sys, "circus.png");
            ace = true;
        } else if (m == 4 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    std::printf("S3 BARON  %s  score %d  wave %d  lives %d  (%.1f s)\n", cart.won() ? "PASS" : "FAIL", cart.score(),
                cart.wave() + 1, cart.lives(), frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 BARON %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3baron [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<baron::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
