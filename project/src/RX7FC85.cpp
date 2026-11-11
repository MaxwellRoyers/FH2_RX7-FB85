#include "fh2mod.h"
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include <exception>
#include <stdexcept>

namespace fs = std::filesystem;

static std::vector<unsigned char> read_binary(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open " + p.string());
    return std::vector<unsigned char>(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}
static std::string read_text(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open " + p.string());
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

FH2MOD_EXPORT int fh2mod_init(const FH2ModApi* api) {
    if (!api || api->api_version < 2 || !api->set_file || !api->add_sql || !api->register_mod) return 1;
    FH2ModInfo info{};
    info.size = sizeof(info);
    info.name = "1985 Mazda RX-7 FB";
    info.version = "1.1.0";
    info.github_repo = nullptr;
    info.platform_version = FH2MOD_PLATFORM_VERSION;
    info.check_updates = 0;
    info.description = "Registers one prebuilt FH2 car ZIP and applies its database patch.";
    FH2Mod* mod = api->register_mod(&info);
    if (!mod) return 1;

    // Reuse the built-in NULL CAR string slots for this custom car.
    // The SQL row below points DisplayName/ModelShort at these keys.
    api->set_string(mod, "Data_Car.str", 0x5C19, L"RX-7 FB");
    api->set_string(mod, "Data_Car.str", 0x5BDD, L"RX-7 FB");

    try {
        if (!api->azimuth_dir || !*api->azimuth_dir) throw std::runtime_error("Azimuth folder is unavailable");
        const fs::path data = fs::path(api->azimuth_dir) / L"RX7FC85";
        const auto car = read_binary(data / L"car.zip");
        const auto manifest = read_binary(data / L"zipmanifest.xml");
        const auto sql = read_text(data / L"car.sql");
        const auto thumbBig =
            read_binary(data / L"Thumbnail_1480_Big.swatchbin");

        const auto thumbSmall =
            read_binary(data / L"Thumbnail_1480_Small.swatchbin");
        if (car.empty() || manifest.empty() || sql.empty() ||
            thumbBig.empty() || thumbSmall.empty())
            throw std::runtime_error("One or more mod files are empty");

        const int manifest_ok = api->set_file(mod, "zipmanifest.xml", manifest.data(), manifest.size());
        const int car_ok = api->set_file(mod, "cars/MAZ_RX7_85.zip", car.data(), car.size());
        if (!manifest_ok || !car_ok) {
            api->clear_file(mod, "zipmanifest.xml");
            api->clear_file(mod, "cars/MAZ_RX7_85.zip");
            api->log(mod, "ERROR: Could not claim zipmanifest.xml or the car ZIP. Another mod may already replace one of these files.");
            return 1;
        }
        const int thumbBigOk = api->set_file(
            mod,
            "Stripped/RC0/Thumbnail_1480_Big.swatchbin",
            thumbBig.data(),
            thumbBig.size()
        );

        const int thumbSmallOk = api->set_file(
            mod,
            "Stripped/RC0/Thumbnail_1480_Small.swatchbin",
            thumbSmall.data(),
            thumbSmall.size()
        );

        if (!thumbBigOk || !thumbSmallOk) {
            api->clear_file(mod, "Stripped/RC0/Thumbnail_1480_Big.swatchbin");
            api->clear_file(mod, "Stripped/RC0/Thumbnail_1480_Small.swatchbin");

            api->log(mod, "ERROR: Could not register RX7 thumbnail files.");
            return 1;
        }
        api->add_sql(mod, sql.c_str());
        api->log(mod, "Car ZIP and zipmanifest registered; database SQL patch queued. Check platform.log for database errors.");
        return 0;
    } catch (const std::exception& e) {
        api->log(mod, (std::string("ERROR: ") + e.what()).c_str());
        return 1;
    } catch (...) {
        api->log(mod, "ERROR: unknown exception while loading car files");
        return 1;
    }
}
