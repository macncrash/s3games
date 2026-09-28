// S3 SHUFFLEMARK
//   s3shufflemark                 play shuffle until a disk finishes the mark
//   s3shufflemark --sim           autopilot finishes the mark
//   s3shufflemark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/mark.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        shufflemark::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 12; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    shufflemark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool table = false;
    int frames = 0;
    const int limit = 60 * 25;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!table && frames == 8) {
            save(sys, "table.png");
            table = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.finished() && cart.onMark()) {
        std::printf("S3 SHUFFLEMARK  FINISHED MARK  disk on the mark  throws %d  (%.1f s)\n", cart.throws(),
                    frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 SHUFFLEMARK  OPEN  no finished mark  throws %d  (%.1f s)\n", cart.throws(), frames / 60.0);
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
            std::printf("S3 SHUFFLEMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3shufflemark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<shufflemark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
