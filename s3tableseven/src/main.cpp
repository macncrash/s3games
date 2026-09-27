// S3 TABLE SEVEN
//   s3tableseven                 play the table until first to seven
//   s3tableseven --sim           autopilot plays until you are first to seven
//   s3tableseven --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/tableseven.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    tableseven::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, shot = false;
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 8) {
            save(sys, "title.png");
            titled = true;
        }
        if (!shot && cart.live()) {
            save(sys, "rally.png");
            shot = true;
        }
    }
    save(sys, "end.png");
    if (cart.rules() && cart.won() && cart.you() >= 7 && cart.them() < 7) {
        std::printf("S3 TABLE SEVEN  PASS  first to seven  you %d  them %d  (%.1f s)\n", cart.you(), cart.them(),
                    frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 TABLE SEVEN  SHORT  you %d  them %d  rules %d  (%.1f s)\n", cart.you(), cart.them(),
                cart.rules() ? 1 : 0, frames / 60.0);
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
            std::printf("S3 TABLE SEVEN %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3tableseven [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<tableseven::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
