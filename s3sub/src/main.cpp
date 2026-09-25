// S3 SUB
//   s3sub                 dive the channel
//   s3sub --sim           autopilot reaches the lock without scraping
//   s3sub --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/sub.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };

    gs::System sys(true);
    sub::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool titled = false, diving = false;
    int frames = 0;
    const int limit = 60 * 90;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!titled && frames == 12) {
            save(sys, "title.png");
            titled = true;
        }
        if (!diving && frames == 180) {
            save(sys, "channel.png");
            diving = true;
        }
    }
    save(sys, "lock.png");
    if (cart.won()) {
        if (cart.scrapes() == 0) {
            std::printf("S3 SUB  CLEAR  hull %d  never scraped the bottom  (%.1f s)\n", cart.hull(), cart.seconds());
        } else {
            std::printf("S3 SUB  CLEAR  hull %d  scraped the bottom %d  (%.1f s)\n", cart.hull(), cart.scrapes(),
                        cart.seconds());
        }
        std::fflush(stdout);
        return 0;
    }
    const char* why = cart.note()[0] ? cart.note() : "still in the channel";
    std::printf("S3 SUB  BREACH  %s  hull %d  scrapes %d  x %.0f  floor %.1f  (%.1f s)\n", why, cart.hull(),
                cart.scrapes(), cart.x(), cart.lowest(), cart.seconds());
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
            std::printf("S3 SUB %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3sub [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<sub::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
