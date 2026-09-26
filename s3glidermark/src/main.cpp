// S3 GLIDER MARK
//   s3glidermark                 set the wheel down on the mark
//   s3glidermark --sim           autopilot has to set down on the mark
//   s3glidermark --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/mark.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        gmark::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; ++i) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    gmark::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool way = false, over = false, settle = false;
    int frames = 0;
    const int limit = 60 * 30;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!way && m >= 1 && frames > 24) {
            save(sys, "glide.png");
            way = true;
        } else if (!over && m == 2) {
            save(sys, "mark.png");
            over = true;
        } else if (!settle && m == 3) {
            save(sys, "settle.png");
            settle = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf("S3 GLIDER MARK  FAIL  %s  x %.1f  alt %.2f  spd %.1f  vs %.2f  wheel %.1f  (%.1f s)\n", why,
                    cart.x(), cart.alt(), cart.speed(), cart.vs(), cart.wheel(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 GLIDER MARK  SET DOWN  the wheel is on the mark  (%.1f s)\n", cart.seconds());
    std::fflush(stdout);
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 GLIDER MARK %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3glidermark [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<gmark::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
