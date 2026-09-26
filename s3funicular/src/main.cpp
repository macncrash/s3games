// S3 FUNICULAR
//   s3funicular                 ride the counterbalanced car
//   s3funicular --sim           autopilot stops level with every platform
//   s3funicular --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/funicular.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    funicular::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);

    bool title = false, halt = false, doors = false, end = false;
    int frames = 0;
    const int limit = 60 * 150;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 0 && !title && frames > 6) {
            save(sys, "title.png");
            title = true;
        } else if (m == 2 && !halt && cart.cleared() >= 1) {
            save(sys, "platform.png");
            halt = true;
        } else if (m == 3 && !doors) {
            save(sys, "doors.png");
            doors = true;
        } else if (m == 4 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    if (!cart.over()) {
        std::printf("S3 FUNICULAR  FAIL  timed out\n");
        std::fflush(stdout);
        return 1;
    }
    std::printf("%s\n", cart.report());
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
            std::printf("S3 FUNICULAR %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3funicular [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<funicular::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
