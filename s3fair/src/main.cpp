// S3 FAIR
//   s3fair                 walk the midway
//   s3fair --sim           autopilot leaves with a score
//   s3fair --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/fair.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    fair::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, ring = false, dart = false, bell = false, leave = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        int m = cart.marker();
        if (!titled && frames == 20) {
            save(sys, "title.png");
            titled = true;
        }
        if (!ring && m == 2) {
            save(sys, "ring.png");
            ring = true;
        }
        if (!dart && m == 3) {
            save(sys, "dart.png");
            dart = true;
        }
        if (!bell && m == 4) {
            save(sys, "bell.png");
            bell = true;
        }
        if (!leave && m == 6) {
            save(sys, "leave.png");
            leave = true;
        }
    }
    if (!leave) save(sys, "end.png");
    if (cart.won()) {
        std::printf("S3 FAIR  LEAVE  score %d  ring %d  dart %d  bell %d  (%.1f s)\n", cart.score(), cart.ring(),
                    cart.dart(), cart.bell(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 FAIR  FAIL  score %d  ring %d  dart %d  bell %d  phase %s  (%.1f s)\n", cart.score(), cart.ring(),
                cart.dart(), cart.bell(), cart.phase(), frames / 60.0);
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
            std::printf("S3 FAIR %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3fair [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<fair::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
