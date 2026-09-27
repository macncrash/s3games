// S3 FERRY SLIP
//   s3ferryslip                 berth before the other crew's tide
//   s3ferryslip --sim           autopilot must make the slip in time
//   s3ferryslip --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/slip.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    ferryslip::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool mid = false;
    int frames = 0;
    const int limit = 60 * 100;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (shotDir && frames == 30) save(sys, "cast.png");
        if (shotDir && !mid && cart.y() > 16.f) {
            save(sys, "slip.png");
            mid = true;
        }
    }
    if (shotDir) save(sys, "end.png");
    if (!cart.won()) {
        std::printf("S3 FERRY SLIP  FAIL  %s  x %.1f  y %.1f  hdg %.2f  spd %.2f  crew %.1f  hold %.2f  (%.1f s)\n",
                    cart.over() ? "tide turned" : "timeout", cart.x(), cart.y(), cart.heading(), cart.speed(),
                    cart.crewLeft(), cart.hold(), frames / 60.0);
        std::fflush(stdout);
        return 1;
    }
    int left = int(cart.crewLeft());
    if (left < 0) left = 0;
    std::printf("S3 FERRY SLIP  BERTHED  in the slip before the tide turned  %02d:%02d on the other crew\n", left / 60,
                left % 60);
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
            std::printf("S3 FERRY SLIP %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3ferryslip [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<ferryslip::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
