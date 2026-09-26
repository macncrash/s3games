// S3 FAIR SEVEN
//   s3fairseven                 play the booth, first to seven
//   s3fairseven --sim           autopilot tosses until you are first to seven
//   s3fairseven --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/fairseven.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    fairseven::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, flying = false;
    int frames = 0;
    const int limit = 60 * 25;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 10) {
            save(sys, "title.png");
            titled = true;
        }
        if (!flying && std::strcmp(cart.phase(), "flight") == 0) {
            save(sys, "toss.png");
            flying = true;
        }
    }
    save(sys, "end.png");
    if (cart.rules() && cart.won() && cart.shortSix() && cart.you() >= 7 && cart.them() < 7) {
        std::printf("S3 FAIR SEVEN  PASS  first to seven  you %d  them %d  (%.1f s)\n", cart.you(), cart.them(),
                    frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 FAIR SEVEN  SHORT  you %d  them %d  rules %d  six %d  phase %s  (%.1f s)\n", cart.you(),
                cart.them(), cart.rules() ? 1 : 0, cart.shortSix() ? 1 : 0, cart.phase(), frames / 60.0);
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
            std::printf("S3 FAIR SEVEN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3fairseven [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<fairseven::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
