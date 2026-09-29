// S3 SHELVE CHIME
//   s3shelvechime                 play until the hour chimes
//   s3shelvechime --sim           autopilot, exits 0 only when the hour chimes
//   s3shelvechime --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/shelvechime.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const char* name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    {
        gs::System sys(true);
        shelvechime::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 8; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    shelvechime::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    int frames = 0;
    const int limit = 60 * 25;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
    }
    save(sys, "end.png");
    bool hour = cart.onTheHour() && cart.hour() == 12 && cart.minute() == 0 && cart.second() < shelvechime::Game::kGraceSec;
    if (cart.won() && hour && cart.shelved() >= 1 && cart.returned() < 3 && std::strcmp(cart.reason(), "CHIME") == 0) {
        std::printf("S3 SHELVE CHIME  WIN  the hour chimes  %d:%02d:%02d  shelf %d  back %d  (%.1f s)\n", cart.hour(),
                    cart.minute(), cart.second(), cart.shelved(), cart.returned(), frames / 60.0);
        std::fflush(stdout);
        return 0;
    }
    std::printf("S3 SHELVE CHIME  FAIL  %s  %d:%02d:%02d  shelf %d  back %d  (%.1f s)\n",
                cart.reason()[0] ? cart.reason() : "OPEN", cart.hour(), cart.minute(), cart.second(), cart.shelved(),
                cart.returned(), frames / 60.0);
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
            std::printf("S3 SHELVE CHIME %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3shelvechime [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<shelvechime::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
