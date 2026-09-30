// S3 PAWNMARK
//   s3pawnmark                 walk the short pawn onto the mark
//   s3pawnmark --sim           autopilot finishes the mark
//   s3pawnmark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/pawnmark.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    pawnmark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool board = false;
    int frames = 0;
    const int limit = 60 * 20;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!board && frames == 8) {
            save(sys, "board.png");
            board = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.finished()) {
        std::printf("S3 PAWNMARK  FINISHED MARK  the short pawn held the mark  (%.1f s)\n", frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 PAWNMARK  OPEN  the mark did not finish  (%.1f s)\n", frames / 60.0);
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
            std::printf("S3 PAWNMARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3pawnmark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<pawnmark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
