// S3 PUTT
//   s3putt                 play nine short holes
//   s3putt --sim           autopilot holes every cup
//   s3putt --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/putt.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        putt::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    putt::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool green = false;
    int frames = 0;
    const int limit = 60 * 50;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!green && cart.cups() >= 1) {
            save(sys, "green.png");
            green = true;
        }
    }
    if (shotDir) save(sys, "end.png");
    const bool won = cart.won() && cart.cups() == 9;
    if (!won) {
        std::printf("S3 PUTT  FAIL  cups %d  strokes %d  hole %s  ball %.1f %.1f  (%.1f s)\n", cart.cups(),
                    cart.strokes(), cart.holeName(), cart.ballX(), cart.ballY(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 PUTT  PASS  cups %d  strokes %d  (%.1f s)\n", cart.cups(), cart.strokes(), frames / 60.0);
    std::fflush(stdout);
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 PUTT %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3putt [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<putt::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
