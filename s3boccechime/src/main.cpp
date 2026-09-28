// S3 BOCCECHIME
//   s3boccechime            play a short bocce until the hour chimes, then leave
//   s3boccechime --sim      autopilot, exits 0 only when the hour chimes
#include <cstdio>
#include <cstring>
#include <memory>

#include "console/system.h"
#include "game/boccechime.h"
#include "version.h"

static int simulate() {
    gs::System sys(true);
    boccechime::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    bool hour = cart.onTheHour() && cart.hour() == 12 && cart.minute() == 0 && cart.second() < boccechime::kGraceSec;
    if (cart.won() && hour && cart.bowls() >= 1 && cart.bowls() <= 3 && std::strcmp(cart.reason(), "CHIME") == 0) {
        std::printf("S3 BOCCECHIME  WIN  the hour chimes  %d:%02d:%02d  bowl %d  (%.1f s)\n", cart.hour(),
                    cart.minute(), cart.second(), cart.bowls(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 BOCCECHIME  OPEN  %d:%02d:%02d  bowl %d  %s  (%.1f s)\n", cart.hour(), cart.minute(), cart.second(),
                cart.bowls(), cart.reason(), frames / 60.0);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv) {
    bool sim = false;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 BOCCECHIME %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3boccechime [--sim] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate();
    auto cart = std::make_unique<boccechime::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
