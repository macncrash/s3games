// S3 TOWER DOOR
//   s3towerdoor                 hold the tower door
//   s3towerdoor --sim           autopilot keeps the bar for three minutes
//   s3towerdoor --sim --shots D also writes PNGs into D
//
//   Left and Right pick a jamb. Hold A, B, C, or Space to bar it.
//   Enter starts. A climber on an unbarred jamb walks the door open.
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/tower.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    tower::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    if (shotDir) {
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }
    int frames = 0;
    const int limit = 60 * 210;
    bool mid = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (shotDir && !mid && cart.secondsLeft() < 150 && cart.secondsLeft() > 140) {
            save(sys, "door.png");
            mid = true;
        }
    }
    if (shotDir) save(sys, cart.won() ? "held.png" : "end.png");
    int sec = cart.secondsLeft();
    std::printf("S3 TOWER DOOR  %s  held %d:%02d  bar %d%%  (%.1f s)\n", cart.won() ? "PASS" : "FAIL",
                sec / 60, sec % 60, cart.barPct(), frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 TOWER DOOR %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3towerdoor [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<tower::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
