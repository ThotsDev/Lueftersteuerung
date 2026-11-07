#pragma once
#include <Matter.h>
#include <cstdint>
#include <optional>

// Zentrale Matter-Geraeteangaben. Aenderungen werden nach Build und Upload aktiv.
// Leere optionale Texte / nullopt behalten die Vorgabe der Matter-Bibliothek bei.
namespace device_identity {
inline constexpr char vendor_name[] = "Kevin Wahl";
inline constexpr char product_name[] = "Lueftungssteuerung";
inline constexpr char device_name[] = "Lueftungssteuerung Simulation";
inline constexpr char serial_number[] = "";
inline constexpr std::optional<std::uint16_t> hardware_version = std::nullopt; // z.B. 1
inline constexpr char hardware_version_string[] = ""; // z.B. "Prototype 1"

// Informationswerte aus dem vorgebauten SDK, keine frei aenderbaren Kennungen.
// Eine andere Vendor-/Product-ID erfordert passende SDK-/Attestierungsdaten.
inline constexpr std::uint16_t sdk_vendor_id = CONFIG_DEVICE_VENDOR_ID;
inline constexpr std::uint16_t sdk_product_id = CONFIG_DEVICE_PRODUCT_ID;
static_assert(sdk_vendor_id == CONFIG_DEVICE_VENDOR_ID, "Vendor-ID is fixed by the precompiled SDK");
static_assert(sdk_product_id == CONFIG_DEVICE_PRODUCT_ID, "Product-ID is fixed by the precompiled SDK");

// Nach dem Erzeugen der Endpunkte und vor Matter.begin() aufrufen.
inline bool apply() {
    if (vendor_name[0] && !Matter.setVendorName(vendor_name)) return false;
    if (!Matter.setProductName(product_name) || !Matter.setDeviceName(device_name)) return false;
    if (serial_number[0] && !Matter.setSerialNumber(serial_number)) return false;
    if (hardware_version && !Matter.setHardwareVersion(*hardware_version)) return false;
    if (hardware_version_string[0] && !Matter.setHardwareVersionString(hardware_version_string)) return false;
    return true;
}
} // namespace device_identity
