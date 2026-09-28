// S3 LANTERNCHIME
//   s3lanternchime                 play until the hour chimes
//   s3lanternchime --sim           autopilot, exits 0 only when the hour chimes
//   s3lanternchime --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/lanternchime.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    lanternchime::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false;
    int frames = 0;
    const int limit = 60 * 25;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 8) {
            save(sys, "title.png");
            titled = true;
        }
    }
    save(sys, "end.png");
    if (cart.won() && cart.hour() == 12 && cart.minute() == 0 && cart.second() == 0 && cart.lamps() == 4 &&
        std::strcmp(cart.reason(), "CHIME") == 0) {
        std::printf("S3 LANTERNCHIME  WIN  the hour chimes  %d:%02d:%02d  lamps %d  (%.1f s)\n", cart.hour(),
                    cart.minute(), cart.second(), cart.lamps(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 LANTERNCHIME  FAIL  %s  %d:%02d:%02d  lamps %d  strikes %d  (%.1f s)\n",
                cart.reason()[0] ? cart.reason() : "OPEN", cart.hour(), cart.minute(), cart.second(), cart.lamps(),
                cart.strikes(), frames / 60.0);
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
            std::printf("S3 LANTERNCHIME %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3lanternchime [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<lanternchime::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
