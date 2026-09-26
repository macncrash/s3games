// S3 SOLITAIRE
//   s3solitaire                 play the one deal
//   s3solitaire --sim           autopilot clears the tableau
//   s3solitaire --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/solitaire.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        sol::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    sol::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 40;
    bool mid = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (frames == 40) save(sys, "deal.png");
        if (!mid && cart.home() >= 6) {
            save(sys, "play.png");
            mid = true;
        }
    }
    save(sys, "end.png");
    char line[180];
    if (cart.won()) {
        std::snprintf(line, sizeof line, "S3 SOLITAIRE  WIN  cleared the tableau  moves %d  home %d/28  (%.1f s)",
                      cart.moves(), cart.home(), frames / 60.0);
    } else if (cart.over()) {
        std::snprintf(line, sizeof line, "S3 SOLITAIRE  FAIL  tableau remains  moves %d  home %d/28  (%.1f s)",
                      cart.moves(), cart.home(), frames / 60.0);
    } else {
        std::snprintf(line, sizeof line, "S3 SOLITAIRE  FAIL  unfinished  moves %d  home %d/28  (%.1f s)", cart.moves(),
                      cart.home(), frames / 60.0);
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
            std::printf("S3 SOLITAIRE %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3solitaire [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<sol::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
