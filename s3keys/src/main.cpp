// S3 KEYS
//   s3keys                 play the song
//   s3keys --sim           autopilot holds the tune
//   s3keys --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/keys.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        keys::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    keys::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 45;
    bool mid = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && frames == 60 * 8) {
            save(sys, "play.png");
            mid = true;
        }
    }
    save(sys, "end.png");
    char line[160];
    if (cart.won())
        std::snprintf(line, sizeof line, "S3 KEYS  WIN  the tune holds  hits %d  misses %d", cart.hits(), cart.misses());
    else
        std::snprintf(line, sizeof line, "S3 KEYS  FAIL  the tune died  hits %d  misses %d", cart.hits(), cart.misses());
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
            std::printf("S3 KEYS %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3keys [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<keys::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
