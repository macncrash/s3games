// S3 HOOP
//   s3hoop                 play first to 21
//   s3hoop --sim           autopilot shoots until the rim counts 21
//   s3hoop --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/hoop.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    hoop::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, shot = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 8) {
            save(sys, "court.png");
            titled = true;
        }
        if (!shot && cart.you() + cart.lane() > 0) {
            save(sys, "count.png");
            shot = true;
        }
    }
    save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 HOOP  COUNT IT  %d-%d  (%.1f s)\n", cart.you(), cart.lane(), frames / 60.0);
        return 0;
    }
    std::printf("S3 HOOP  NO COUNT  %d-%d  (%.1f s)\n", cart.you(), cart.lane(), frames / 60.0);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 HOOP %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3hoop [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<hoop::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
