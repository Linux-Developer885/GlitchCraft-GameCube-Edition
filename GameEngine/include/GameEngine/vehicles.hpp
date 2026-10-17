#ifndef GLITCHCRAFT_GAME_ENGINE_VEHICLES_HPP
#define GLITCHCRAFT_GAME_ENGINE_VEHICLES_HPP

#include "glitchcraft/game_api.h"

#include <array>
#include <cstddef>
#include <string>

namespace glitchcraft::gameengine {

struct VehicleColor {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
};

inline bool isShinkansenEngine(GcVehicleType type)
{
    return type >= GC_VEHICLE_SHINKANSEN_ENGINE_RED &&
           type <= GC_VEHICLE_SHINKANSEN_REAR_ENGINE_BLACK;
}

inline bool isShinkansenCoach(GcVehicleType type)
{
    return type >= GC_VEHICLE_SHINKANSEN_COACH_RED &&
           type <= GC_VEHICLE_SHINKANSEN_COACH_BLACK;
}

inline bool isSubwayEngine(GcVehicleType type)
{
    return type >= GC_VEHICLE_SUBWAY_ENGINE_A &&
           type <= GC_VEHICLE_SUBWAY_ENGINE_Z;
}

inline bool isSubwayCoach(GcVehicleType type)
{
    return type >= GC_VEHICLE_SUBWAY_COACH_A &&
           type <= GC_VEHICLE_SUBWAY_COACH_Z;
}

inline bool isElectricLocomotive(GcVehicleType type)
{
    return isShinkansenEngine(type) || isSubwayEngine(type);
}

inline bool isSteamLocomotive(GcVehicleType type)
{
    return type == GC_VEHICLE_BIG_BOY ||
           type == GC_VEHICLE_FLYING_SCOTSMAN;
}

inline bool isSteamTender(GcVehicleType type)
{
    return type == GC_VEHICLE_BIG_BOY_TENDER ||
           type == GC_VEHICLE_FLYING_SCOTSMAN_TENDER;
}

inline GcVehicleType steamTenderFor(GcVehicleType locomotive)
{
    if (locomotive == GC_VEHICLE_BIG_BOY) {
        return GC_VEHICLE_BIG_BOY_TENDER;
    }
    if (locomotive == GC_VEHICLE_FLYING_SCOTSMAN) {
        return GC_VEHICLE_FLYING_SCOTSMAN_TENDER;
    }
    return GC_VEHICLE_COUNT;
}

inline bool isLocomotive(GcVehicleType type)
{
    return isSteamLocomotive(type) ||
           (type >= GC_VEHICLE_CSX_DIESEL &&
            type <= GC_VEHICLE_UNION_PACIFIC_DIESEL) ||
           isElectricLocomotive(type);
}

inline bool isPassengerVehicle(GcVehicleType type)
{
    return isShinkansenCoach(type) || isSubwayCoach(type) ||
           type == GC_VEHICLE_FLYING_SCOTSMAN_COACH;
}

inline VehicleColor vehicleColor(GcVehicleType type)
{
    constexpr VehicleColor shinkansenColors[]{
        {207, 52, 54}, {236, 190, 42}, {40, 91, 177},
        {46, 139, 87}, {101, 184, 220}, {28, 162, 169},
        {238, 238, 230}, {126, 131, 137}, {35, 37, 41}};
    constexpr VehicleColor subwayColors[]{
        {40, 91, 177}, {236, 120, 40}, {40, 91, 177},
        {236, 120, 40}, {40, 91, 177}, {236, 120, 40},
        {70, 152, 102}, {126, 131, 137}, {156, 123, 67},
        {156, 123, 67}, {70, 152, 102}, {126, 131, 137},
        {236, 120, 40}, {226, 190, 40}, {126, 131, 137},
        {156, 123, 67}, {226, 190, 40}, {226, 190, 40},
        {126, 131, 137}, {70, 152, 102}, {126, 131, 137},
        {126, 131, 137}, {226, 190, 40}, {126, 131, 137},
        {126, 131, 137}, {156, 123, 67}};
    if (type >= GC_VEHICLE_SHINKANSEN_ENGINE_RED &&
        type <= GC_VEHICLE_SHINKANSEN_ENGINE_BLACK) {
        return shinkansenColors[
            static_cast<std::size_t>(type - GC_VEHICLE_SHINKANSEN_ENGINE_RED)];
    }
    if (type >= GC_VEHICLE_SHINKANSEN_REAR_ENGINE_RED &&
        type <= GC_VEHICLE_SHINKANSEN_REAR_ENGINE_BLACK) {
        return shinkansenColors[static_cast<std::size_t>(
            type - GC_VEHICLE_SHINKANSEN_REAR_ENGINE_RED)];
    }
    if (isShinkansenCoach(type)) {
        return shinkansenColors[
            static_cast<std::size_t>(type - GC_VEHICLE_SHINKANSEN_COACH_RED)];
    }
    if (isSubwayEngine(type)) {
        return subwayColors[
            static_cast<std::size_t>(type - GC_VEHICLE_SUBWAY_ENGINE_A)];
    }
    if (isSubwayCoach(type)) {
        return subwayColors[
            static_cast<std::size_t>(type - GC_VEHICLE_SUBWAY_COACH_A)];
    }
    switch (type) {
    case GC_VEHICLE_BIG_BOY: return {25, 27, 30};
    case GC_VEHICLE_FLYING_SCOTSMAN: return {28, 83, 58};
    case GC_VEHICLE_FLYING_SCOTSMAN_TENDER: return {36, 73, 53};
    case GC_VEHICLE_FLYING_SCOTSMAN_COACH: return {130, 78, 49};
    case GC_VEHICLE_CSX_DIESEL: return {32, 85, 164};
    case GC_VEHICLE_NS_DIESEL: return {40, 43, 49};
    case GC_VEHICLE_BNSF_DIESEL: return {226, 94, 27};
    case GC_VEHICLE_UNION_PACIFIC_DIESEL: return {206, 171, 35};
    case GC_VEHICLE_BIG_BOY_TENDER: return {32, 34, 36};
    case GC_VEHICLE_BOXCAR: return {153, 66, 48};
    case GC_VEHICLE_HOPPER_CAR: return {73, 79, 75};
    case GC_VEHICLE_GONDOLA_CAR: return {95, 91, 81};
    case GC_VEHICLE_FLATCAR: return {106, 69, 45};
    case GC_VEHICLE_TANK_CAR: return {183, 180, 165};
    case GC_VEHICLE_CABOOSE: return {164, 47, 39};
    case GC_VEHICLE_AUTORACK_CAR: return {97, 114, 124};
    default: return {127, 127, 127};
    }
}

inline std::string vehicleName(GcVehicleType type)
{
    static constexpr const char *legacyNames[]{
        "4014 Big Boy", "CSX Diesel", "NS Diesel", "BNSF Diesel",
        "Union Pacific Diesel", "Big Boy Tender", "Boxcar", "Hopper Car",
        "Gondola Car", "Flatcar", "Tank Car", "Caboose", "Autorack Car"};
    if (type == GC_VEHICLE_FLYING_SCOTSMAN) {
        return "Flying Scotsman";
    }
    if (type == GC_VEHICLE_FLYING_SCOTSMAN_TENDER) {
        return "Flying Scotsman Tender";
    }
    if (type == GC_VEHICLE_FLYING_SCOTSMAN_COACH) {
        return "Flying Scotsman Coach";
    }
    static constexpr const char *colors[]{
        "Red", "Yellow", "Blue", "Green", "Light Blue",
        "Cyan", "White", "Gray", "Black"};
    if (type >= GC_VEHICLE_BIG_BOY && type <= GC_VEHICLE_AUTORACK_CAR) {
        return legacyNames[static_cast<std::size_t>(type)];
    }
    if (type >= GC_VEHICLE_SHINKANSEN_ENGINE_RED &&
        type <= GC_VEHICLE_SHINKANSEN_ENGINE_BLACK) {
        return std::string("Shinkansen ") +
               colors[static_cast<std::size_t>(
                   type - GC_VEHICLE_SHINKANSEN_ENGINE_RED)] + " Engine";
    }
    if (type >= GC_VEHICLE_SHINKANSEN_REAR_ENGINE_RED &&
        type <= GC_VEHICLE_SHINKANSEN_REAR_ENGINE_BLACK) {
        return std::string("Shinkansen ") +
               colors[static_cast<std::size_t>(
                   type - GC_VEHICLE_SHINKANSEN_REAR_ENGINE_RED)] +
               " Rear Engine";
    }
    if (isShinkansenCoach(type)) {
        return std::string("Shinkansen ") +
               colors[static_cast<std::size_t>(
                   type - GC_VEHICLE_SHINKANSEN_COACH_RED)] + " Coach";
    }
    if (isSubwayEngine(type)) {
        return std::string("NYC Subway ") +
               static_cast<char>('A' + type - GC_VEHICLE_SUBWAY_ENGINE_A) +
               " Engine";
    }
    if (isSubwayCoach(type)) {
        return std::string("NYC Subway ") +
               static_cast<char>('A' + type - GC_VEHICLE_SUBWAY_COACH_A) +
               " Coach";
    }
    return "Unknown Vehicle";
}

}

#endif
