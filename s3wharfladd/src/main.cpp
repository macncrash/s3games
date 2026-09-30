// S3 WHARF LADD
//   s3wharfladd                 play
//   s3wharfladd --sim           autopilot reaches the far ladder
//   s3wharfladd --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/ladd.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        wharfladd::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    wharfladd::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool near = false, dolly = false, far = false, end = false;
    int frames = 0;
    const int limit = 60 * 45;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!near && m == 1 && frames > 20) {
            save(sys, "wharf.png");
            near = true;
        } else if (!dolly && m == 2) {
            save(sys, "dolly.png");
            dolly = true;
        } else if (!far && m == 3) {
            save(sys, "far.png");
            far = true;
        } else if (!end && m == 4) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (!end) save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 WHARF LADD  reached the far ladder  (%.1f s)\n", frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 WHARF LADD  the wharf is over  %s  x %.0f  y %.0f  plat %d  (%.1f s)\n",
                cart.reason()[0] ? cart.reason() : "STILL ON THE WHARF", cart.heroX(), cart.heroY(), cart.heroPlat(),
                frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 WHARF LADD %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3wharfladd [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<wharfladd::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
