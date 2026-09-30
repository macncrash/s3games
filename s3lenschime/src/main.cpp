// S3 LENSCHIME
//   s3lenschime                 play the lens until the hour chimes, then leave
//   s3lenschime --sim           autopilot, exits 0 only when the hour chimes
//   s3lenschime --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/lenschime.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    lenschime::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    save(sys, "boot.png");
    int frames = 0;
    const int limit = 60 * 20;
    bool aimed = false;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (frames == 4) save(sys, "title.png");
        if (!aimed && cart.hour() == 11 && frames > 40) {
            save(sys, "lens.png");
            aimed = true;
        }
    }
    save(sys, "end.png");
    bool hour = cart.onTheHour() && cart.hour() == 12 && cart.minute() == 0 && cart.second() < lenschime::kGraceSec;
    if (cart.won() && hour && std::strcmp(cart.reason(), "CHIME") == 0 && cart.plates() == lenschime::kPlates &&
        cart.cracked() == 0) {
        std::printf("S3 LENSCHIME  WIN  the hour chimes  %d:%02d:%02d  left  (%.1f s)\n", cart.hour(), cart.minute(),
                    cart.second(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 LENSCHIME  FAIL  %s  %d:%02d:%02d  plates %d  cracked %d  (%.1f s)\n",
                cart.reason()[0] ? cart.reason() : "OPEN", cart.hour(), cart.minute(), cart.second(), cart.plates(),
                cart.cracked(), frames / 60.0);
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
            std::printf("S3 LENSCHIME %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3lenschime [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<lenschime::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
