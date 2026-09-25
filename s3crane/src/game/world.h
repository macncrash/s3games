#pragma once

// Side-view dock. Numbers are screen pixels. The pier, the ship and the
// painted mark share these constants with the boot picture.
namespace crane {

constexpr float RAIL_Y = 36.f;
constexpr float TX_MIN = 28.f;
constexpr float TX_MAX = 300.f;
constexpr float LEN_MIN = 22.f;
constexpr float LEN_MAX = 116.f;
constexpr float CRATE_W = 28.f;
constexpr float CRATE_H = 22.f;
constexpr float HOOK_GAP = 6.f;
constexpr float DOCK_L = 6.f;
constexpr float DOCK_R = 132.f;
constexpr float DOCK_Y = 168.f;
constexpr float SHIP_L = 176.f;
constexpr float SHIP_R = 316.f;
constexpr float DECK_Y = 148.f;
constexpr float WATER_Y = 188.f;
constexpr float MARK_X = 232.f;
constexpr float MARK_HALF = 32.f;
constexpr float SET_X = 13.f;
constexpr float HOME_X[3] = {38.f, 72.f, 106.f};
constexpr float GRAV = 680.f;
constexpr float TRAVEL_L = 30.f;

constexpr float grabLen() { return (DOCK_Y - CRATE_H - HOOK_GAP) - RAIL_Y; }
constexpr float placeLen(int stack) { return (DECK_Y - float(stack + 1) * CRATE_H - HOOK_GAP) - RAIL_Y; }

static_assert(HOME_X[2] + CRATE_W * 0.5f < DOCK_R, "third crate sits on the pier");
static_assert(MARK_X + MARK_HALF < SHIP_R - 36.f, "mark stays clear of the cabin");
static_assert(MARK_X - MARK_HALF > SHIP_L, "mark is on the ship");
static_assert(grabLen() < LEN_MAX, "hook reaches the pier");
static_assert(placeLen(2) > TRAVEL_L, "the third crate is lowered onto the stack");
static_assert(RAIL_Y + TRAVEL_L + HOOK_GAP + CRATE_H < DECK_Y - 2.f * CRATE_H, "travel clears two crates");

}  // namespace crane
