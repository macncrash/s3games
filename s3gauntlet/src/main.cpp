// S3 GAUNTLET
//   s3gauntlet                 play
//   s3gauntlet --sim           autopilot walks the corridor and opens the door
//   s3gauntlet --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/gauntlet.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [](gs::System& sys, const std::string& path) {
        sys.render();
        sys.saveScreenshot(path);
    };
    if (shotDir) {
        gs::System sys(true);
        gauntlet::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 24; i++) sys.step();
        save(sys, std::string(shotDir) + "/title.png");
    }

    gs::System sys(true);
    gauntlet::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool mid = false;
    bool door = false;
    int frames = 0;
    const int limit = 60 * 150;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (shotDir && !mid && frames == 150) {
            save(sys, std::string(shotDir) + "/corridor.png");
            mid = true;
        }
        if (shotDir && cart.won() && !door) {
            save(sys, std::string(shotDir) + "/door.png");
            door = true;
        }
    }
    if (shotDir && cart.won() && !door) save(sys, std::string(shotDir) + "/door.png");
    std::printf("S3 GAUNTLET  %s  score %d  lives %d  hearts %d  (%.1f s)\n",
                cart.won() ? "the door opens" : "the corridor holds", cart.score(), cart.lives(), cart.hearts(),
                frames / 60.0);
    return cart.won() ? 0 : 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 GAUNTLET %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3gauntlet [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<gauntlet::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
