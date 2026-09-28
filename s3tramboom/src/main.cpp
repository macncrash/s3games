// S3 TRAM BOOM
//   s3tramboom                 ride the leg
//   s3tramboom --sim           autopilot delivers the drive
//   s3tramboom --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/tram.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        tramboom::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 30; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    tramboom::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool leg = false, boom = false, slide = false, end = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (m == 1 && frames == 70 && !leg) {
            save(sys, "leg.png");
            leg = true;
        } else if (m == 2 && !boom) {
            save(sys, "boom.png");
            boom = true;
        } else if (m == 3 && !slide) {
            save(sys, "drive.png");
            slide = true;
        } else if (m == 4 && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 TRAM BOOM  DELIVERED  the drive is on the boom  the leg is made  (%.1f s)\n", cart.seconds());
        return 0;
    }
    std::printf("S3 TRAM BOOM  FAILED  %s  (%.1f s)\n", cart.why(), cart.seconds());
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 TRAM BOOM %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3tramboom [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<tramboom::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
