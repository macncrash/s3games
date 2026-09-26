// S3 PUTT SEVEN
//   s3puttseven                 play putt until first to seven
//   s3puttseven --sim           autopilot putts until you are first to seven
//   s3puttseven --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/puttseven.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        puttseven::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    puttseven::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool rolling = false;
    int frames = 0;
    const int limit = 60 * 100;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!rolling && cart.strokes() >= 1) {
            save(sys, "roll.png");
            rolling = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.you() >= 7 && cart.them() < 7) {
        std::printf("S3 PUTT SEVEN  PASS  first to seven  you %d  them %d  (%.1f s)\n", cart.you(), cart.them(),
                    frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 PUTT SEVEN  SHORT  you %d  them %d  putts %d  (%.1f s)\n", cart.you(), cart.them(), cart.strokes(),
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
            std::printf("S3 PUTT SEVEN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3puttseven [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<puttseven::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
