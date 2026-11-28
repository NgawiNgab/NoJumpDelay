#include "kaura/AuraConfig.h"

#include <cstdlib>

#include <nlohmann/json.hpp>

namespace kaura {

namespace {

std::vector<std::string> toList(const std::string &csv) {
    // modmenu Text fields carry comma-separated values (sigma's radio-style
    // comma-string typing); JSON config stores real arrays.
    std::vector<std::string> out;
    std::string cur;
    for (char c : csv) {
        if (c == ',') {
            if (!cur.empty()) out.push_back(cur);
            cur.clear();
        } else if (c != ' ') {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

std::string listToCsv(const std::vector<std::string> &v) {
    std::string out;
    for (std::size_t i = 0; i < v.size(); ++i) {
        if (i) out += ',';
        out += v[i];
    }
    return out;
}

} // namespace

bool applyConfigKey(AuraSettings &s, const std::string &key,
                    const std::string &value) {
    try {
        if (key == "killAura") s.killAura = value == "1" || value == "true";
        else if (key == "auraType") s.auraType = static_cast<AuraType>(std::atoi(value.c_str()));
        else if (key == "killRange") s.killRange = std::atof(value.c_str());
        else if (key == "minKillAuraCPS") s.minKillAuraCps = std::atoi(value.c_str());
        else if (key == "maxKillAuraCPS") s.maxKillAuraCps = std::atoi(value.c_str());
        else if (key == "rotations") s.rotations = value == "1" || value == "true";
        else if (key == "rotationsRange") s.rotationsRange = std::atof(value.c_str());
        else if (key == "swing") s.swing = value == "1" || value == "true";
        else if (key == "hurtCheck") s.hurtCheck = value == "1" || value == "true";
        else if (key == "emulateNativeAttack") s.emulateNativeAttack = value == "1" || value == "true";
        else if (key == "killItems") s.killItems = value == "1" || value == "true";
        else if (key == "antiBot") s.antiBot = value == "1" || value == "true";
        else if (key == "friendList") s.friendList = toList(value);
        else if (key == "botList") s.botList = toList(value);
        else if (key == "killAuraShortcut") s.killAuraShortcut = value == "1" || value == "true";
        else if (key == "killItemsShortcut") s.killItemsShortcut = value == "1" || value == "true";
        else return false;
    } catch (...) {
        return false;
    }
    return true;
}

std::string serializeConfig(const AuraSettings &s) {
    nlohmann::json j;
    j["killAura"] = s.killAura;
    j["auraType"] = static_cast<int>(s.auraType);
    j["killRange"] = s.killRange;
    j["minKillAuraCPS"] = s.minKillAuraCps;
    j["maxKillAuraCPS"] = s.maxKillAuraCps;
    j["rotations"] = s.rotations;
    j["rotationsRange"] = s.rotationsRange;
    j["swing"] = s.swing;
    j["hurtCheck"] = s.hurtCheck;
    j["emulateNativeAttack"] = s.emulateNativeAttack;
    j["killItems"] = s.killItems;
    j["antiBot"] = s.antiBot;
    j["friendList"] = s.friendList;
    j["botList"] = s.botList;
    j["killAuraShortcut"] = s.killAuraShortcut;
    j["killItemsShortcut"] = s.killItemsShortcut;
    return j.dump(2) + "\n";
}

bool deserializeConfig(const std::string &text, AuraSettings &s) {
    try {
        nlohmann::json j = nlohmann::json::parse(text);
        if (j.contains("killAura")) s.killAura = j["killAura"].get<bool>();
        if (j.contains("auraType"))
            s.auraType = static_cast<AuraType>(j["auraType"].get<int>());
        if (j.contains("killRange")) s.killRange = j["killRange"].get<float>();
        if (j.contains("minKillAuraCPS"))
            s.minKillAuraCps = j["minKillAuraCPS"].get<int>();
        if (j.contains("maxKillAuraCPS"))
            s.maxKillAuraCps = j["maxKillAuraCPS"].get<int>();
        if (j.contains("rotations")) s.rotations = j["rotations"].get<bool>();
        if (j.contains("rotationsRange"))
            s.rotationsRange = j["rotationsRange"].get<float>();
        if (j.contains("swing")) s.swing = j["swing"].get<bool>();
        if (j.contains("hurtCheck")) s.hurtCheck = j["hurtCheck"].get<bool>();
        if (j.contains("emulateNativeAttack"))
            s.emulateNativeAttack = j["emulateNativeAttack"].get<bool>();
        if (j.contains("killItems")) s.killItems = j["killItems"].get<bool>();
        if (j.contains("antiBot")) s.antiBot = j["antiBot"].get<bool>();
        if (j.contains("friendList"))
            s.friendList = j["friendList"].get<std::vector<std::string>>();
        if (j.contains("botList"))
            s.botList = j["botList"].get<std::vector<std::string>>();
        if (j.contains("killAuraShortcut"))
            s.killAuraShortcut = j["killAuraShortcut"].get<bool>();
        if (j.contains("killItemsShortcut"))
            s.killItemsShortcut = j["killItemsShortcut"].get<bool>();
    } catch (...) {
        return false;
    }
    return true;
}

} // namespace kaura
