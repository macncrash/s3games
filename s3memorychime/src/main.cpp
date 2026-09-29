// S3 MEMORYCHIME
//   s3memorychime                 play memory until the hour chimes, then leave
//   s3memorychime --sim           autopilot, exits 0 only when the hour chimes
//   s3memorychime --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/memorychime.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    if (shotDir) {
        gs::System sys(true);
        memchime::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    memchime::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool table = false;
    int frames = 0;
    const int limit = 60 * 40;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!table && frames == 48) {
            save(sys, "table.png");
            table = true;
        }
    }
    save(sys, "end.png");
    bool hour = cart.onTheHour() && cart.hour() == 12 && cart.minute() == 0 && cart.second() < memchime::kGraceSec;
    if (cart.won() && hour && cart.pairs() >= 1 && std::strcmp(cart.reason(), "CHIME") == 0) {
        std::printf("S3 MEMORYCHIME  WIN  the hour chimes  %d:%02d:%02d  pair %d  (%.1f s)\n", cart.hour(),
                    cart.minute(), cart.second(), cart.pairs(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 MEMORYCHIME  FAIL  %s  %d:%02d:%02d  pair %d  (%.1f s)\n", cart.reason()[0] ? cart.reason() : "OPEN",
                cart.hour(), cart.minute(), cart.second(), cart.pairs(), frames / 60.0);
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
            std::printf("S3 MEMORYCHIME %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3memorychime [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<memchime::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
