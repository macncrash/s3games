// S3 CRANE TURN
//   s3craneturn             drive the three turns
//   s3craneturn --sim       autopilot clears the yard ahead of the other crew
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/turn.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        craneturn::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 20; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    craneturn::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 90;
    bool mid = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && frames == 120) {
            save(sys, "turn.png");
            mid = true;
        }
    }
    save(sys, "end.png");
    if (cart.won())
        std::printf("S3 CRANETURN  WIN  three turns clear, the other crew still on the clock\n");
    else if (!cart.over())
        std::printf("S3 CRANETURN  FAIL  the shift ran out\n");
    else
        std::printf("S3 CRANETURN  FAIL  %s\n", cart.reason());
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 CRANE TURN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3craneturn [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<craneturn::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
