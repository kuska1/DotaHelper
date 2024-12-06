#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include "app_manager.h"
#include <windows.h>
#include "window.h"
#include "resource.h"
#include "win_hook.h"
#include <thread>
#include "server.h"
#include <nlohmann/json.hpp> // vckpg
#include <map>
#include <unordered_map>
using namespace std;

using json = nlohmann::json;

// Global data
HWND hwnd = NULL;
int FPSChange;
int horizontal, vertical = 0;
json json_data;

// Global var
const string settings_file = "settings.json";
wstring text_provider = L"...";
vector<string> arrows = { "↓", "→", "↑", "←" };
wstring conjure_arrow = L"↓";
bool conjure_cd = false;
std::unordered_map<std::string, int> invoker_skills_cooldown_list = {
    {"invoker_cold_snap", 0}, {"invoker_ghost_walk", 0}, {"invoker_ice_wall", 0},
    {"invoker_emp", 0}, {"invoker_tornado", 0}, {"invoker_alacrity", 0},
    {"invoker_sun_strike", 0}, {"invoker_forge_spirit", 0}, {"invoker_chaos_meteor", 0},
    {"invoker_deafening_blast", 0}
};
std::unordered_map<std::string, int> kez_skills_cooldown_list = {
    {"kez_echo_slash", 0}, {"kez_grappling_claw", 0}, {"kez_kazurai_katana", 0}, {"kez_raptor_dance", 0},
    {"kez_falcon_rush", 0}, {"kez_talon_toss", 0}, {"kez_shodo_sai", 0}, {"kez_ravens_veil", 0}
};
string hero_name;
bool hero_scepter;
json abilities;
json items;
std::unordered_map<int, float> aegis_coords = {
    {1, 3.39}, {2, 3.04}, {3, 2.77}, {4, 2.55}, {5, 2.36},
    {6, 1.78}, {7, 1.68}, {8, 1.596}, {9, 1.515}, {10, 1.444}
};
string event_type;
int event_game_time, event_player;
json previously;
int abilities_size;
int map_game_time, map_clock_time; // Diffent from each other
string map_name, map_game_state = "lobby"; // States: DOTA_GAMERULES_STATE_CUSTOM_GAME_SETUP, DOTA_GAMERULES_STATE_HERO_SELECTION, DOTA_GAMERULES_STATE_STRATEGY_TIME, DOTA_GAMERULES_STATE_WAIT_FOR_PLAYERS_TO_LOAD, DOTA_GAMERULES_STATE_GAME_IN_PROGRESS, DOTA_GAMERULES_STATE_PRE_GAME, DOTA_GAMERULES_STATE_POST_GAME
int gold, gold_reliable, gold_unreliable, gold_from_hero_kills, gold_from_creep_kills, gold_from_income, gold_from_shared, gpm, xpm;

// Global settings
std::map<std::string, int> getDefaultSettings() {
    return {
        {"FPSLobby", 1},
        {"FPSGame", 24},
        {"TokenMsgDisappear", 60},
        {"CooldownShowAlways", 0}
    };
}

json loadSettings(const std::string& filename) {
    std::ifstream inFile(filename);
    json j;

    if (inFile) {
        inFile >> j; // Read JSON
    }

    // Default settings
    auto defaultSettings = getDefaultSettings();

    // Add missing settings
    bool updated = false;
    for (const auto& pair : defaultSettings) {
        if (j.find(pair.first) == j.end()) {
            // Not found? Add!
            j[pair.first] = pair.second;
            updated = true;
        }
    }

    // Updated? Save!
    if (updated) {
        std::ofstream outFile(filename);
        if (outFile) {
            outFile << j.dump(4); // Saveds
        }
    }

    return j;
}

string formatTimestamp(int seconds) {
    int minutes = seconds / 60;
    int remaining_seconds = seconds % 60;

    // Format the output
    std::ostringstream formattedTimestamp;
    formattedTimestamp << minutes << ":" << std::setw(2) << std::setfill('0') << remaining_seconds;

    return formattedTimestamp.str();
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    APP app;
    BUILD build;
    json userSettings = loadSettings(settings_file);
    switch (uMsg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Clear
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hdc, &clientRect, hBrush);
        DeleteObject(hBrush);

        // Set up text properties
        SetTextColor(hdc, RGB(255, 255, 255)); // White text color
        SetBkMode(hdc, TRANSPARENT); // Transparent background for text

        // Build info
        LPCWSTR text_build = build.wide_build_full.c_str();
        RECT textRect_build = { 5, vertical - 20 }; // Bottom left
        DrawText(hdc, text_build, -1, &textRect_build, DT_SINGLELINE | DT_NOCLIP);

        #ifdef _DEBUG
            RECT textRect_provider = { 5, vertical - 35 }; // Bottom left
            DrawText(hdc, text_provider.c_str(), -1, &textRect_provider, DT_SINGLELINE | DT_NOCLIP);
            RECT textRect_game_state = { horizontal / 2 - map_game_state.length() * 4, vertical - 20}; // Bottom center
            DrawText(hdc, wstring(map_game_state.begin(), map_game_state.end()).c_str(), -1, &textRect_game_state, DT_SINGLELINE | DT_NOCLIP);
            RECT textRect_fps = { horizontal - 110, 40 }; // Top right
            DrawText(hdc, (L"FPS Timer: " + to_wstring(FPSChange)).c_str(), -1, &textRect_fps, DT_SINGLELINE | DT_NOCLIP);
        #endif

        if (map_game_state != "lobby") {
            if (map_game_state == "DOTA_GAMERULES_STATE_GAME_IN_PROGRESS" || map_game_state == "DOTA_GAMERULES_STATE_PRE_GAME") {
                int gxx = 150;
                int gxy = 13;
                if (map_name != "hero_demo_main") {
                    if (GetAsyncKeyState(VK_MENU) & 0x8000) { // VK_MENU corresponds to the Alt key
                        RECT textRect_gold = { gxx, gxy }; // Top left
                        DrawText(hdc, (L"Gold: " + to_wstring(gold)).c_str(), -1, &textRect_gold, DT_SINGLELINE | DT_NOCLIP);
                        RECT textRect_reliable = { gxx, gxy * 2 }; // Top left
                        DrawText(hdc, (L"Reliable: " + to_wstring(gold_reliable)).c_str(), -1, &textRect_reliable, DT_SINGLELINE | DT_NOCLIP);
                        RECT textRect_unreliable = { gxx, gxy * 3 }; // Top left
                        DrawText(hdc, (L"Unreliable: " + to_wstring(gold_unreliable)).c_str(), -1, &textRect_unreliable, DT_SINGLELINE | DT_NOCLIP);
                        RECT textRect_fhk = { gxx, gxy * 4 }; // Top left
                        DrawText(hdc, (L"From hero kills: " + to_wstring(gold_from_hero_kills)).c_str(), -1, &textRect_fhk, DT_SINGLELINE | DT_NOCLIP);
                        RECT textRect_fck = { gxx, gxy * 5 }; // Top left
                        DrawText(hdc, (L"From creep kills: " + to_wstring(gold_from_creep_kills)).c_str(), -1, &textRect_fck, DT_SINGLELINE | DT_NOCLIP);
                        RECT textRect_income = { gxx, gxy * 6 }; // Top left
                        DrawText(hdc, (L"From income: " + to_wstring(gold_from_creep_kills)).c_str(), -1, &textRect_income, DT_SINGLELINE | DT_NOCLIP);
                        RECT textRect_shared = { gxx, gxy * 7 }; // Top left
                        DrawText(hdc, (L"From shared: " + to_wstring(gold_from_creep_kills)).c_str(), -1, &textRect_shared, DT_SINGLELINE | DT_NOCLIP);
                    } else {
                        RECT textRect_gpm = { gxx, gxy }; // Top left
                        DrawText(hdc, (L"GPM: " + to_wstring(gpm)).c_str(), -1, &textRect_gpm, DT_SINGLELINE | DT_NOCLIP);
                        RECT textRect_xpm = { gxx, gxy * 2 }; // Top left
                        DrawText(hdc, (L"XPM: " + to_wstring(xpm)).c_str(), -1, &textRect_xpm, DT_SINGLELINE | DT_NOCLIP);
                    }
                } else {
                    if (GetAsyncKeyState(VK_MENU) & 0x8000) { // VK_MENU corresponds to the Alt key
                        RECT textRect_gold = { gxx + 50, gxy }; // Top left
                        DrawText(hdc, (L"Gold: " + to_wstring(gold)).c_str(), -1, &textRect_gold, DT_SINGLELINE | DT_NOCLIP);
                        RECT textRect_reliable = { gxx + 50, gxy * 2 }; // Top left
                        DrawText(hdc, (L"Reliable: " + to_wstring(gold_reliable)).c_str(), -1, &textRect_reliable, DT_SINGLELINE | DT_NOCLIP);
                        RECT textRect_unreliable = { gxx + 50, gxy * 3 }; // Top left
                        DrawText(hdc, (L"Unreliable: " + to_wstring(gold_unreliable)).c_str(), -1, &textRect_unreliable, DT_SINGLELINE | DT_NOCLIP);
                        RECT textRect_fhk = { gxx + 50, gxy * 4 }; // Top left
                        DrawText(hdc, (L"From hero kills: " + to_wstring(gold_from_hero_kills)).c_str(), -1, &textRect_fhk, DT_SINGLELINE | DT_NOCLIP);
                        RECT textRect_fck = { gxx + 50, gxy * 5 }; // Top left
                        DrawText(hdc, (L"From creep kills: " + to_wstring(gold_from_creep_kills)).c_str(), -1, &textRect_fck, DT_SINGLELINE | DT_NOCLIP);
                        RECT textRect_income = { gxx + 50, gxy * 6 }; // Top left
                        DrawText(hdc, (L"From income: " + to_wstring(gold_from_creep_kills)).c_str(), -1, &textRect_income, DT_SINGLELINE | DT_NOCLIP);
                        RECT textRect_shared = { gxx + 50, gxy * 7 }; // Top left
                        DrawText(hdc, (L"From shared: " + to_wstring(gold_from_creep_kills)).c_str(), -1, &textRect_shared, DT_SINGLELINE | DT_NOCLIP);
                    } else {
                        RECT textRect_gpm = { gxx + 50, gxy }; // Top left
                        DrawText(hdc, (L"GPM: " + to_wstring(gpm)).c_str(), -1, &textRect_gpm, DT_SINGLELINE | DT_NOCLIP);
                        RECT textRect_xpm = { gxx + 50, gxy * 2 }; // Top left
                        DrawText(hdc, (L"XPM: " + to_wstring(xpm)).c_str(), -1, &textRect_xpm, DT_SINGLELINE | DT_NOCLIP);
                    }
                }
                if (map_clock_time >= 420 && map_clock_time <= 420+userSettings["TokenMsgDisappear"] || map_clock_time >= 1020 && map_clock_time <= 1020+userSettings["TokenMsgDisappear"] || map_clock_time >= 1620 && map_clock_time <= 1620+userSettings["TokenMsgDisappear"] || map_clock_time >= 2220 && map_clock_time <= 2220+userSettings["TokenMsgDisappear"] || map_clock_time >= 3600 && map_clock_time <= 3600+userSettings["TokenMsgDisappear"]) {
                    RECT textRect_neutral = { horizontal - 620 + (25 * abilities_size), vertical - 95 }; // Near token slot
                    DrawText(hdc, L"NEW NEUTRAL TOKENS", -1, &textRect_neutral, DT_SINGLELINE | DT_NOCLIP);
                }
                if (items.contains("teleport0") && !items["teleport0"].empty() && items["teleport0"]["charges"] <= 0) {
                    RECT textRect_neutral = { horizontal - 615 + (25 * abilities_size), vertical - 40 }; // Near token slot
                    DrawText(hdc, L"NO TELEPORT", -1, &textRect_neutral, DT_SINGLELINE | DT_NOCLIP);
                }
                if (hero_name == "npc_dota_hero_terrorblade" && abilities["ability1"]["level"] != 0) {
                    if (map_clock_time >= -60 && map_clock_time <= 60) {
                        RECT textRect_fix_conjure = { horizontal / 2.5, vertical - 200 }; // Center upper abilities
                        DrawText(hdc, L"TO FIX ILLUSION PREDICT HOLD 'ALT + W'", -1, &textRect_fix_conjure, DT_SINGLELINE | DT_NOCLIP);
                    }
                    // '↓', '→', '↑', '←'
                    HFONT hFont = CreateFont(
                        22, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
                    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
                    RECT textRect_conjure = { horizontal - 915 + (-25 * abilities_size), vertical - 180 }; // Upper abilities
                    DrawText(hdc, conjure_arrow.c_str(), -1, &textRect_conjure, DT_SINGLELINE | DT_NOCLIP);
                    if (GetAsyncKeyState(0x57)) {
                        if (abilities["ability1"]["cooldown"] == 0) {
                            if (conjure_cd == false) {
                                conjure_cd = true;
                                if (conjure_arrow == L"↓") {
                                    conjure_arrow = L"→";
                                }
                                else if (conjure_arrow == L"→") {
                                    conjure_arrow = L"↑";
                                }
                                else if (conjure_arrow == L"↑") {
                                    conjure_arrow = L"←";
                                }
                                else if (conjure_arrow == L"←") {
                                    conjure_arrow = L"↓";
                                }
                            }
                        }
                    } else {
                        if (conjure_cd == true) {
                            conjure_cd = false;
                        }
                    }
                    if (GetAsyncKeyState(VK_MENU) & 0x8000 && GetAsyncKeyState(0x57)) {
                        if (conjure_arrow == L"↓") {
                            conjure_arrow = L"→";
                        }
                        else if (conjure_arrow == L"→") {
                            conjure_arrow = L"↑";
                        }
                        else if (conjure_arrow == L"↑") {
                            conjure_arrow = L"←";
                        }
                        else if (conjure_arrow == L"←") {
                            conjure_arrow = L"↓";
                        }
                    }
                } else if (hero_name == "npc_dota_hero_invoker") {
                    if (abilities["ability3"]["passive"] != true || abilities["ability4"]["passive"] != true) {
                        invoker_skills_cooldown_list[abilities["ability3"]["name"]] = map_clock_time + abilities["ability3"]["cooldown"];
                        invoker_skills_cooldown_list[abilities["ability4"]["name"]] = map_clock_time + abilities["ability4"]["cooldown"];
                    }
                    if (!GetAsyncKeyState(VK_MENU) && userSettings["CooldownShowAlways"] == 0) {
                        RECT textRect_cd_abi = { horizontal / 4, vertical - 200 }; // Upper portrait
                        DrawText(hdc, L"HOLD 'ALT' TO CHECK COOLDOWN", -1, &textRect_cd_abi, DT_SINGLELINE | DT_NOCLIP);
                    } else {
                        SetTextColor(hdc, RGB(151, 217, 255));
                        RECT textRect_q_cd = { horizontal / 4.6, vertical - 245 }; // Upper portrait
                        DrawText(hdc, (L"COLD SNAP:    " + to_wstring(max((invoker_skills_cooldown_list["invoker_cold_snap"] - map_clock_time), 0)) + L"\nGHOST WALK: " + to_wstring(max((invoker_skills_cooldown_list["invoker_ghost_walk"] - map_clock_time), 0)) + L"\nICE WALL:       " + to_wstring(max((invoker_skills_cooldown_list["invoker_ice_wall"] - map_clock_time), 0))).c_str(), -1, &textRect_q_cd, DT_NOCLIP);
                        SetTextColor(hdc, RGB(250, 173, 246));
                        RECT textRect_w_cd = { horizontal / 3.53, vertical - 245 }; // Upper portrait
                        DrawText(hdc, (L"E.M.P.:       " + to_wstring(max((invoker_skills_cooldown_list["invoker_emp"] - map_clock_time), 0)) + L"\nTORNADO: " + to_wstring(max((invoker_skills_cooldown_list["invoker_tornado"] - map_clock_time), 0)) + L"\nALACRITY: " + to_wstring(max((invoker_skills_cooldown_list["invoker_alacrity"] - map_clock_time), 0))).c_str(), -1, &textRect_w_cd, DT_NOCLIP);
                        SetTextColor(hdc, RGB(244, 204, 53));
                        RECT textRect_e_cd = { horizontal / 3, vertical - 245 }; // Upper portrait
                        DrawText(hdc, (L"SUNSTRIKE:        " + to_wstring(max((invoker_skills_cooldown_list["invoker_sun_strike"] - map_clock_time), 0)) + L"\nFORGE SPIRIT:    " + to_wstring(max((invoker_skills_cooldown_list["invoker_forge_spirit"] - map_clock_time), 0)) + L"\nCHAOS METEOR: " + to_wstring(max((invoker_skills_cooldown_list["invoker_chaos_meteor"] - map_clock_time), 0))).c_str(), -1, &textRect_e_cd, DT_NOCLIP);
                        SetTextColor(hdc, RGB(255, 255, 255));
                        RECT textRect_b_cd = { horizontal / 3.8, vertical - 190 }; // Upper portrait
                        DrawText(hdc, (L"DEAFENING BLAST: " + to_wstring(max((invoker_skills_cooldown_list["invoker_deafening_blast"] - map_clock_time), 0))).c_str(), -1, &textRect_b_cd, DT_NOCLIP);
                    }
                } else if (hero_name == "npc_dota_hero_kez" && hero_scepter == true) {
                    kez_skills_cooldown_list[abilities["ability0"]["name"]] = map_clock_time + abilities["ability0"]["cooldown"];
                    kez_skills_cooldown_list[abilities["ability1"]["name"]] = map_clock_time + abilities["ability1"]["cooldown"];
                    kez_skills_cooldown_list[abilities["ability2"]["name"]] = map_clock_time + abilities["ability2"]["cooldown"];
                    kez_skills_cooldown_list[abilities["ability4"]["name"]] = map_clock_time + abilities["ability4"]["cooldown"];
                    if (!GetAsyncKeyState(VK_MENU) && userSettings["CooldownShowAlways"] == 0) {
                        RECT textRect_cd_abi = { horizontal / 2.32, vertical - 180 }; // Upper abilities
                        DrawText(hdc, L"HOLD 'ALT' TO CHECK COOLDOWN", -1, &textRect_cd_abi, DT_SINGLELINE | DT_NOCLIP);
                    } else {
                        if (abilities["ability4"]["name"] == "kez_raptor_dance") {
                            SetTextColor(hdc, RGB(220, 220, 255));
                            //RECT textRect_k1_cd = { horizontal / 2.413, vertical - 160 }; // Upper abilities (Q) (OLD ONE)
                            RECT textRect_k1_cd = { (horizontal / 2) - 165, vertical - 160 }; // Upper abilities (Q)
                            DrawText(hdc, to_wstring(max((kez_skills_cooldown_list["kez_falcon_rush"] - map_clock_time), 0)).c_str(), -1, &textRect_k1_cd, DT_NOCLIP);
                            //RECT textRect_k2_cd = { horizontal / 2.25, vertical - 160 }; // Upper abilities (W) (OLD ONE)
                            RECT textRect_k2_cd = { (horizontal / 2) - 105, vertical - 160 }; // Upper abilities (W)
                            DrawText(hdc, to_wstring(max((kez_skills_cooldown_list["kez_talon_toss"] - map_clock_time), 0)).c_str(), -1, &textRect_k2_cd, DT_NOCLIP);
                            //RECT textRect_k3_cd = { horizontal / 2.109, vertical - 160 }; // Upper abilities (E) (OLD ONE)
                            RECT textRect_k3_cd = { (horizontal / 2) - 50, vertical - 160 }; // Upper abilities (E)
                            DrawText(hdc, to_wstring(max((kez_skills_cooldown_list["kez_shodo_sai"] - map_clock_time), 0)).c_str(), -1, &textRect_k3_cd, DT_NOCLIP);
                            //RECT textRect_k4_cd = { horizontal / 1.762, vertical - 160 }; // Upper abilities (R) (OLD ONE)
                            RECT textRect_k4_cd = { (horizontal / 2) + 125, vertical - 160 }; // Upper abilities (R)
                            DrawText(hdc, to_wstring(max((kez_skills_cooldown_list["kez_ravens_veil"] - map_clock_time), 0)).c_str(), -1, &textRect_k4_cd, DT_NOCLIP);
                        } else {
                            SetTextColor(hdc, RGB(204, 204, 0));
                            RECT textRect_k1_cd = { (horizontal / 2) - 165, vertical - 160 }; // Upper abilities (Q)
                            DrawText(hdc, to_wstring(max((kez_skills_cooldown_list["kez_echo_slash"] - map_clock_time), 0)).c_str(), -1, &textRect_k1_cd, DT_NOCLIP);
                            RECT textRect_k2_cd = { (horizontal / 2) - 105, vertical - 160 }; // Upper abilities (W)
                            DrawText(hdc, to_wstring(max((kez_skills_cooldown_list["kez_grappling_claw"] - map_clock_time), 0)).c_str(), -1, &textRect_k2_cd, DT_NOCLIP);
                            RECT textRect_k3_cd = { (horizontal / 2) - 50, vertical - 160 }; // Upper abilities (E)
                            DrawText(hdc, to_wstring(max((kez_skills_cooldown_list["kez_kazurai_katana"] - map_clock_time), 0)).c_str(), -1, &textRect_k3_cd, DT_NOCLIP);
                            RECT textRect_k4_cd = { (horizontal / 2) + 125, vertical - 160 }; // Upper abilities (R)
                            DrawText(hdc, to_wstring(max((kez_skills_cooldown_list["kez_raptor_dance"] - map_clock_time), 0)).c_str(), -1, &textRect_k4_cd, DT_NOCLIP);
                        }
                    }
                }
                // 1: 3.39, 3.04, 2.77, 2.55, 2.36
                // 2: 1.78, 1.68, 1.596, 1.515, 1.444
                //RECT textRect_aegis = { horizontal / 1.444, 50 }; // Bot players
                //DrawText(hdc, L"5:00", -1, &textRect_aegis, DT_NOCLIP);
                if (event_type == "aegis_picked_up") {
                    RECT textRect_aegis_info = { horizontal - 240, 40 }; // Bot FPS / PING
                    DrawText(hdc, L"USE 'ALT + F1' TO HIDE AEGIS INFO", -1, &textRect_aegis_info, DT_NOCLIP);
                    int aegis_life = max((event_game_time - map_game_time) + 300, 0);
                    string aegis_timestamp = formatTimestamp(aegis_life);
                    SetTextColor(hdc, RGB(255, 50, 50));
                    RECT textRect_aegis = { horizontal / aegis_coords[event_player + 1], 50 }; // Bot players
                    DrawText(hdc, wstring(aegis_timestamp.begin(), aegis_timestamp.end()).c_str(), -1, &textRect_aegis, DT_NOCLIP);
                    if (aegis_life <= 0 || (GetAsyncKeyState(VK_MENU) && GetAsyncKeyState(VK_F1))) {
                        event_type = "";
                    }
                }
            }
        }
        else {
            RECT textRect_settings = { horizontal - 680, 19}; // Top right
            HFONT hFont = CreateFont(
                22, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
            HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
            SetTextColor(hdc, RGB(95, 101, 109));
            DrawText(hdc, (app.wide_app_full).c_str(), -1, &textRect_settings, DT_SINGLELINE | DT_NOCLIP);
            SelectObject(hdc, hOldFont);
            DeleteObject(hFont);
        }

        EndPaint(hwnd, &ps);
        break;
    }
    case WM_CREATE:
        SetTimer(hwnd, 1, 1000 / 60, NULL); // 60 FPS refresh rate (adjust as needed)
        break;
    case WM_TIMER:
        // Trigger data update or periodic UI refresh
        InvalidateRect(hwnd, NULL, TRUE); // Request window redraw
        if (map_game_state == "lobby") {
            if (FPSChange != userSettings["FPSLobby"]) {
                FPSChange = userSettings["FPSLobby"];
                KillTimer(hwnd, 1);
                SetTimer(hwnd, 1, 1000 / FPSChange, NULL);
                #ifdef _DEBUG
                        cout << "[D] WM_TIMER NEW FPS: " << FPSChange << endl;
                #endif
            }
        }
        else {
            if (FPSChange != userSettings["FPSGame"]) {
                FPSChange = userSettings["FPSGame"];
                KillTimer(hwnd, 1);
                SetTimer(hwnd, 1, 1000 / FPSChange, NULL);
                #ifdef _DEBUG
                    cout << "[D] WM_TIMER NEW FPS: " << FPSChange << endl;
                #endif
            }
        }
        break;
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void GetDesktopResolution(int& horizontal, int& vertical)
{
    RECT desktop;
    const HWND hDesktop = GetDesktopWindow();
    GetWindowRect(hDesktop, &desktop);
    horizontal = desktop.right;
    vertical = desktop.bottom;
}

void OnWindowChange(const std::string& windowTitle) {
    #ifdef _DEBUG
        cout << "[D] Active window changed to: " << windowTitle << endl;
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    #endif
    if (windowTitle == "Dota 2") {
        #ifndef _DEBUG
            ShowWindow(hwnd, SW_SHOW);
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE); // IDK, without it, will be lower layer then dota...
        #endif
    }
    else {
        #ifndef _DEBUG
            ShowWindow(hwnd, SW_HIDE);
            SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        #endif
    }
}

void onServerDataReceived(const std::string& data) {
    #ifdef _DEBUG
        cout << "[D] Received: " << data << endl;
    #endif
    json json_data = json::parse(data);
    string str_some_data;
    if (json_data.contains("provider") && !json_data["provider"].empty()) {
        string str_some_data = to_string(json_data["provider"]["name"]) + " (v" + to_string(json_data["provider"]["version"]) + "): " + to_string(json_data["provider"]["timestamp"]);
        text_provider = wstring(str_some_data.begin(), str_some_data.end()).c_str();
    }
    if (json_data.contains("hero") && !json_data["hero"].empty() && !json_data["hero"].contains("hero")) {
        hero_name = json_data["hero"]["name"];
        hero_scepter = json_data["hero"]["aghanims_scepter"];
        #ifdef _DEBUG
                cout << "[D] Hero: Success!" << endl;
        #endif
    }
    if (json_data.contains("abilities") && !json_data["abilities"].empty() && !json_data["abilities"].contains("team2")) {
        abilities = json_data["abilities"];
        abilities_size = json_data["abilities"].size() - 3;
        #ifdef _DEBUG
                cout << "[D] Abilities: Success!" << endl;
        #endif
    }
    if (json_data.contains("items") && !json_data["items"].empty() && !json_data["items"].contains("team2")) {
        items = json_data["items"];
        #ifdef _DEBUG
            cout << "[D] Items: Success!" << endl;
        #endif
    }
    if (json_data.contains("map") && !json_data["map"].empty()) {
        map_name = json_data["map"]["name"];
        map_game_time = json_data["map"]["game_time"];
        map_clock_time = json_data["map"]["clock_time"];
        map_game_state = json_data["map"]["game_state"];
        #ifdef _DEBUG
                cout << "[D] Map: Success!" << endl;
        #endif
    }
    else {
        map_game_state = "lobby";
    }
    if (json_data.contains("player") && !json_data["player"].empty() && !json_data["player"].contains("team2")) {
        gold = json_data["player"]["gold"];
        gold_reliable = json_data["player"]["gold_reliable"];
        gold_unreliable = json_data["player"]["gold_unreliable"];
        gold_from_hero_kills = json_data["player"]["gold_from_hero_kills"];
        gold_from_creep_kills = json_data["player"]["gold_from_creep_kills"];
        gold_from_income = json_data["player"]["gold_from_income"];
        gold_from_shared = json_data["player"]["gold_from_shared"];
        gpm = json_data["player"]["gpm"];
        xpm = json_data["player"]["xpm"];
        #ifdef _DEBUG
                cout << "[D] Player: Success!" << endl;
        #endif
    }
    if (json_data.contains("events") && !json_data["events"].empty() && !json_data["events"].contains("team2")) {
        event_game_time = json_data["events"][0]["game_time"];
        event_type = json_data["events"][0]["event_type"];
        event_player = json_data["events"][0]["player_id"];
        #ifdef _DEBUG
                cout << "[D] Events: Success!" << endl;
        #endif
    }
    if (json_data.contains("previously") && !json_data["previously"].empty() && !json_data["previously"].contains("team2")) {
        previously = json_data["previously"];
        #ifdef _DEBUG
                cout << "[D] Previously: Success!" << endl;
        #endif
    }
    //InvalidateRect(hwnd, NULL, TRUE);
}

void startServerAsync() {
    std::thread serverThread([]() {
        startServer(onServerDataReceived);
        });

    serverThread.detach();
}


int RunWindow(HINSTANCE hInstance, int nCmdShow) {
    system("chcp 1251 > nul"); // Fix language issues
    startServerAsync();

    if (!StartWinEventHook(OnWindowChange)) {
        cout << "[!] Failed to start hook..." << endl;
        return 1;
    }

    APP app;
    auto app_info = app_manager();

    const string window_title = app.app_name + " Overlay";
    // Convert app_name to wide string
    wstring wAppName = wstring(window_title.begin(), window_title.end());
    const wchar_t* CLASS_NAME = wAppName.c_str();

    GetDesktopResolution(horizontal, vertical);

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1)); // Application icon
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH); // Transparent background
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW, // Transparent, click-through, and not in ALT+TAB
        CLASS_NAME,
        wAppName.c_str(), // Title
        WS_POPUP,
        0, 0,
        horizontal, vertical, // 1920x1080 most possible
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (hwnd == NULL) {
        return 0;
    }

    // Set the window to be always on top
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);


    // Set transparency
    COLORREF transparentColor = RGB(0, 0, 0); // Black as the transparent color
    SetLayeredWindowAttributes(hwnd, transparentColor, 0, LWA_COLORKEY);

    // Hide or show overlay
    #ifdef _DEBUG
        ShowWindow(hwnd, SW_SHOW);
    #else
        ShowWindow(hwnd, SW_HIDE);
    #endif

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    DestroyWindow(hwnd);
    StopWinEventHook();
    return 0;
}
