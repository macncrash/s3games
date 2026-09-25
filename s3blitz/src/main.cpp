// S3 BLITZ
//   s3blitz                 fly the trench
//   s3blitz --sim           autopilot must open the port
//   s3blitz --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/blitz.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    blitz::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, running = false;
    int frames = 0;
    const int limit = 60 * 100;
    while (!cart.over() && frames < limit) {
        sys.step();
        ++frames;
        if (!titled && frames == 8) {
            save(sys, "title.png");
            titled = true;
        }
        if (!running && frames == 90) {
            save(sys, "trench.png");
            running = true;
        }
    }
    save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 BLITZ  PORT  rounds %d  towers %d  cells %d  score %d  (%.1f s)\n", cart.rounds(), cart.towers(),
                    cart.cells(), cart.score(), frames / 60.0);
        return 0;
    }
    const char* why = cart.note()[0] ? cart.note() : "UNFINISHED";
    std::printf("S3 BLITZ  FAIL  %s  rounds %d  towers %d  cells %d  dist %.0f  (%.1f s)\n", why, cart.rounds(),
                cart.towers(), cart.cells(), cart.traveled(), frames / 60.0);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 BLITZ %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3blitz [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<blitz::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
