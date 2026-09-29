// S3 LOOMCHIME
//   s3loomchime                 play loom until the hour chimes, then leave
//   s3loomchime --sim           autopilot, exits 0 only when the hour chimes
//   s3loomchime --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/loomchime.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    loomchime::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 25;
    bool mid = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (frames == 8) save(sys, "title.png");
        if (!mid && cart.dead() == 1 && !cart.won()) {
            save(sys, "loom.png");
            mid = true;
        }
    }
    save(sys, "end.png");
    bool hour = cart.onTheHour() && cart.hour() == 12 && cart.minute() == 0 && cart.second() < loomchime::kGraceSec;
    if (cart.won() && hour && std::strcmp(cart.reason(), "CHIME") == 0 && cart.laid() == 1 && cart.early() == 0 &&
        cart.dead() == 1 && cart.spent() == 2) {
        std::printf("S3 LOOMCHIME  WIN  the hour chimes  %d:%02d:%02d  pick %d  shuttle %d  (%.1f s)\n", cart.hour(),
                    cart.minute(), cart.second(), cart.laid(), cart.spent(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 LOOMCHIME  FAIL  %s  %d:%02d:%02d  laid %d  early %d  dead %d  spent %d  (%.1f s)\n",
                cart.reason()[0] ? cart.reason() : "OPEN", cart.hour(), cart.minute(), cart.second(), cart.laid(),
                cart.early(), cart.dead(), cart.spent(), frames / 60.0);
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
            std::printf("S3 LOOMCHIME %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3loomchime [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<loomchime::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
