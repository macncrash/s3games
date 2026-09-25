// S3 MEMORY
//   s3memory                 play the table
//   s3memory --sim           autopilot matches the table
//   s3memory --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/memory.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        memo::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 4; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    memo::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 40;
    bool mid = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && cart.pairs() >= 2) {
            save(sys, "play.png");
            mid = true;
        }
    }
    save(sys, "end.png");
    char line[160];
    if (cart.won()) {
        std::snprintf(line, sizeof line, "S3 MEMORY  WIN  matched the table  pairs %d/8  turns %d  clock %.1fs",
                      cart.pairs(), cart.turns(), cart.clockLeft());
    } else if (cart.over()) {
        std::snprintf(line, sizeof line, "S3 MEMORY  FAIL  the clock won  pairs %d/8  turns %d  clock %.1fs",
                      cart.pairs(), cart.turns(), cart.clockLeft());
    } else {
        std::snprintf(line, sizeof line, "S3 MEMORY  FAIL  unfinished  pairs %d/8  turns %d  clock %.1fs",
                      cart.pairs(), cart.turns(), cart.clockLeft());
    }
    std::printf("%s\n", line);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 MEMORY %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3memory [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<memo::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
