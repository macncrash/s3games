// S3 GLIDER BUOY
//   s3gliderbuoy                 fly the bay
//   s3gliderbuoy --sim           autopilot rounds the buoys and takes the dock
//   s3gliderbuoy --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/glider.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        gbuoy::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    gbuoy::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool out = false, rounding = false, home = false;
    int frames = 0;
    const int limit = 60 * 180;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!out && m >= 1 && frames > 20) {
            save(sys, "glider.png");
            out = true;
        } else if (!rounding && m == 2) {
            save(sys, "buoy.png");
            rounding = true;
        } else if (!home && m == 3) {
            save(sys, "dock.png");
            home = true;
        }
    }
    save(sys, "end.png");
    if (!cart.won()) {
        if (cart.over() && cart.report()[0]) std::printf("%s\n", cart.report());
        else
            std::printf(
                "S3 GLIDER BUOY  FAIL  timed out  leg %d  x %.0f  y %.0f  alt %.0f  hdg %.2f  spd %.0f  round %.2f  (%.1fs)\n",
                cart.leg(), cart.x(), cart.y(), cart.alt(), cart.heading(), cart.speed(), cart.roundProg(),
                frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 GLIDER BUOY %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3gliderbuoy [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<gbuoy::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
