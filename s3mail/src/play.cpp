// S3 MAIL
//   s3mail                 ride the route
//   s3mail --sim           the autopilot delivers every box and makes every turn
//   s3mail --sim --shots D also writes PNGs into D
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "console/system.h"
#include "game/mail.h"
#include "version.h"

static int simulate(const char* shotDir) {
    auto save = [&](gs::System& sys, const std::string& name) {
        if (!shotDir) return;
        sys.render();
        sys.saveScreenshot(std::string(shotDir) + "/" + name);
    };
    if (shotDir) {
        gs::System sys(true);
        mail::Game cart;
        sys.bootCart(cart);
        for (int i = 0; i < 50; i++) sys.step();
        save(sys, "title.png");
    }

    gs::System sys(true);
    mail::Game cart;
    cart.setBot(true);
    sys.bootCart(cart);
    bool mid = false, bend = false, done = false;
    int frames = 0;
    const int limit = 60 * 45;
    while (!cart.over() && frames < limit) {
        sys.step();
        frames++;
        if (!mid && cart.boxesGot() == 1) {
            save(sys, "box.png");
            mid = true;
        }
        if (!bend && cart.turnsMade() == 1) {
            save(sys, "turn.png");
            bend = true;
        }
        if (cart.over() && !done) {
            save(sys, "end.png");
            done = true;
        }
    }
    if (shotDir && !done) save(sys, "end.png");
    if (!cart.won()) {
        const char* why = cart.fail()[0] ? cart.fail() : "UNFINISHED";
        std::printf("S3 MAIL  FAIL  %s  boxes %d/%d  turns %d/%d  (%.1f s)\n", why, cart.boxesGot(), cart.boxCount(),
                    cart.turnsMade(), cart.turnCount(), cart.seconds());
        std::fflush(stdout);
        return 1;
    }
    std::printf("S3 MAIL  THE BOX GOT THE PAPER  boxes %d/%d  turns %d/%d  (%.1f s)\n", cart.boxesGot(),
                cart.boxCount(), cart.turnsMade(), cart.turnCount(), cart.seconds());
    std::fflush(stdout);
    return 0;
}

int main(int argc, char** argv) {
    bool sim = false;
    const char* shots = nullptr;
    for (int i = 1; i < argc; i++) {
        if (!std::strcmp(argv[i], "--sim")) sim = true;
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--version")) {
            std::printf("S3 MAIL %s\n", S3_VERSION_STRING);
            return 0;
        } else {
            std::fprintf(stderr, "usage: s3mail [--sim] [--shots DIR] [--version]\n");
            return 2;
        }
    }
    if (sim) return simulate(shots);
    auto cart = std::make_unique<mail::Game>();
    auto sys = std::make_unique<gs::System>();
    return sys->run(*cart);
}
