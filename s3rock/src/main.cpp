// S3 ROCK
//   s3rock                 play a minute of rocks
//   s3rock --sim           autopilot holds the channel; the hull is the score
//   s3rock --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/rock.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        rock::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 24; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    rock::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool mid = false;
    int frames = 0;
    const int limit = 60 * 70;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && frames == 120) {
            save(sys, "rapid.png");
            mid = true;
        }
    }
    save(sys, "end.png");
    return cart.report(frames);
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 ROCK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3rock [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<rock::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
