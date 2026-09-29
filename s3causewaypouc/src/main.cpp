// S3 CAUSEWAY POUC
//   s3causewaypouc                 play
//   s3causewaypouc --sim           autopilot carries the pouch across
//   s3causewaypouc --sim --shots D also writes PNGs into D
#include "console/system.h"
#include "game/pouc.h"
#include "version.h"

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        cwpouc::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 40; ++i) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    cwpouc::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool road = false, carry = false, end = false;
    int frames = 0;
    int playFrames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        ++frames;
        int m = cart.marker();
        if (m == 1) {
            if (++playFrames == 30 && !road) {
                save(sys, "causeway.png");
                road = true;
            }
        } else if (m == 2 && !carry) {
            save(sys, "carry.png");
            carry = true;
        } else if ((m == 3 || m == 4) && !end) {
            save(sys, "end.png");
            end = true;
        }
    }
    if (shotDir && !end) save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 CAUSEWAY POUC  THE POUCH CROSSED  (%.1f s)\n", frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    const char* why = cart.over() ? cart.reason() : "THE CAUSEWAY RAN OUT";
    std::printf("S3 CAUSEWAY POUC  FAIL  %s  (%.1f s)\n", why, frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 CAUSEWAY POUC %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3causewaypouc [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<cwpouc::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
