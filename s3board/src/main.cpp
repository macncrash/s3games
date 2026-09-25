// S3 BOARD
//   s3board                 play the night desk
//   s3board --sim           autopilot patches the shift
//   s3board --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/board.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        board::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; i++) sys.step();
        sys.render();
        save(sys, "title.png");
    }

    gs::System sys(true);
    board::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool mid = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && cart.marker() == 1 && frames > 20) {
            save(sys, "board.png");
            mid = true;
        }
    }
    sys.render();
    save(sys, cart.won() ? "win.png" : "end.png");
    const char* verdict = cart.won() ? "WIN" : "FAIL";
    const char* line = cart.won() ? "the lines held" : "the lines dropped";
    std::printf("S3 BOARD  %s  %s  connected %d  drops %d  (%.1f s)\n", verdict, line, cart.made(), cart.drops(),
                frames / 60.0);
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
            std::printf("S3 BOARD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3board [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<board::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
