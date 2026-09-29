#pragma once

// Side-view yard. Screen pixels. The pier, the hull and the painted mark
// share these numbers with the picture drawn at boot.
namespace cranemark {

constexpr float RAIL_Y = 34.f;
constexpr float TX_MIN = 36.f;
constexpr float TX_MAX = 292.f;
constexpr float LEN_MIN = 18.f;
constexpr float LEN_MAX = 130.f;
constexpr float CRATE_W = 26.f;
constexpr float CRATE_H = 20.f;
constexpr float HOOK_GAP = 5.f;
constexpr float PIER_L = 8.f;
constexpr float PIER_R = 116.f;
constexpr float PIER_Y = 174.f;
constexpr float SHIP_L = 170.f;
constexpr float SHIP_R = 314.f;
constexpr float DECK_Y = 152.f;
constexpr float WATER_Y = 198.f;
constexpr float MARK_X = 228.f;
constexpr float MARK_HALF = 20.f;
constexpr float SET_X = 9.f;
constexpr float HOME_X = 58.f;
constexpr float GRAV = 620.f;
constexpr float TRAVEL_L = 32.f;
constexpr float LEG_END = 52.f;
constexpr float HOLD_NEED = 0.65f;

constexpr float placeLen() { return (DECK_Y - HOOK_GAP - CRATE_H) - RAIL_Y; }

static_assert(placeLen() < LEN_MAX, "the cable reaches the deck");
static_assert(placeLen() > TRAVEL_L + 20.f, "travel clears the deck");
static_assert(MARK_X - MARK_HALF > SHIP_L + 8.f, "the mark is on the hull");
static_assert(MARK_X + MARK_HALF < SHIP_R - 28.f, "the mark stays clear of the house");
static_assert(HOME_X > PIER_L && HOME_X < PIER_R, "the load starts over the pier");

}  // namespace cranemark
