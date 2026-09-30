// S3 QUILLBELL
//   s3quillbell                 ink the stroke until the bell rings
//   s3quillbell --sim           autopilot rings the bell before the third try dies
//   s3quillbell --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/quillbell.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    quillbell::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false;
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 8) {
            save(sys, "title.png");
            titled = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.dead() < 3 && cart.seals() >= cart.marks()) {
        std::printf(
            "S3 QUILLBELL  RUNG  the bell rings before the third try dies  dead %d  seals %d/%d  (%.1f s)\n",
            cart.dead(), cart.seals(), cart.marks(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 QUILLBELL  OPEN  the third try died  dead %d  seals %d/%d  (%.1f s)\n", cart.dead(),
                cart.seals(), cart.marks(), frames / 60.0);
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
            std::printf("S3 QUILLBELL %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3quillbell [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<quillbell::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
