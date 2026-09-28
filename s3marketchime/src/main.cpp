// S3 MARKETCHIME
//   s3marketchime                 play market until the hour chimes, then leave
//   s3marketchime --sim           autopilot, exits 0 only when the hour chimes
//   s3marketchime --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/market.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    marketchime::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    if (!cart.rules()) {
        std::printf("S3 MARKETCHIME  FAIL  RULES  (0.0 s)\n");
        std::fflush(stdout);
        return 1;
    }
    int frames = 0;
    const int limit = 60 * 20;
    bool titled = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 10) {
            save(sys, "title.png");
            titled = true;
        }
    }
    save(sys, "end.png");
    bool hour = cart.onTheHour() && cart.hour() == 12 && cart.minute() == 0 && cart.second() < marketchime::kGraceSec;
    if (cart.won() && hour && cart.served() >= 1 && cart.faults() < 2 && std::strcmp(cart.reason(), "CHIME") == 0) {
        std::printf("S3 MARKETCHIME  WIN  the hour chimes  %d:%02d:%02d  served %d  (%.1f s)\n", cart.hour(),
                    cart.minute(), cart.second(), cart.served(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 MARKETCHIME  FAIL  %s  %d:%02d:%02d  served %d  fault %d  (%.1f s)\n",
                cart.reason()[0] ? cart.reason() : "OPEN", cart.hour(), cart.minute(), cart.second(), cart.served(),
                cart.faults(), frames / 60.0);
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
            std::printf("S3 MARKETCHIME %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3marketchime [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<marketchime::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
