// S3 GLIDER TURN
//   s3gliderturn                 make the three turns, then the end
//   s3gliderturn --sim           autopilot banks the pylons and takes the gate
//   s3gliderturn --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/turn.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        gliderturn::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    gliderturn::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool glide = false, turn = false, run = false;
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!glide && m >= 1 && frames > 24) {
            save(sys, "glide.png");
            glide = true;
        } else if (!turn && m == 2) {
            save(sys, "turn.png");
            turn = true;
        } else if (!run && m == 3) {
            save(sys, "run.png");
            run = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.over() ? cart.why() : "timed out";
        std::printf(
            "S3 GLIDER TURN  FAIL  %s  x %.1f  alt %.1f  spd %.1f  bank %+.2f  turn %d  arc %.2f  (%.1f s)\n", why,
            cart.x(), cart.alt(), cart.speed(), cart.bank(), cart.turns(), cart.arc(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 GLIDER TURN  CLEAN  made the three turns without tipping  (%.1f s)\n", cart.seconds());
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
            std::printf("S3 GLIDER TURN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3gliderturn [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<gliderturn::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
