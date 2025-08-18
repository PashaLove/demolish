// Dear ImGui: standalone example application for DirectX 9

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp
// - OpenSSL https://slproweb.com/download/Win32OpenSSL-3_2_1.exe

#define IMGUI_DEFINE_MATH_OPERATORS
#include "stdafx.h"
#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include "BuildLauncherConfig.h"
#include "resource.h"
#include <d3d9.h>
#include <tchar.h>
#include <json/json.h>
#include "CPatcher.h"
#include "imgui_freetype.h"
#include "imgui_edited.hpp"
#include "image.h"
#include "font.h"
#include "imgui_notify.h"
#include "imfilebrowser.h"
#include "imspinner.h"
#include "set"
#include "NewsMarkdown.h"
#include <future>
#include <Shellapi.h>

//#include <d3d11.h>
#include <tchar.h>
#include <dwmapi.h>

#include <d3dx9.h>
#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")
#pragma comment (lib, "freetype.lib")

#ifdef DEBUG
#pragma comment(lib, "jsoncpp_static_debug.lib")    // libcurl debug version
#else
#pragma comment (lib, "jsoncpp_static.lib")
#endif

#pragma warning(disable:4081 4100 4244 4189 4018 4390 4996 4129 4305 4101 28251)

#include "MainThread.h"
//Sirin Mode
#include "ConfigMgr.h"
#include "SirinAccountInfo.h"
#pragma comment (lib, "sirin-launcher.lib")


// Data
static LPDIRECT3D9              g_pD3D = nullptr;
static LPDIRECT3DDEVICE9        g_pd3dDevice = nullptr;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Helpers macros
// We normally try to not use many helpers in imgui_demo.cpp in order to make code easier to copy and paste,
// but making an exception here as those are largely simplifying code...
// In other imgui sources we can use nicer internal functions from imgui_internal.h (ImMin/ImMax) but not in the demo.
#define IM_MIN(A, B)            (((A) < (B)) ? (A) : (B))
#define IM_MAX(A, B)            (((A) >= (B)) ? (A) : (B))
#define IM_CLAMP(V, MN, MX)     ((V) < (MN) ? (MN) : (V) > (MX) ? (MX) : (V))

namespace texture
{
    IDirect3DTexture9* background = nullptr;
    IDirect3DTexture9* logo_bcc = nullptr;
    IDirect3DTexture9* logo_ccc = nullptr;
    IDirect3DTexture9* logo_acc = nullptr;
    IDirect3DTexture9* wordmark = nullptr;
    IDirect3DTexture9* logo = nullptr;
    IDirect3DTexture9* rfs = nullptr;
    IDirect3DTexture9* banner = nullptr;    
    IDirect3DTexture9* Wordmark_Black = nullptr;
    IDirect3DTexture9* logo_voidA = nullptr;
    IDirect3DTexture9* logo_voidB = nullptr;
    IDirect3DTexture9* logo_voidC = nullptr;
}

namespace font
{
    inline ImFont* lexend_regular = nullptr;
    inline ImFont* lexend_medium = nullptr;
    inline ImFont* lexend_medium_x = nullptr;
    inline ImFont* lexend_bold = nullptr;
    inline ImFont* font_awesome[3];
    inline ImFont* icomoon;
}

HWND hwnd;
RECT rc;

int tabs = 0;
bool SirinMode = false;
bool LoginSuccess = false; // Default Protocol use, not Sirin
bool critError = false;
int active_tab = 0;
bool allowRefresh = true;
bool showWaitLogin = false;
bool showServerList = false;
static float velocity = 1.f;

static float nextdot = 0, nextdot2;
static bool show_number = false;
float timer = 0;
bool showLoginPanel = false;
char errMsg[255] = { 0 };
using namespace ImGui;

R3Engine R3EngineConfig;
static int selectedGraph = 0;
static int selectedRes = 0; // Resolution
static int blacklistcuridx = 0; // Here we store our selection data as an index.
static int selectedServer = 0;
static int selectedGate = 0;
static int selectedLanguage = 6; //default PH - AOP ?

enum elementShadow { Element_Fire, Element_Earth, Element_Air, Element_Water, Element_COUNT };
const char* s_pszR3Path = ".\\R3Engine.ini";
const char* texture_cfg[4] = { "Low", "Default", "High", "Very High" };
const char* dynamiclight_cfg[4] = { "None", "1", "4", "Unlimited" };
const char* gloweff_cfg[3] = { "None", "Low", "High" };
const char* shadow_cfg[4] = { "Default", "+1", "+10", "Unlimited" };
const char* multiAA_cfg[5] = { "0", "2", "4", "8", "16"};
const char* gamma_cfg[5] = { "0.8", "1.0", "1.2", "1.4", "1.8" };
const char* language_cfg[11][2] = { {"KR", "Korea" }, {"BR","Brasil"}, {"CN", "China"}, {"EU","English-EU"}, {"ID","Indonesia"}, {"JP","Japan"}, {"PH","English-PH"}, {"RU","Russian"}, {"TW","Taiwan"}, {"US","English-US"}, {"TH","Thailand"} };
const ImU32   u32_zero = 0, u32_one = 1, u32_double = 2, u32_fifty = 50, u32_min = 0, u32_max = UINT_MAX / 2, u32_hi_a = UINT_MAX / 2 - 100, u32_hi_b = UINT_MAX / 2, u32_kilobytes = 1024;
const ImU64   u64_zero = 0, u64_one = 1, u64_fifty = 50, u64_min = 0, u64_max = ULLONG_MAX / 2, u64_hi_a = ULLONG_MAX / 2 - 100, u64_hi_b = ULLONG_MAX / 2, u64_kilobytes = 1024;
bool GMMode = false;
bool calledCheckUpdate = false;
bool calledNews = false;
bool calledRenderImgNews = false;
bool renderAllNews = false;
int tabNewsSize = TABSNEWSSIZE;

std::vector<std::string> resolutions;
std::vector<std::string> graphicCards;
std::string enckey = ENCKEY;


void LoadR3EngineConfig() {
    char ReturnedString[128] = { 0 }; // [esp+2Ch] [ebp-104h]
    char defaultGraphics[128];
    sprintf_s(defaultGraphics, "%s", graphicCards.size() > 0 ? graphicCards[0].c_str() : "None");
    GetPrivateProfileStringA("RenderState", "Adapter", defaultGraphics, R3EngineConfig.RenderState.Adapter, sizeof(R3EngineConfig.RenderState.Adapter), s_pszR3Path);
    GetPrivateProfileStringA("RenderState", "ScreenXSize", "640", ReturnedString, sizeof(ReturnedString), s_pszR3Path);
    R3EngineConfig.RenderState.ScreenXSize = atol(ReturnedString);
    GetPrivateProfileStringA("RenderState", "ScreenYSize", "480", ReturnedString, sizeof(ReturnedString), s_pszR3Path);
    R3EngineConfig.RenderState.ScreenYSize = atol(ReturnedString);
    GetPrivateProfileStringA("RenderState", "RenderBits", "32", ReturnedString, 0x80u, s_pszR3Path);
    R3EngineConfig.RenderState.RenderBits = atol(ReturnedString);
    GetPrivateProfileStringA("RenderState", "TextureDetail", "3", ReturnedString, 0x80u, s_pszR3Path);
    R3EngineConfig.RenderState.TextureDetail = atol(ReturnedString);
    GetPrivateProfileStringA("RenderState", "DynamicLight", "1", ReturnedString, 0x80u, s_pszR3Path);
    R3EngineConfig.RenderState.DynamicLight = atol(ReturnedString);
    GetPrivateProfileStringA("RenderState", "ShadowDetail", "0", ReturnedString, 0x80u, s_pszR3Path);
    R3EngineConfig.RenderState.ShadowDetail = atol(ReturnedString);
    GetPrivateProfileStringA("RenderState", "BBoShasi", "2", ReturnedString, 0x80u, s_pszR3Path);
    R3EngineConfig.RenderState.BBoShasi = atol(ReturnedString);
    GetPrivateProfileStringA("RenderState", "Gamma", "1.0f", ReturnedString, 0x80u, s_pszR3Path);
    R3EngineConfig.RenderState.Gamma = atof(ReturnedString);
    GetPrivateProfileStringA("RenderState", "AntiAliasing", "0", ReturnedString, 0x80u, s_pszR3Path);
    R3EngineConfig.RenderState.AntiAliasing = atol(ReturnedString);
    if (R3EngineConfig.RenderState.AntiAliasing > 8)
        R3EngineConfig.RenderState.AntiAliasing = 8;
    if (R3EngineConfig.RenderState.TextureDetail > 3)
        R3EngineConfig.RenderState.TextureDetail = 1;
    if (R3EngineConfig.RenderState.DynamicLight > 3)
        R3EngineConfig.RenderState.DynamicLight = 1;
    if (R3EngineConfig.RenderState.ShadowDetail > 0)
        R3EngineConfig.RenderState.ShadowDetail = 0;
    if (R3EngineConfig.RenderState.BBoShasi > 2)
        R3EngineConfig.RenderState.BBoShasi = 0;

    GetPrivateProfileStringA("RenderState", "bFullScreen", "FALSE", ReturnedString, 0x80u, s_pszR3Path);
    _strupr(ReturnedString);
    R3EngineConfig.RenderState.bFullScreen = memcmp(&ReturnedString, "TRUE", 5u) == 0;

    GetPrivateProfileStringA("RenderState", "bDetailTexture", "TRUE", ReturnedString, 0x80u, s_pszR3Path);
    _strupr(ReturnedString);
    R3EngineConfig.RenderState.bDetailTexture = memcmp(&ReturnedString, "TRUE", 5u) == 0;

    GetPrivateProfileStringA("RenderState", "bMouseAccelation", "TRUE", ReturnedString, 0x80u, s_pszR3Path);
    _strupr(ReturnedString);
    R3EngineConfig.RenderState.bMouseAccelation = memcmp(&ReturnedString, "TRUE", 5u) == 0;

    GetPrivateProfileStringA("Sound", "Sound", "TRUE", ReturnedString, 0x80u, s_pszR3Path);
    _strupr(ReturnedString);
    R3EngineConfig.Sound.Sound = memcmp(&ReturnedString, "TRUE", 5u) == 0;

    GetPrivateProfileStringA("Sound", "Music", "TRUE", ReturnedString, 0x80u, s_pszR3Path);
    _strupr(ReturnedString);
    R3EngineConfig.Sound.Music = memcmp(&ReturnedString, "TRUE", 5u) == 0;

    GetPrivateProfileStringA("Sound", "SoundVol", "1.0f", ReturnedString, 0x80u, s_pszR3Path);
    R3EngineConfig.Sound.SoundVol = atof(ReturnedString);

    GetPrivateProfileStringA("Sound", "MusicVol", "1.0f", ReturnedString, 0x80u, s_pszR3Path);
    R3EngineConfig.Sound.MusicVol = atof(ReturnedString);

    GetPrivateProfileStringA("Sound", "AmbVol", "1.0f", ReturnedString, 0x80u, s_pszR3Path);
    R3EngineConfig.Sound.AmbVol = atof(ReturnedString);

    GetPrivateProfileStringA("Launcher", "Throttle", "FALSE", ReturnedString, 0x80u, s_pszR3Path);
    _strupr(ReturnedString);
    R3EngineConfig.Launcher.enableThrottle = memcmp(&ReturnedString, "TRUE", 5u) == 0;

    GetPrivateProfileStringA("Launcher", "SpeedLimit", "1000", ReturnedString, 0x80u, s_pszR3Path);
    R3EngineConfig.Launcher.throtleSpeed = atoll(ReturnedString);

    GetPrivateProfileStringA("Launcher", "ServerGame", "0", ReturnedString, 0x80u, s_pszR3Path);
    R3EngineConfig.Launcher.SelectedServer = atoll(ReturnedString);
    selectedServer = R3EngineConfig.Launcher.SelectedServer;

    GetPrivateProfileStringA("Launcher", "ServerGate", "0", ReturnedString, 0x80u, s_pszR3Path);
    R3EngineConfig.Launcher.SelectedGate = atoll(ReturnedString);
    selectedGate = R3EngineConfig.Launcher.SelectedGate;

    GetPrivateProfileStringA("Game", "Language", "6", ReturnedString, 0x80u, s_pszR3Path);
    R3EngineConfig.Launcher.Language = atol(ReturnedString);
    selectedLanguage = R3EngineConfig.Launcher.Language;
    g_ConfigMgr.SetLanguage(language_cfg[selectedLanguage][0]);
    g_Main.SetLanguage(language_cfg[selectedLanguage][0]);

}
bool SaveDownloadConfig() {
    char szBuffer[25];
    WritePrivateProfileStringA("Launcher", "Throttle", c_Patcher.enableThrottle ? "TRUE" : "FALSE", s_pszR3Path);
    if (c_Patcher.throtleSpeedLimit <= 0) {
        PushBackNotify(3, "Download Speed Can't be lowered than 1");
        return false;
    }

    sprintf(szBuffer, "%lld", c_Patcher.throtleSpeedLimit);
    WritePrivateProfileStringA("Launcher", "SpeedLimit", szBuffer, s_pszR3Path);   
    return true;
}

bool SaveServerConfig() {
    char szBuffer[25];
    sprintf(szBuffer, "%d", selectedServer);
    WritePrivateProfileStringA("Launcher", "ServerGame", szBuffer, s_pszR3Path);
    sprintf(szBuffer, "%d", selectedGate);
    WritePrivateProfileStringA("Launcher", "ServerGate", szBuffer, s_pszR3Path);
    return true;
}

bool SaveR3Config() {
    char szBuffer[25];
    WritePrivateProfileStringA("RenderState", "Adapter", R3EngineConfig.RenderState.Adapter, s_pszR3Path);
    WritePrivateProfileStringA("RenderState", "bFullScreen", R3EngineConfig.RenderState.bFullScreen ? "TRUE" : "FALSE", s_pszR3Path);
    WritePrivateProfileStringA("RenderState", "bDetailTexture", R3EngineConfig.RenderState.bDetailTexture ? "TRUE" : "FALSE", s_pszR3Path);
    sprintf(szBuffer, "%d", R3EngineConfig.RenderState.ScreenXSize);
    WritePrivateProfileStringA("RenderState", "ScreenXSize", szBuffer, s_pszR3Path);
    sprintf(szBuffer, "%d", R3EngineConfig.RenderState.ScreenYSize);
    WritePrivateProfileStringA("RenderState", "ScreenYSize", szBuffer, s_pszR3Path);
    sprintf(szBuffer, "%d", R3EngineConfig.RenderState.TextureDetail);
    WritePrivateProfileStringA("RenderState", "TextureDetail", szBuffer, s_pszR3Path);
    sprintf(szBuffer, "%d", R3EngineConfig.RenderState.DynamicLight);
    WritePrivateProfileStringA("RenderState", "DynamicLight", szBuffer, s_pszR3Path);
    sprintf(szBuffer, "%d", R3EngineConfig.RenderState.ShadowDetail);
    WritePrivateProfileStringA("RenderState", "ShadowDetail", szBuffer, s_pszR3Path);
    sprintf(szBuffer, "%d", R3EngineConfig.RenderState.BBoShasi);
    WritePrivateProfileStringA("RenderState", "BBoShasi", szBuffer, s_pszR3Path);
    sprintf(szBuffer, "%d", R3EngineConfig.RenderState.RenderBits);
    WritePrivateProfileStringA("RenderState", "RenderBits", szBuffer, s_pszR3Path);
    sprintf(szBuffer, "%d", R3EngineConfig.RenderState.AntiAliasing);
    WritePrivateProfileStringA("RenderState", "AntiAliasing", szBuffer, s_pszR3Path);
    sprintf(szBuffer, "%.1f", R3EngineConfig.RenderState.Gamma);
    WritePrivateProfileStringA("RenderState", "Gamma", szBuffer, s_pszR3Path);
    sprintf(szBuffer, "%d", R3EngineConfig.RenderState.BBoShasi);
    WritePrivateProfileStringA("RenderState", "BBoShasi", szBuffer, s_pszR3Path);
    WritePrivateProfileStringA("RenderState", "bMouseAccelation", R3EngineConfig.RenderState.bMouseAccelation ? "TRUE" : "FALSE", s_pszR3Path);
    WritePrivateProfileStringA("Sound", "Sound", R3EngineConfig.Sound.Sound ? "TRUE" : "FALSE", s_pszR3Path);
    sprintf(szBuffer, "%.1f", R3EngineConfig.Sound.SoundVol);
    WritePrivateProfileStringA("Sound", "SoundVol", szBuffer, s_pszR3Path);
    WritePrivateProfileStringA("Sound", "Music", R3EngineConfig.Sound.Music ? "TRUE" : "FALSE", s_pszR3Path);
    sprintf(szBuffer, "%.1f", R3EngineConfig.Sound.MusicVol);
    WritePrivateProfileStringA("Sound", "MusicVol", szBuffer, s_pszR3Path);
    sprintf(szBuffer, "%.1f", R3EngineConfig.Sound.AmbVol);
    WritePrivateProfileStringA("Sound", "AmbVol", szBuffer, s_pszR3Path);
    sprintf(szBuffer, "%d", R3EngineConfig.Launcher.Language);
    WritePrivateProfileStringA("Game", "Language", szBuffer, s_pszR3Path);
    return true;
}

bool isValidHexDigit(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

// Function to convert binary data to hexadecimal string
std::string toHex(const std::string& data) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned char c : data) {
        oss << std::setw(2) << static_cast<int>(c);
    }
    return oss.str();
}
// Function to convert binary data to hexadecimal string
std::string toHex(const std::vector<unsigned char>& data) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (unsigned char byte : data) {
        ss << std::setw(2) << static_cast<unsigned int>(byte);
    }
    return ss.str();
}

// Function to convert hexadecimal string to binary data
std::string fromHex(const std::string& hex) {
    // Validate hexadecimal string
    for (char c : hex) {
        if (!isValidHexDigit(c)) {
            // If any character is not a valid hexadecimal digit, return an empty string
            return "";
        }
    }

    std::string result;
    for (size_t i = 0; i < hex.length(); i += 2) {
        result.push_back(std::stoi(hex.substr(i, 2), nullptr, 16));
    }
    return result;
}


void loadAccounts(std::string filename) {
    // Read JSON file
    std::ifstream file(filename);
    if (!file.is_open()) {
        return;
    }

    // Parse JSON
    Json::Value root;
    file >> root;

    // Iterate over accounts
    for (const auto& account : root["account"]) {
        Account acc;
        // Convert hex to binary
        std::string encryptedUser = fromHex(account.get("loginID", "").asString());
        std::string encryptedPassword = fromHex(account.get("password", "").asString());

        // Decrypt Username
        std::string decryptedUser = c_Patcher.decrypt(encryptedUser, enckey);
        // Decrypt password
        std::string decryptedPassword = c_Patcher.decrypt(encryptedPassword, enckey);
        if (!decryptedUser.empty() && !decryptedPassword.empty() ) {
            strcpy_s(acc.m_szLoginID, decryptedUser.c_str());
            strcpy_s(acc.m_szPW, decryptedPassword.c_str());
            m_AccountInfoWidget.accounts.push_back(acc);
        }
    }

    // Close file
    file.close();
}

bool findAccounts(const char* username) {
    for (const auto& account : m_AccountInfoWidget.accounts) {
        if (std::strcmp(account.m_szLoginID, username) == 0) {
            return true; // Username exists
        }
    }
    return false;
}

void addAccounts(const char* username, const char* password) {
    if (!findAccounts(username)){
        Account newAccount;
        strcpy_s(newAccount.m_szLoginID, username);
        strcpy_s(newAccount.m_szPW, password);
        // Set password as needed
        // std::strcpy(newAccount.m_szPW, "password"); // Set password
        m_AccountInfoWidget.accounts.push_back(newAccount);
    }
}

bool removeAccounts(const char* username) {
    if (findAccounts(username)) {
        auto it = std::remove_if(m_AccountInfoWidget.accounts.begin(), m_AccountInfoWidget.accounts.end(), [username](const Account& account) {
            return std::strcmp(account.m_szLoginID, username) == 0;
        });
        m_AccountInfoWidget.accounts.erase(it, m_AccountInfoWidget.accounts.end());
        return true;
    }
    return false;
}

// Function to save accounts to JSON file and encrypt passwords
void saveAccounts(const std::vector<Account> accounts, const std::string& filename, const std::string& key) {
    // Create JSON root object
    Json::Value root;
    Json::Value accountsArray(Json::arrayValue);

    // Iterate over accounts
    for (const auto& acc : accounts) {

        // Convert encrypted user to hex
        std::vector<unsigned char> encryptedUser = c_Patcher.encrypt(acc.m_szLoginID, key);
        // Convert encrypted password to hex
        std::vector<unsigned char> encryptedPW = c_Patcher.encrypt(acc.m_szPW, key);

        std::string encryptedUserHex = toHex(encryptedUser);
        std::string encryptedPWHex = toHex(encryptedPW);
        // Create JSON object for account
        Json::Value account;
        account["Nickaccount"] = acc.m_szLoginID;
        account["loginID"] = encryptedUserHex;
        account["password"] = encryptedPWHex;

        // Add account to accounts array
        accountsArray.append(account);
    }

    // Add accounts array to root
    root["account"] = accountsArray;

    // Write JSON to file
    std::ofstream file(filename);
    if (file.is_open()) {
        file << root;
        file.close();
        //std::cout << "Accounts saved to " << filename << std::endl;
    }
    /*else {
        std::cerr << "Failed to open file: " << filename << std::endl;
    }*/
}

// Function to create a new JSON object with specified structure
Json::Value createNewBlacklistJSON() {
    Json::Value root;
    // Add excludeAutoUpdates array
    Json::Value excludeAutoUpdates(Json::arrayValue);
    root["excludeAutoUpdates"] = excludeAutoUpdates;

    return root;
}

bool loadBlacklist(std::string jsonFilePath, bool CreateNotExist = false) {
    // Check if JSON file exists
    std::ifstream file(jsonFilePath);
    if (!file.good()) {
        if (CreateNotExist) {
            // Create a new JSON object with default structure
            Json::Value root = createNewBlacklistJSON();

            // Convert JSON to string
            Json::StreamWriterBuilder writer;
            std::string jsonString = Json::writeString(writer, root);

            // Write new JSON to file
            std::ofstream newFile(jsonFilePath);
            newFile << jsonString;
            newFile.close();
        }
        return false;
    }
    else {
        file.close();
    }

    // Read JSON file content into string
    std::ifstream readFile(jsonFilePath);
    std::string fileContent((std::istreambuf_iterator<char>(readFile)), std::istreambuf_iterator<char>());
    readFile.close();

    // Parse JSON
    Json::Value root;
    std::istringstream(fileContent) >> root;

    // Push excludeAutoUpdates to vector blacklistAutoUpdate
    c_Patcher.pushExcludeAutoUpdatesToVector(root, c_Patcher.blacklistAutoUpdate);
    return true;
}

// Function to save blacklist list to JSON file
void saveBlacklistList(const std::string& filePath, const std::vector<std::string>& blacklist) {
    // Create a JSON object with the blacklist
    Json::Value root(Json::objectValue);
    Json::Value blacklistArray(Json::arrayValue);
    for (const auto& item : blacklist) {
        blacklistArray.append(item);
    }
    root["excludeAutoUpdates"] = blacklistArray;

    // Convert JSON to string
    Json::StreamWriterBuilder writer;
    std::string jsonString = Json::writeString(writer, root);

    // Write JSON to file
    std::ofstream outFile(filePath);
    outFile << jsonString;
    outFile.close();
}

// Function to get graphics card information using EnumDisplayDevicesA
std::vector<std::string> GetGraphicsCardInfo()
{
    std::vector<std::string> cardInfo;

    DISPLAY_DEVICE displayDevice;
    displayDevice.cb = sizeof(displayDevice);

    DWORD deviceIndex = 0;
    while (EnumDisplayDevicesA(nullptr, deviceIndex, &displayDevice, 0))
    {
        // Check if the device is a GPU (graphics adapter)
        if (displayDevice.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
        {
            // Primary graphics adapter found, retrieve its information
            std::string cardName = displayDevice.DeviceString;
            cardInfo.push_back(cardName);
        }

        // Move to the next display device
        ++deviceIndex;
    }

    std::string textGraphicsSelected;
    textGraphicsSelected = R3EngineConfig.RenderState.Adapter;

    for (int i = 0; i < cardInfo.size(); ++i)
    {
        if (cardInfo[i] == textGraphicsSelected)
        {
            selectedGraph = i;
            break;
        }
    }

    return cardInfo;
}

std::vector<std::string> GetResolution() {
    LPDIRECT3D9 videoDevice = Direct3DCreate9(
        D3D_SDK_VERSION);
    std::vector<std::string> rsx;
    typedef std::pair< UINT, UINT > Resolution;
    typedef std::set< Resolution > ResolutionSet;
    ResolutionSet resolutionSet;

    //현재 모니터의 해상도만 얻어오도록 변경
    UINT	adapterIndex = 0;

    D3DADAPTER_IDENTIFIER9 adapterIdentifier = { 0 };

    if (FAILED(videoDevice->GetAdapterIdentifier(adapterIndex, D3DENUM_WHQL_LEVEL, &adapterIdentifier)))
    {
        return rsx;
    }

    D3DCAPS9 caps;
    ZeroMemory(&caps,sizeof(caps));

    if (FAILED(videoDevice->GetDeviceCaps( adapterIndex,D3DDEVTYPE_HAL,&caps)))
    {
        return rsx;
    }

    const UINT displayModeSize = videoDevice->GetAdapterModeCount(adapterIndex, D3DFMT_X8R8G8B8);

    for (UINT modeIndex = 0; modeIndex < displayModeSize; ++modeIndex)
    {
        D3DDISPLAYMODE displayMode = { 0 };
        //D3DFMT_DXT1
        if (FAILED(videoDevice->EnumAdapterModes(adapterIndex,D3DFMT_X8R8G8B8,modeIndex,&displayMode)))
        {
            continue;
        }
        else if (640 > displayMode.Width)
        {
            continue;
        }
        else if (480 > displayMode.Height)
        {
            continue;
        }
        //displayMode.dmBitsPerPel displayMode.dmDisplayFrequency;
        const Resolution resolution(
            displayMode.Width,
            displayMode.Height);

        if (resolutionSet.end() != resolutionSet.find(resolution))
        {
            continue;
        }

        resolutionSet.insert(resolution);

        TCHAR textResolution[MAX_PATH] = { 0 };
        _stprintf( textResolution, _T("%d x %d"),  resolution.first,   resolution.second);

        rsx.push_back(textResolution);
        //mResolutionCombo.AddString(textResolution);
    }

    std::string textResolutionSelected;
    textResolutionSelected = std::to_string(R3EngineConfig.RenderState.ScreenXSize) + " x " + std::to_string(R3EngineConfig.RenderState.ScreenYSize);

    for (int i = 0; i < rsx.size(); ++i)
    {                
        if (rsx[i] == textResolutionSelected)
        {
            selectedRes = i;
            break;
        }
    }

    return rsx;
}

void setTabs(int xTabs, bool ForceChange) {
    tabs = xTabs;
    if (ForceChange) {
        active_tab = xTabs;
    }
}

void setErrMessage(char* text, bool setCrit, bool ChangeTab) {
    strcpy_s(errMsg, text);
    if (ChangeTab) {
        setTabs(-1, true);
    }
    PushBackNotify(3, text, 3000);
    if (setCrit) {
        critError = true;
    }
}

void ApplyServerSettings(bool firstLaunch) {
    char bero[255];
    if (firstLaunch) {
        int numServer = c_Patcher.ServerListConfiguration.size();
        if (numServer == 0) {
            selectedServer = 0;
        }
        else {
            if (selectedServer >= numServer) {
                selectedServer = 0;
            }
            int numGate = c_Patcher.ServerListConfiguration[selectedServer].Gates.size();
            if (numGate == 0) {
                selectedGate == 0;
            }
            if (selectedGate >= numGate) {
                selectedGate == 0;
            }
        }
        showServerList = true;
    }
    c_Patcher.patchURL = c_Patcher.ServerListConfiguration[selectedServer].patchURL;
    c_Patcher.playerAPI = c_Patcher.ServerListConfiguration[selectedServer].playerAPI;
    c_Patcher.TopPlayerURL = c_Patcher.ServerListConfiguration[selectedServer].TopPlayerURL;
    if (c_Patcher.ServerListConfiguration[selectedServer].ServerType) {
        SirinMode = true;
        g_ConfigMgr.m_strLoginAddr = c_Patcher.ServerListConfiguration[selectedServer].ServerIP;
        g_ConfigMgr.m_wLoginPort = c_Patcher.ServerListConfiguration[selectedServer].ServerLoginPort;
        g_ConfigMgr.m_byClientVer = c_Patcher.ServerListConfiguration[selectedServer].ServerVersion;
        g_ConfigMgr.ApplySettings();
    }
    else {
        //Set If Default
        SirinMode = false;
        inet_pton(AF_INET, c_Patcher.ServerListConfiguration[selectedServer].ServerIP.c_str(), &g_Main.m_dwLoginServerIP);
        g_Main.m_wLoginPort = c_Patcher.ServerListConfiguration[selectedServer].ServerLoginPort;
        g_Main.m_dwTempDataVer = c_Patcher.ServerListConfiguration[selectedServer].ServerVersion;
    }

    int numGate = c_Patcher.ServerListConfiguration[selectedServer].Gates.size();
    if (numGate > 0) {
        if (c_Patcher.ServerListConfiguration[selectedServer].ServerType) {
            g_ConfigMgr.m_bOverrideZoneAddr = true;
            g_ConfigMgr.m_strZoneAddr = c_Patcher.ServerListConfiguration[selectedServer].Gates[selectedGate].GateIP;
            g_ConfigMgr.m_wZonePort = c_Patcher.ServerListConfiguration[selectedServer].Gates[selectedGate].GatePort;
            g_ConfigMgr.ApplySettings();
        }
        else {
            //Set If Default
            g_Main.m_nIsWorldAddrOverride = true;
            inet_pton(AF_INET, c_Patcher.ServerListConfiguration[selectedServer].Gates[selectedGate].GateIP.c_str(), &g_Main.m_dwWorldIP);
            g_Main.m_wWorldPort = c_Patcher.ServerListConfiguration[selectedServer].Gates[selectedGate].GatePort;
        }
        sprintf_s(bero, "Server Game : %s , Server Gate : %s !", c_Patcher.ServerListConfiguration[selectedServer].ServerName.c_str(), c_Patcher.ServerListConfiguration[selectedServer].Gates[selectedGate].GateName.c_str());
    }
    else {
        g_ConfigMgr.m_bOverrideZoneAddr = false;
        g_Main.m_nIsWorldAddrOverride = false;
        sprintf_s(bero, "Server Game : %s , Server Gate : Default !", c_Patcher.ServerListConfiguration[selectedServer].ServerName.c_str());
    }
    PushBackNotify(4, bero);
    g_ConfigMgr.ApplySettings();
    SaveServerConfig();

    if (!firstLaunch) {
        if (SirinMode) {
            Sirin_Logout();
            g_ConfigMgr.Logout();
        }else{
            g_Main.cmd_LogOut();
        }
        c_Patcher.filesToUpdate.clear();
        c_Patcher.NeedUpdate = false;
        c_Patcher.ReqUpdate = false;
        c_Patcher.PatchComplete = false;
        c_Patcher.Patchchecked = false;
        calledRenderImgNews = false;
        calledCheckUpdate = false;
        renderAllNews = false;
        calledNews = false;
        c_News.m_lNews = false;
        c_News.ListNews.clear();
        c_News.playersTopLevel.clear();
        c_News.playersTopKill.clear();
        c_News.playersTopCpt.clear();
    }
}

void ChangeshowWaitLogin(bool show) {
    if (!show) {
        showWaitLogin = false;
        showLoginPanel = true;
    }
}
void ExitProcessAfterFewMins(){

}

void ProgressBar(const char* name, float progress, float max, float width, float height)
{
    static float tickness = 3.f;
    static float position = 0.f;
    static float alpha_text = 1.f;

    ImVec4 progress_color = ImColor(0, 0, 0, 0);
    ImVec2 vecCenter = { ImGui::GetContentRegionMax() / 2 };

    position = progress / max;

    ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(vecCenter.x - width / 2, vecCenter.y - height / 2),
        ImVec2(vecCenter.x + width / 2, vecCenter.y + height / 2),
        ImGui::GetColorU32(progress_color), 0, 0);

    ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(vecCenter.x - width / 2, vecCenter.y - height / 2),
        ImVec2(vecCenter.x - width / 2 + width * position, vecCenter.y + height / 2),
        ImGui::GetColorU32(c::accent), 0, 0);

    int percent = static_cast<int>((progress / max) * 100);
    std::string percent_str = std::to_string(percent) + "%";

    ImGui::PushFont(font::lexend_regular);

    ImGui::GetForegroundDrawList()->AddText(vecCenter - ImGui::CalcTextSize(percent_str.c_str()) / 2,
        ImGui::GetColorU32(c::text::text_active),
        percent_str.c_str());

    alpha_text = ImLerp(alpha_text, percent > 80 ? 0.f : 1.f, ImGui::GetIO().DeltaTime * 6);

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha_text);
    if (percent != 0)
        ImGui::GetForegroundDrawList()->AddText(ImVec2((ImGui::GetContentRegionMax().x - ImGui::CalcTextSize(name).x) / 2,
            ImGui::GetContentRegionMax().y - alpha_text * 60),
            ImGui::GetColorU32(c::red), name);
    ImGui::PopStyleVar();

    ImGui::PopFont();
}

void move_window() {

    ImGui::SetCursorPos(ImVec2(0, 0));
    if (ImGui::InvisibleButton("Move_detector", ImVec2(c::background::size)));
    if (ImGui::IsItemActive()) {
        GetWindowRect(hwnd, &rc);
        MoveWindow(hwnd, rc.left + ImGui::GetMouseDragDelta().x, rc.top + ImGui::GetMouseDragDelta().y, c::background::size.x, c::background::size.y, TRUE);
    }

}

void PushBackNotify(int iconType, LPCSTR str, int dismissTime) {
    /*
    case ImGuiToastType_None:
    return NULL;
    case ImGuiToastType_Success:
    return "Success";
    case ImGuiToastType_Warning:
    return "Warning";
    case ImGuiToastType_Error:
    return "Error";
    case ImGuiToastType_Info:
    return "Info";*/
    ImGuiToast toast(iconType, dismissTime); // <-- content can also be passed here as above
    toast.set_title(str);
    ImGui::InsertNotification(toast);
}

int accountNo = -1;
char MenuList[7][25] = { "Home", "Configuration", "Download Settings", "Server Settings", "Launching Game", "No Response", "Example"};
bool b2FAEnableConfirmPending = false;
bool b2FAAuthConfirmPending = false;

std::string QRLink;
bool m_bShowTrust = false;
bool m_bHaveQR = false;

std::string urlencode(std::string s)
{
    auto hexchar = [=](unsigned char c, unsigned char& hex1, unsigned char& hex2)
        {
            hex1 = c / 16;
            hex2 = c % 16;
            hex1 += hex1 <= 9 ? '0' : 'a' - 10;
            hex2 += hex2 <= 9 ? '0' : 'a' - 10;
        };

    const char* str = s.c_str();
    std::string e;

    for (size_t i = 0, l = s.size(); i < l; i++)
    {
        char c = str[i];

        if ((c >= '0' && c <= '9') ||
            (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            c == '-' || c == '_' || c == '.' || c == '~')
        {
            e += c;
        }
        else if (c == ' ')
        {
            e += "%20";
        }
        else
        {
            e += '%';
            unsigned char d1, d2;
            hexchar(c, d1, d2);
            e += (char)d1;
            e += (char)d2;
        }
    }

    return e;
}


std::string removeCurrentDirFromPath(const fs::path& absoluteFilePath, const fs::path& absoluteCurrentDir) {
    std::string filePathStr = fs::absolute(absoluteFilePath).generic_string(); // Convert to string
    std::string currentDirStr = fs::absolute(absoluteCurrentDir).generic_string(); // Convert to string

    // Ensure current directory path ends with a separator
    if (!currentDirStr.empty() && currentDirStr.back() != fs::path::preferred_separator) {
        currentDirStr += fs::path::preferred_separator;
    }

    // Normalize path separators to forward slashes
    std::replace(filePathStr.begin(), filePathStr.end(), '\\', '/');
    std::replace(currentDirStr.begin(), currentDirStr.end(), '\\', '/');

    // Check if absoluteFilePath starts with absoluteCurrentDir
    if (filePathStr.compare(0, currentDirStr.size(), currentDirStr) == 0) {
        // Remove current directory path from the file path
        filePathStr.erase(0, currentDirStr.size());
    }

    return filePathStr;
}

void MakeQR(const std::string& link) {
    /*
    auto widgetQR = new CQRWidget(this);
    widgetQR->SetData(link);
    auto l = qobject_cast<QVBoxLayout*>(layout());
    l->insertWidget(-1, widgetQR);
    m_bHaveQR = true;
    ui.checkLost2FADevice->hide();
    */
}
void ShowTrustCheck() {

    //ui.checkTrust->show();
}

void FitContent()
{
    if (m_bHaveQR)
    {
        /*resize(250, 315);
        setMinimumSize(QSize(250, 315));
        setMaximumSize(QSize(250, 315));
        */
    }
}

void SecondFactorPrompt(bool bTrust /*= false*/, bool bQR /*= false*/)
{
    if (bQR)
        MakeQR(QRLink);

    if (bTrust)
        ShowTrustCheck();

    FitContent();
}


void CallbackHandler(unsigned char byType, unsigned char bySubType, unsigned char byResult, const void* pData)
{
    std::string data;
    char bebek[255];
    do
    {
        if (byResult == 0xFF) {

            // Assuming pData is a wide character string
            const wchar_t* wideData = reinterpret_cast<const wchar_t*>(pData);
            // Convert wide character string to std::wstring
            std::wstring wstr(wideData);
            // Convert std::wstring to std::string
            std::string errorMessage(wstr.begin(), wstr.end());
            // Convert std::string to a modifiable C-style string
            char* errorMessageCStr = const_cast<char*>(errorMessage.c_str());
            setErrMessage(errorMessageCStr);
        }
        else if (byResult == 0xFE)
        {
            char* errorMessage = const_cast<char*>(reinterpret_cast<const char*>(pData));
            setErrMessage(errorMessage);
        }
        else if (byResult == 0xFD)
        {
            g_ConfigMgr.Logout();
            GMMode = false;
            break;
        }
        else if (byResult == 0xFC)
        {
            switch (bySubType)
            {
            case param_ban_remain:
                g_ConfigMgr.SetRemainBan(*(unsigned*)pData);
                break;
            case param_chat_lock_remain:
                g_ConfigMgr.SetRemainChatLock(*(unsigned*)pData);
                break;
            case param_billing_type:
                g_ConfigMgr.SetBillingType(*(unsigned char*)pData);
                break;
            case param_billing_remain:
                g_ConfigMgr.SetRemainBiling(*(unsigned*)pData);
                break;
            case param_premium_remain:
                g_ConfigMgr.SetRemainPremium(*(unsigned*)pData);
                break;
            case param_cash_value:
                g_ConfigMgr.isLoggedIn = true;
                showLoginPanel = true;
                showWaitLogin = false;
                g_ConfigMgr.SetCash(*(unsigned*)pData);
                saveAccounts(m_AccountInfoWidget.accounts, JSONACCLIST, enckey);
                tabs = 0;
                break;
            case param_2fa_status:
                g_ConfigMgr.Set2FAStatus(*(bool*)pData);
                break;
            case param_capture_status:
                g_ConfigMgr.SetCaptureStatus(*(bool*)pData);
                break;
            default:
                sprintf_s(bebek, "Update value Error! Unknown type: %d", bySubType);
                setErrMessage(bebek);
                //data.append(tr("<font color=red>Update value Error! Unknown type: %1</font>").arg(bySubType));
                break;
            }

            break;
        }
        else
        {
            bool bHandled = true;
            switch (byResult)
            {
            case err_lulo_fail_timeout:
                setErrMessage("Too many requests!");
                break;
            case err_lulo_fail_not_authed:
                //data.append(g_MainWindow->qstrRedFormated.arg(tr("Authentication required!")));
                setErrMessage("Authentication required!");
                break;
            case err_lulo_2f_confirm_required:
                if (bySubType == pt_lulo_2f_enable_rsp && !g_ConfigMgr.Get2FAStatus())
                {
                    b2FAEnableConfirmPending = true;
                    b2FAAuthConfirmPending = false;
                    auto pszTOTPLinkPattern = "otpauth://totp/%s:%s?secret=%s&issuer=%s";
                    auto strID = urlencode(g_ConfigMgr.m_strUserName);
                    auto strIssuer = urlencode("RF Online");
                    auto Len = _scprintf(pszTOTPLinkPattern, strIssuer.c_str(), strID.c_str(), (const char*)pData, strIssuer.c_str());
                    QRLink = pszTOTPLinkPattern + strIssuer + strID + reinterpret_cast<const char*>(pData) + strIssuer;
                }
                else if (bySubType == pt_lulo_auth_rsp)
                {
                    b2FAAuthConfirmPending = true;
                    b2FAEnableConfirmPending = false;
                }
                else
                {
                    b2FAAuthConfirmPending = false;
                    b2FAEnableConfirmPending = false;
                }
                SecondFactorPrompt(b2FAAuthConfirmPending, b2FAEnableConfirmPending);
                //QMetaObject::invokeMethod(g_MainWindow, "SecondFactorPrompt", Qt::QueuedConnection, Q_ARG(bool, b2FAAuthConfirmPending), Q_ARG(bool, b2FAEnableConfirmPending));
                break;
            case err_lulo_fail_2f_code:
                setErrMessage("2FA: wrong code!");
                //data.append(g_MainWindow->qstrRedFormated.arg(tr("2FA: wrong code!")));
                break;
            case err_lulo_fail_internal_error:
                setErrMessage("Internal error!");
                //data.append(g_MainWindow->qstrRedFormated.arg(tr("Internal error!")));
                break;
            case err_lulo_fail_data_error:
                setErrMessage("Invalid input data!");
                //data.append(g_MainWindow->qstrRedFormated.arg(tr("Invalid input data!")));
                break;
            case err_lulo_fail_incorrect_state:
                setErrMessage("Invalid state to use this operation!");
                //data.append(g_MainWindow->qstrRedFormated.arg(tr("Invalid state to use this operation!")));
                break;
            case err_lulo_fail_feature_disabled:
                setErrMessage("Feature disabled!");
                //data.append(g_MainWindow->qstrRedFormated.arg(tr("Feature disabled!")));
                break;
            case err_lulo_fail_closed:
                setErrMessage("Request fail! Login closed.");
                //data.append(g_MainWindow->qstrRedFormated.arg(tr("Request fail! Login closed.")));
                break;
            case err_lulo_fail_banned_hwid:
                setErrMessage("Request fail! Computer banned.");
                //data.append(g_MainWindow->qstrRedFormated.arg(tr("Request fail! Computer banned.")));
                break;
            default:
                bHandled = false;
                break;
            }

            if (bHandled)
                break;

            if (byType != 0)
            {
                sprintf_s(bebek, "Callback Error! Type: %d SubType: %d Result: %d", byType, bySubType, byResult);
                setErrMessage(bebek);
                //data.append(tr("<font color=red>Callback Error! Type: %1 SubType: %2 Result: %3</font>").arg(byType).arg(bySubType).arg(byResult));
                break;
            }

            switch (bySubType)
            {
            case pt_lulo_register_rsp:
                if (byResult == err_lulo_register_success)
                    //data.append(tr("Register success!"));
                    setErrMessage("Register success!");
                else if (byResult == err_lulo_register_fail_duplicate){
                    setErrMessage("Register fail! Already exist.");
                    //data.append(g_MainWindow->qstrRedFormated.arg(tr("Register fail! Already exist.")));
                }
                else
                {
                    sprintf_s(bebek, "Register Callback Error! Result: %u", byResult);
                    setErrMessage(bebek);
                //data.append(tr("<font color=red>Register Callback Error! Result: %u</font>").arg(byResult));
                }
                break;
            case pt_lulo_auth_rsp:
                switch (byResult)
                {
                case err_lulo_auth_success:
                    b2FAAuthConfirmPending = false;
                    //QMetaObject::invokeMethod(g_MainWindow, "CloseSecondFactorPrompt", Qt::QueuedConnection);
                    m_AccountInfoWidget.CloseSecondFactorPrompt();
                    //QMetaObject::invokeMethod(g_MainWindow, "SwitchWidget", Qt::QueuedConnection, Q_ARG(bool, true));
                    break;
                case err_lulo_auth_fail_id_or_pass:
                    //data.append(g_MainWindow->qstrRedFormated.arg(tr("Login fail! Wrong login and/or password.")));
                    setErrMessage("Login fail! Wrong login and/or password.");
                    removeAccounts(g_ConfigMgr.GetUserName());
                    break;
                case err_lulo_auth_fail_banned:
                    setErrMessage("Login fail! Account banned.");
                    //data.append(g_MainWindow->qstrRedFormated.arg(tr("Login fail! Account banned.")));
                    break;
                case err_lulo_auth_capture_fail_id:
                    setErrMessage("Login fail! Capturing account not found.");
                    //data.append(g_MainWindow->qstrRedFormated.arg(tr("Login fail! Capturing account not found.")));
                    break;
                case err_lulo_auth_capture_fail_permission:
                    setErrMessage("Login fail! Account capture not permitted.");
                    //data.append(g_MainWindow->qstrRedFormated.arg(tr("Login fail! Account capture not permitted.")));
                    break;
                default:
                    sprintf_s(bebek, "Auth Callback Error! Result: %u", byResult);
                    setErrMessage(bebek);
                    //data.append(tr("<font color=red>Auth Callback Error! Result: %u</font>").arg(byResult));
                    break;
                }
                break;
            case pt_lulo_2f_auth_rsp:
                if (byResult == err_lulo_2f_enable_operation_cancel)
                {
                    b2FAAuthConfirmPending = false;
                    b2FAEnableConfirmPending = false;
                    m_AccountInfoWidget.CloseSecondFactorPrompt();
                    setErrMessage("2F confirm canceled.");
                    //QMetaObject::invokeMethod(g_MainWindow, "CloseSecondFactorPrompt", Qt::QueuedConnection);
                    //data.append(g_MainWindow->qstrRedFormated.arg(tr("2F confirm canceled.")));
                }

                break;
            case pt_lulo_2f_enable_rsp:
                if (byResult == err_lulo_2f_enable_success)
                {
                    b2FAEnableConfirmPending = false;
                    m_AccountInfoWidget.CloseSecondFactorPrompt();
                    //QMetaObject::invokeMethod(g_MainWindow, "CloseSecondFactorPrompt", Qt::QueuedConnection);

                    if (g_ConfigMgr.Get2FAStatus())
                        setErrMessage("2FA enabled.");
                    //data.append(tr("2FA enabled."));
                    else
                        setErrMessage("2FA disabled. All trusted PCs removed.");
                    //data.append(tr("2FA disabled. All trusted PCs removed."));

                //QMetaObject::invokeMethod(g_MainWindow->m_AccountInfoWidget, "Switch2FAButtonText", Qt::QueuedConnection);
                    m_AccountInfoWidget.Switch2FAButtonText();
                }
                else if (byResult == err_lulo_2f_enable_fail_already_done)
                    setErrMessage("2F toggle fail! Already enabled/disabled.");
                //data.append(g_MainWindow->qstrRedFormated.arg(tr("2F toggle fail! Already enabled/disabled.")));
                else {
                    sprintf_s(bebek, "2FA toggle Callback Error! Result: %u", byResult);
                    setErrMessage("2F toggle fail! Already enabled/disabled.");
                    ////data.append(tr("<font color=red>2FA toggle Callback Error! Result: %u</font>").arg(byResult));
                }                    

                break;
            case pt_lulo_2f_untrust_rsp:
                if (byResult == err_lulo_2f_untrust_success)
                    setErrMessage("Remove from trusted success.");
                    //data.append(tr("Remove from trusted success."));
                else if (byResult == err_lulo_2f_untrust_fail_2f_disabled)
                    setErrMessage("Remove from trusted failed! 2FA disabled.");
                    //data.append(g_MainWindow->qstrRedFormated.arg(tr("Remove from trusted failed! 2FA disabled.")));
                else {
                    sprintf_s(bebek, "Remove from trusted Callback Error! Result: %u", byResult);
                    setErrMessage(bebek);
                    //data.append(tr("<font color=red>Remove from trusted Callback Error! Result: %u</font>").arg(byResult));
                }
                break;
            case pt_lulo_change_pass_rsp:
                if (byResult == err_lulo_change_pass_success)
                    setErrMessage("Change password success.");
                //data.append("Change password success."));
                else if (byResult == err_lulo_change_pass_fail_wrong_pass)
                    setErrMessage("Change password failed. Wrong current password.");
                //data.append("Change password failed. Wrong current password."));
                else {
                    sprintf_s(bebek, "Change password Callback Error! Result: %u", byResult);
                    setErrMessage(bebek);
                }
                //data.append("<font color=red>Change password Callback Error! Result: %u</font>").arg(byResult));

                break;
            case pt_lulo_reset_pass_rsp:
                if (byResult == err_lulo_reset_pass_success)
                    setErrMessage("Reset password success.");
                //data.append("Reset password success."));
                else if (byResult == err_lulo_reset_pass_fail_wrong_id)
                    setErrMessage("Reset password failed. Login not found.");
                //data.append("Reset password failed. Login not found."));
                else if (byResult == err_lulo_reset_pass_fail_2f_disabled)
                    setErrMessage("Reset password failed. 2FA required but disabled.");
                //data.append("Reset password failed. 2FA required but disabled."));
                else {
                    sprintf_s(bebek, "Reset password Callback Error! Result: %u", byResult);
                    setErrMessage(bebek);
                    //data.append("<font color=red>Reset password Callback Error! Result: %u</font>").arg(byResult));
                }

                break;
            case pt_lulo_help_rsp:
                if (byResult == err_lulo_help_success)
                    setErrMessage("Help request success.");
                //data.append("Help request success."));
                else {
                    sprintf_s(bebek, "Help request Callback Error! Result: %u", byResult);
                    setErrMessage(bebek);
                    //data.append("<font color=red>Help request Callback Error! Result: %u</font>").arg(byResult));
                }

                break;
            case pt_lulo_enter_world_rsp:
                switch (byResult)
                {
                case err_lulo_enter_world_success:
                    Sirin_Logout();
                    g_ConfigMgr.Logout();
                    showLoginPanel = false;
                    GMMode = false;
                    setTabs(0); 
                    //SendQuit();
                    //QMetaObject::invokeMethod(g_MainWindow, "SendQuit", Qt::QueuedConnection);
                    break;
                case err_lulo_enter_world_fail_push_require:
                    //PushPrompt();
                    //MessageBoxA(NULL, "err_lulo_enter_world_fail_push_require", "ERROR", NULL);
                    setErrMessage("Account already login in somewhere else, will pushing Loggin");
                    //QMetaObject::invokeMethod(g_MainWindow, "PushPrompt", Qt::QueuedConnection);
                    break;
                case err_lulo_enter_world_fail_gm_controlled:
                    setErrMessage("Enter world fail! Account captured by GM.");
                    //data.append("Enter world fail! Account captured by GM."));
                    break;
                case err_lulo_enter_world_push_pending:
                    setErrMessage("Push fail! Push operation already in progress.");
                    //data.append("Push fail! Push operation already in progress."));
                    break;
                case err_lulo_enter_world_push_close_wait:
                    setErrMessage("Push connection success!");
                    //data.append("Push connection success!"));
                    break;
                case err_lulo_enter_world_fail_max_online:
                    setErrMessage("Enter world fail! Max server online reached.");
                    //data.append("Enter world fail! Max server online reached."));
                    break;
                case err_lulo_enter_world_fail_billing_end:
                    setErrMessage("Enter world fail! Your subscription expired.");
                    //data.append("Enter world fail! Your subscription expired."));
                    break;
                default: 
                    sprintf_s(bebek, "Enter world Callback Error! Result: %u", byResult);
                    setErrMessage(bebek);
                    //setErrMessage("<font color=red>Enter world Callback Error! Result: %u</font>").arg(byResult);
                    //data.append("<font color=red>Enter world Callback Error! Result: %u</font>").arg(byResult));
                    break;
                }
                break;
            default:
                sprintf_s(bebek, "Callback Error! Sub: %u; Result: %u", bySubType, byResult);
                setErrMessage(bebek);
                //setErrMessage("<font color=red>Callback Error! Sub: %u; Result: %u</font>").arg(bySubType).arg(byResult);
                //data.append("<font color=red>Callback Error! Sub: %u; Result: %u</font>").arg(bySubType).arg(byResult));
                break;
            }
        }

    } while (false);

/*    if (!data.isEmpty())
        QMetaObject::invokeMethod(g_MainWindow, "AppendOutput", Qt::QueuedConnection, Q_ARG(QString, data));
*/
}

void* CreateTexFromData(const char* imageData, size_t imageSize, float& width, float& height) {
    D3DXIMAGE_INFO info;
    HRESULT hr = D3DXGetImageInfoFromFileInMemory(imageData, imageSize, &info);
    width = info.Width;
    height = info.Height;

    if (FAILED(hr)) {
        return nullptr;
    }

    IDirect3DTexture9* pTexture = nullptr;
    //HRESULT hr = D3DXCreateTextureFromFileInMemory(g_pd3dDevice, imageData, imageSize, &pTexture);
    hr = D3DXCreateTextureFromFileInMemoryEx(
        g_pd3dDevice,
        imageData,
        imageSize,
        info.Width,
        info.Height,
        D3DX_DEFAULT,
        0,
        D3DFMT_UNKNOWN,
        D3DPOOL_MANAGED,
        D3DX_DEFAULT,
        D3DX_DEFAULT,
        0,
        &info,
        NULL,
        &pTexture
    );


    if (FAILED(hr)) {
        // Failed to create texture
        if (pTexture) {
            pTexture->Release(); // Release the texture to avoid memory leaks
            pTexture = nullptr;
        }
        return nullptr;
    }

    return reinterpret_cast<void*>(pTexture);
}

void* CreateTexFromData(const char* imageData, size_t imageSize) {
    IDirect3DTexture9* pTexture = nullptr;  
    HRESULT hr = D3DXCreateTextureFromFileInMemory(g_pd3dDevice, imageData, imageSize, &pTexture);
    if (FAILED(hr)) {
        // Failed to create texture
        if (pTexture) {
            pTexture->Release(); // Release the texture to avoid memory leaks
            pTexture = nullptr;
        }
        return nullptr;
    }
    return reinterpret_cast<ImTextureID>(pTexture);
}

bool PrepareRenderImages(LPVOID param) {    
    std::vector<std::future<TextureInfo>> futures;
    for (const auto& item : c_News.ListNews) {
        futures.push_back(std::async(std::launch::async, LoadImageFromURL, item.imageURL.c_str()));
    }

    for (auto& future : futures) {
        TextureInfo textureInfo = future.get();
    }
    renderAllNews = true;
    allowRefresh = true;
    return true;
}


void RankSystem(int tab) {
    int lengthTable = 6;
    static ImGuiTableFlags flags =
        ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchSame;
    if (ImGui::BeginTable("myTable", lengthTable, flags)) {
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(10, 15));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 15));
        // Table headers
        ImGui::TableSetupColumn("Place", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Race", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("CPT", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Kill", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
        for (int column_n = 0; column_n < lengthTable; column_n++)
        {
            if (!ImGui::TableSetColumnIndex(column_n))
                continue;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - ImGui::CalcTextSize(ImGui::TableGetColumnName(column_n)).x) / 2);
            ImGui::TableHeader(ImGui::TableGetColumnName(column_n));
        }
        ImGui::PopStyleVar(2);
        // ImGuiListClipper for efficient scrolling        
        static ImGuiListClipper clipper;
        if (tab == 0) { //Level
            clipper.Begin(c_News.playersTopLevel.size());
        }else if (tab == 1) { //Kill
            clipper.Begin(c_News.playersTopKill.size());
        }else if (tab == 2) { //CPT
            clipper.Begin(c_News.playersTopCpt.size());
        }

        // Table data
        while (clipper.Step()) {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
                structTopPlayer item;
                if (tab == 0) { //Level
                    item = c_News.playersTopLevel[i];
                }
                else if (tab == 1) { //Kill
                    item = c_News.playersTopKill[i];
                }
                else if (tab == 2) { //CPT
                    item = c_News.playersTopCpt[i];
                }
                ImGui::TableNextRow();
                // Center-align text within each column
                for (int j = 0; j < lengthTable; ++j) {
                    ImGui::TableSetColumnIndex(j);        
                    char tempStr[17];// máximo de caracteres
                    switch (j) {
                        case 0:
                            sprintf_s(tempStr, "%d", i +1 );
                            break;
                        case 1:
                            sprintf_s(tempStr, "%s", item.name.c_str());
                            break;
                        case 2:
                            sprintf_s(tempStr, "%d", item.race);
                            break;
                        case 3:
                            sprintf_s(tempStr, "%s", item.level.c_str());
                            break;
                        case 4:
                            sprintf_s(tempStr, "%d", item.cpt);
                            break;
                        case 5:
                            sprintf_s(tempStr, "%d", item.kill);
                            break;
                        }
                    if (i == 0) {
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
                    }
                    if (j != 2) {
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (32 - ImGui::CalcTextSize(tempStr).y) / 2);
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - ImGui::CalcTextSize(tempStr).x) / 2);
                        ImGui::Text(tempStr);
                    }
                    else {
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - 32) / 2);
                        switch (item.race) {
                            case 0:
                            case 1:
                                ImGui::Image(texture::logo_bcc, ImVec2(32, 32));
                                break;
                            case 2:
                            case 3:
                                ImGui::Image(texture::logo_ccc, ImVec2(32, 32));
                                break;
                            case 4:
                                ImGui::Image(texture::logo_acc, ImVec2(32, 32));
                                break;
                            default:
                                break;
                        }
                    }
                }
            }
        }

        edited::ScrollWhenDragging(ImVec2(0.0f, -1.0f), 0);
        // End table
        ImGui::EndTable();
    }
}

void RenderCharInfo(Account* accountData) {
    int lengthTable = 5;
    static ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchSame;
    char fStr[50];
    sprintf_s(fStr, "Information about account : %s",  accountData->m_szLoginID);
    float centerX = (GetContentRegionMax().x + GetStyle().WindowPadding.x - ImGui::CalcTextSize(fStr).x) / 2;
    SetCursorPosX(centerX);
    Text("Information about account : ");
    SameLine(0, 5);
    TextColored(ImColor(GetColorU32(c::orange)), "%s", accountData->m_szLoginID);
    if (ImGui::BeginTable("charTable", lengthTable, flags)) {
        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(10, 15));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 15));
        // Table headers
        ImGui::TableSetupColumn("No", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Race", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Sellable", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
        for (int column_n = 0; column_n < lengthTable; column_n++)
        {
            if (!ImGui::TableSetColumnIndex(column_n))
                continue;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - ImGui::CalcTextSize(ImGui::TableGetColumnName(column_n)).x) / 2);
            ImGui::TableHeader(ImGui::TableGetColumnName(column_n));
        }
        ImGui::PopStyleVar(2);
        // ImGuiListClipper for efficient scrolling        
        static ImGuiListClipper clipper;
        clipper.Begin(accountData->charInfo.size());
        // Table data
        while (clipper.Step()) {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
                CharInfo item = accountData->charInfo[i];
                ImGui::TableNextRow();
                // Center-align text within each column
                for (int j = 0; j < lengthTable; ++j) {
                    ImGui::TableSetColumnIndex(j);
                    char tempStr[17];
                    switch (j) {
                    case 0:
                        sprintf_s(tempStr, "%d", i + 1);
                        break;
                    case 1:
                        sprintf_s(tempStr, "%s", item.Name.c_str());
                        break;
                    case 2:
                        sprintf_s(tempStr, "%d", item.Race);
                        break;
                    case 3:
                        sprintf_s(tempStr, "%d", item.level);
                        break;
                    case 4:
                        sprintf_s(tempStr, "%s", (item.Sellable ? "No restrictions" : "Restricted"));
                        break;
                    }
                    if (i == 0) {
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 5);
                    }
                    if (j != 2) {
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (32 - ImGui::CalcTextSize(tempStr).y) / 2);
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - ImGui::CalcTextSize(tempStr).x) / 2);
                        ImGui::Text(tempStr);
                    }
                    else {
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetColumnWidth() - 32) / 2);
                        switch (item.Race) {
                        case 0:
                        case 1:
                            ImGui::Image(texture::logo_bcc, ImVec2(32, 32));
                            break;
                        case 2:
                        case 3:
                            ImGui::Image(texture::logo_ccc, ImVec2(32, 32));
                            break;
                        case 4:
                            ImGui::Image(texture::logo_acc, ImVec2(32, 32));
                            break;
                        default:
                            break;
                        }
                    }
                }
            }
        }
        // End table
        ImGui::EndTable();
    }
    if (accountData->charInfo.size() == 0) {
        edited::TextCenter("No Character Found in Account %s", accountData->m_szLoginID);
    }

    ImGui::SetCursorPosY(ImGui::GetContentRegionMax().y + GetStyle().WindowPadding.y - 150);
    edited::Separator_line(); 
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 15.0f));
    ImGui::Dummy(ImVec2(5, 0));
    Text("   %s Cash Coin : %d", ICON_FA_COINS, accountData->CashCoin);
    Text("   %s Premium : ", ICON_FA_STAR_HALF_ALT);
    SameLine(0,5);
    if (accountData->PremiumDays > 0) {
        auto s = accountData->PremiumDays;
        auto d = s / 86400;
        s -= d * 86400;
        auto h = s / 3600;
        s -= h * 3600;
        auto m = s / 60;
        s -= m * 60;
        if (d > 0)
            Text("ends in %d day(s)", d);
        else if (h > 0)
            Text("ends in %d hour(s)", h);
        else if (m > 0)
            Text("ends in %d minute(s)", m);
        else if (s > 0)
            Text("ends in %d second(s)", s);
    }
    else {
        TextColored(ImColor(GetColorU32(c::red)), "None");
    }
    Text("   %s Account Ban Status : ", ICON_FA_USER_SLASH);
    SameLine(0, 5);
    if (accountData->idBan > 0) {
        if (accountData->idBan > 999) {
            TextColored(ImColor(GetColorU32(c::red)), "Permanent");
        }
        else {
            auto s = accountData->idBan;
            auto d = s / 86400;
            s -= d * 86400;
            auto h = s / 3600;
            s -= h * 3600;
            auto m = s / 60;
            s -= m * 60;
            if (d > 0)
                TextColored(ImColor(GetColorU32(c::red)), "ends in %d day(s)", d);
            else if (h > 0)
                TextColored(ImColor(GetColorU32(c::red)), "ends in %d hour(s)", h);
            else if (m > 0)
                TextColored(ImColor(GetColorU32(c::red)), "ends in %d minute(s)", m);
            else if (s > 0)
                TextColored(ImColor(GetColorU32(c::red)), "ends in %d second(s)", s);
        }
    }
    else
        TextColored(ImColor(GetColorU32(c::green)), "None");
    Text("   %s Chat Ban Status : ", ICON_FA_COMMENT_SLASH);
    SameLine(0, 5);
    if (accountData->Chatban > 0) {
        if (accountData->Chatban > 999) {
            TextColored(ImColor(GetColorU32(c::red)), "Permanent");
        }
        else {
            auto s = accountData->Chatban;
            auto d = s / 86400;
            s -= d * 86400;
            auto h = s / 3600;
            s -= h * 3600;
            auto m = s / 60;
            s -= m * 60;
            if (d > 0)
                TextColored(ImColor(GetColorU32(c::red)), "ends in %d day(s)", d);
            else if (h > 0)
                TextColored(ImColor(GetColorU32(c::red)), "ends in %d hour(s)", h);
            else if (m > 0)
                TextColored(ImColor(GetColorU32(c::red)), "ends in %d minute(s)", m);
            else if (s > 0)
                TextColored(ImColor(GetColorU32(c::red)), "ends in %d second(s)", s);
        }
    }
    else
        TextColored(ImColor(GetColorU32(c::green)), "None");
    ImGui::PopStyleVar();
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){
    WNDCLASSEXW wc;
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = NULL;
    wc.cbWndExtra = NULL;
    wc.hInstance = nullptr;
    wc.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszMenuName = LauncherNameX;
    wc.lpszClassName = L"TheBeginning";
    wc.hIconSm = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));

    RegisterClassExW(&wc);
    hwnd = CreateWindowExW(NULL, wc.lpszClassName, wc.lpszMenuName, WS_POPUP, (GetSystemMetrics(SM_CXSCREEN) / 2) - (c::background::size.x / 2), (GetSystemMetrics(SM_CYSCREEN) / 2) - (c::background::size.y / 2), c::background::size.x, c::background::size.y, 0, 0, 0, 0);

    SetWindowLongA(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);

    //ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.00f, 0.00f);
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    COLORREF colorKey = RGB(clear_color.x * 255, clear_color.y * 255, clear_color.z * 255);

    // Set the layered window attributes
    SetLayeredWindowAttributes(hwnd, colorKey, 255, ULW_COLORKEY);
    //SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
	//SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, ULW_COLORKEY); // Fix Integrated Alpha Color

    MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    HRGN hRgn = CreateRoundRectRgn(0, 0, c::background::size.x, c::background::size.y, 11, 11);

    // ”становить форму пути на окно
    SetWindowRgn(hwnd, hRgn, TRUE);

    POINT mouse;
    rc = { 0 };
    GetWindowRect(hwnd, &rc);

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    LoadR3EngineConfig();
    // Start Threading Patcher
    c_Patcher.InitPatchThread();
#ifdef DEBUG
    c_Patcher.CreateSample();
#endif DEBUG
    c_News.InitThread();
    resolutions = GetResolution();
    graphicCards = GetGraphicsCardInfo();
    Sirin_SetHandler(CallbackHandler);    
    g_ConfigMgr.ApplySettings();
    g_Main.InitMainThread();
    loadBlacklist(JSONBLAU, true);
    loadAccounts(JSONACCLIST);

    if (R3EngineConfig.Launcher.enableThrottle) {
        c_Patcher.enableThrottle = R3EngineConfig.Launcher.enableThrottle;
    }
    c_Patcher.throtleSpeedLimit = R3EngineConfig.Launcher.throtleSpeed;
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.IniFilename = NULL;
    io.LogFilename = NULL;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    // Setup Dear ImGui style
    //ImGui::StyleColorsDark(); // Edit Here to display ImGui::
    //ImGui::StyleColorsClassic();
    //ImGui::StyleColorsLight();
    edited::StyleMyOwnColor();


    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    ImFontConfig cfg;
    cfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_ForceAutoHint | ImGuiFreeTypeBuilderFlags_LightHinting | ImGuiFreeTypeBuilderFlags_LoadColor | ImGuiFreeTypeBuilderFlags_Bitmap;
    cfg.FontDataOwnedByAtlas = false;

    font::lexend_medium = io.Fonts->AddFontFromMemoryTTF(lexend_medium, sizeof(lexend_medium), 14.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    ImGui::MergeIconsWithLatestFont(14.f, false);
    font::lexend_regular = io.Fonts->AddFontFromMemoryTTF(lexend_regular, sizeof(lexend_regular), 14.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    ImGui::MergeIconsWithLatestFont(14.f, false);
    font::lexend_medium_x = io.Fonts->AddFontFromMemoryTTF(lexend_medium, sizeof(lexend_medium), 13.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    ImGui::MergeIconsWithLatestFont(13.f, false);
    font::font_awesome[0] = io.Fonts->AddFontFromMemoryTTF((void*)lexend_regular, sizeof(lexend_regular), 20.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    ImGui::MergeIconsWithLatestFont(20.f, false);
    font::font_awesome[1] = io.Fonts->AddFontFromMemoryTTF((void*)lexend_regular, sizeof(lexend_regular), 25.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    ImGui::MergeIconsWithLatestFont(25.f, false);
    font::font_awesome[2] = io.Fonts->AddFontFromMemoryTTF((void*)lexend_medium, sizeof(lexend_medium), 40.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());
    ImGui::MergeIconsWithLatestFont(40.f, false);
    font::icomoon = io.Fonts->AddFontFromMemoryTTF(icomoon, sizeof(icomoon), 25.f, &cfg, io.Fonts->GetGlyphRangesCyrillic());

    if (texture::background == nullptr) 
        D3DXCreateTextureFromFileInMemoryEx(g_pd3dDevice, &background, sizeof(background), 1920, 1080, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture::background);
    if (texture::logo_acc == nullptr)
        D3DXCreateTextureFromFileInMemoryEx(g_pd3dDevice, &logo_acc, sizeof(logo_acc), 32, 32, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture::logo_acc);
    if (texture::logo_bcc == nullptr)
        D3DXCreateTextureFromFileInMemoryEx(g_pd3dDevice, &logo_bcc, sizeof(logo_bcc), 32, 32, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture::logo_bcc);
    if (texture::logo_ccc == nullptr)
        D3DXCreateTextureFromFileInMemoryEx(g_pd3dDevice, &logo_ccc, sizeof(logo_ccc), 32, 32, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture::logo_ccc);
    if (texture::wordmark == nullptr)
        D3DXCreateTextureFromFileInMemoryEx(g_pd3dDevice, &wordmark, sizeof(wordmark), 672, 124, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture::wordmark);
    if (texture::logo == nullptr)
        D3DXCreateTextureFromFileInMemoryEx(g_pd3dDevice, &logo, sizeof(logo), 512, 512, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture::logo);
    if (texture::rfs == nullptr)
        D3DXCreateTextureFromFileInMemoryEx(g_pd3dDevice, &rfs, sizeof(rfs), 1500, 829, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture::rfs);
    if (texture::banner == nullptr)
        D3DXCreateTextureFromFileInMemoryEx(g_pd3dDevice, &banner, sizeof(banner), 851, 315, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture::banner);
    if (texture::Wordmark_Black == nullptr)
        D3DXCreateTextureFromFileInMemoryEx(g_pd3dDevice, &Wordmark_Black, sizeof(Wordmark_Black), 1024, 1024, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture::Wordmark_Black);
    if (texture::logo_voidA == nullptr)
        D3DXCreateTextureFromFileInMemoryEx(g_pd3dDevice, &logo_voidA, sizeof(logo_voidA), 32, 32, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture::logo_voidA);
    if (texture::logo_voidB == nullptr)
        D3DXCreateTextureFromFileInMemoryEx(g_pd3dDevice, &logo_voidB, sizeof(logo_voidB), 32, 32, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture::logo_voidB);
    if (texture::logo_voidC == nullptr)
        D3DXCreateTextureFromFileInMemoryEx(g_pd3dDevice, &logo_voidC, sizeof(logo_voidC), 32, 32, D3DX_DEFAULT, 0, D3DFMT_UNKNOWN, D3DPOOL_MANAGED, D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, &texture::logo_voidC);
    // Initialize notify

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    // Our state
    bool show_demo_window = true;
    bool show_another_window = true;

    edited::setup_circles();

    // Main loop
    bool done = false;    

    ImGui::FileBrowser fileDialog(ImGuiFileBrowserFlags_NoTitleBar | ImGuiFileBrowserFlags_CloseOnEsc);
    fileDialog.SetTitle("Add Files to Exclusion AutoUpdate");
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            g_d3dpp.BackBufferWidth = g_ResizeWidth;
            g_d3dpp.BackBufferHeight = g_ResizeHeight;
            g_ResizeWidth = g_ResizeHeight = 0;
            ResetDevice();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        {

            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(c::background::size));

            ImGuiStyle* style = &ImGui::GetStyle();

            style->WindowPadding = ImVec2(0, 0);
            style->ItemSpacing = ImVec2(0, 0);
            //style->WindowBorderSize = 0;
            style->ScrollbarSize = 7.f;
            style->ScrollbarRounding = 3.0f;
            style->FrameBorderSize = 1;
            style->FrameRounding = 3;
            style->FramePadding = ImVec2(15, 5);

            ImGui::Begin(LauncherName, nullptr, ImGuiWindowFlags_NoDecoration);
            {
                const ImVec2& pos = ImGui::GetWindowPos();
                const ImVec2& region = ImGui::GetContentRegionMax();
                const ImVec2& spacing = style->ItemSpacing;

                GetBackgroundDrawList()->AddImage(texture::background, ImVec2(0, 0), ImVec2(c::background::size), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255));

                static float tab_alpha = 0.f; 
                static float tab_add;

                tab_alpha = ImClamp(tab_alpha + (4.f * ImGui::GetIO().DeltaTime * (tabs == active_tab ? 1.f : -1.f)), 0.f, 1.f);

                /*if (tab_alpha == 0.f && tab_add == 0.f)
                {
                    timer++;
                    //CircleProgress("Hold on, verification is in progress ", timer, 300, 60);
                    //ProgressBar("Hold on, verification is in progress ", timer, 300, GetContentRegionMax().x, 25);

                    ImVec2 vecCenter = { ImGui::GetContentRegionMax() / 2 };
                    ImGui::SetCursorPosX(vecCenter.x - 50);
                    ImGui::SetCursorPosY(vecCenter.y - 50);
                    ImSpinner::SpinnerIngYang("SpinnerIngYangR", 50, 10, true, 0.1f, c::accent, c::text::text_active, 4 * velocity, IM_PI * 0.8f);
                    //nextdot -= 0.07f;
                    //ImSpinner::Spinner<ImSpinner::e_st_dots>("SpinnerDots", ImSpinner::Radius{ 50 }, ImSpinner::Thickness{ 7 }, ImSpinner::Color{ c::accent }, ImSpinner::FloatPtr{ &nextdot }, ImSpinner::Speed{ 1 * velocity }, ImSpinner::Dots{ 10 }, ImSpinner::MinThickness{ -1.f });
                    if (timer > 100) {
                        active_tab = tabs;
                        timer = 0;
                    }
                }*/

                active_tab = tabs;
                PushStyleVar(ImGuiStyleVar_Alpha, tab_alpha * style->Alpha);

                if ((!g_ConfigMgr.isLoggedIn || !g_Main.m_dwAccountSerial) && tabs == 99) {
                    ImVec2 vecCenter = { ImGui::GetContentRegionMax() / 2 };
                    ImGui::SetCursorPosX(vecCenter.x - 50);
                    ImGui::SetCursorPosY(vecCenter.y - 50);
                    ImSpinner::SpinnerIngYang("SpinnerIngYangR", 50, 10, true, 0.1f, c::accent, c::text::text_active, 4 * velocity, IM_PI * 0.8f);
                }
                
                if (active_tab == -2) {
                    ImVec2 vecCenter = { ImGui::GetContentRegionMax() / 2 };
                    ImGui::SetCursorPosX(vecCenter.x - 110);
                    ImGui::SetCursorPosY(vecCenter.y - 110);
                    ImSpinner::SpinnerSimpleArcFade("Getting Configuration Server",  110, 5, c::accent, 4* velocity);
                    //ImSpinner::SpinnerArcFade("SpinnerArcFade", 110, 5, c::accent, 3 * velocity, 4);
                    ImGui::GetForegroundDrawList()->AddText(vecCenter - ImGui::CalcTextSize("Getting Configuration Server") / 2, ImGui::GetColorU32(c::text::text_active), "Getting Configuration Server");
                }

                else if (active_tab == 0)
                {
                    GetWindowDrawList()->AddRectFilled(pos, pos + ImVec2(c::background::size.x, 50), GetColorU32(c::element::panel), c::element::rounding, ImDrawFlags_RoundCornersTop);
                    GetWindowDrawList()->AddRectFilled(pos + ImVec2(0, 50), pos + ImVec2(c::background::size.x / 3.5, c::background::size.y), GetColorU32(c::element::panel), c::element::rounding, ImDrawFlags_RoundCornersBottomLeft);
                    //GetWindowDrawList()->AddText(pos + (ImVec2(c::background::size.x + 50, 48) - CalcTextSize(LauncherName)) / 2, GetColorU32(c::accent), LauncherName);

                    //top left logo
                   /*SetCursorPos(ImVec2(150, 30));
                   ImGui::Image(texture::logo, ImVec2(50, 50));*/

                   //RF Solutions canto

                    SetCursorPos(ImVec2(c::background::size.x - 200, c::background::size.y - 33));

                    float wordmarkWidth = 672;
                    float wordmarkHeight = 124;

                    float wordmarkSizeX = wordmarkWidth * 1 / 4;
                    float wordmarkSizeY = wordmarkHeight * 1 / 4;

                    ImGui::Image(texture::wordmark, ImVec2(wordmarkSizeX, wordmarkSizeY));


                    SetCursorPos(ImVec2(0, 0));
                    ImVec2 containerSize(330, 120);

                    float originalWidth = 1024;
                    float originalHeight = 1024;
                    float aspectRatio = originalWidth / originalHeight;

                    // Reduzido para 40% da largura do container
                    float imageSizeX = containerSize.x * 0.4f;
                    float imageSizeY = imageSizeX / aspectRatio;

                    float posX = (containerSize.x - imageSizeX) / 2;
                    float posY = (containerSize.y - imageSizeY) / 2;

                    edited::BeginChild("ContainerLogo", containerSize, ImVec2(0, 0), false);
                    SetCursorPos(ImVec2(posX, posY));
                    //ImGui::Image(texture::Wordmark_Black, ImVec2(imageSizeX, imageSizeY));
                    edited::EndChild();

                    // SetCursorPos(ImVec2(335, -5));

                    SetCursorPos(ImVec2(0, 660));
                    edited::BeginChild("Container##0", ImVec2(c::background::size.x / 3.5, 30), ImVec2(18, 0), false);
                    {
                        // Calculate the width for each button
                        float buttonWidth = (GetContentRegionAvail().x - 30) / 3.0f; // Subtracting 30 for spacing between buttons

                        // Set the width for the first button
                        ImGui::SetNextItemWidth(buttonWidth);
                        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0)); // Set item spacing to zero
                        if (edited::Button(ICON_FA_DISCORD"  Discord", ImVec2(buttonWidth, 30))) {

                            std::string link = "https://discord.gg/";
                            ShellExecuteA(NULL, "open", link.c_str(), NULL, NULL, SW_SHOWNORMAL);
                        }
                        ImGui::PopStyleVar();

                        ImGui::SameLine();

                        // Set the width for the second button
                        ImGui::SetNextItemWidth(buttonWidth);
                        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0)); // Set item spacing to zero
                        if (edited::Button(ICON_FA_GLOBE " Website", ImVec2(buttonWidth, 30))) {
                            std::string link = "https://google.com/";
                            ShellExecuteA(NULL, "open", link.c_str(), NULL, NULL, SW_SHOWNORMAL);
                        }
                        ImGui::PopStyleVar();

                        ImGui::SameLine();

                        // Set the width for the third button
                        ImGui::SetNextItemWidth(buttonWidth);
                        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0)); // Set item spacing to zero
                        if (edited::Button(ICON_FA_LIST " GameCP", ImVec2(buttonWidth, 30))) {
                            std::string link = "https://google.com/";
                            ShellExecuteA(NULL, "open", link.c_str(), NULL, NULL, SW_SHOWNORMAL);
                        }
                        ImGui::PopStyleVar(); // Restore default item spacing
                    }
                    edited::EndChild();


                    //PushFont(font::font_awesome[2]);
                    //GetWindowDrawList()->AddText(pos + ImVec2((c::background::size.x / 3.5 - CalcTextSize(ICON_FA_ROCKET).x), 50) / 2, GetColorU32(c::accent), ICON_FA_ROCKET);
                    //PopFont();
                    //GetWindowDrawList()->AddText(pos + ImVec2((c::background::size.x / 3.5 - CalcTextSize(LauncherName).x), 140) / 2, GetColorU32(c::accent), LauncherName);
                    
                    SetCursorPos(ImVec2(0, 100)); //

                    edited::BeginChild("Container##1", ImVec2(c::background::size.x / 3.5, c::background::size.y - 147), ImVec2(18, 18), false);
                    {
                        if (showWaitLogin || !c_Patcher.allowLogin) {
                            ImVec2 vecCenter = { ImGui::GetContentRegionMax() / 2};
                            ImGui::SetCursorPosX(vecCenter.x - style->WindowPadding.x - 25);
                            //ImGui::SetCursorPosY(vecCenter.y);
                            ImSpinner::SpinnerIngYang("SpinnerIngYangR", 50, 10, true, 0.1f, c::accent, c::text::text_active, 4 * velocity, IM_PI * 0.8f);
                            if (errMsg[0] != '\0') {
                                edited::TextColorCenter(GetColorU32(c::red),errMsg);
                            }
                        }
                        else {
                            if (!showLoginPanel) {
                                if (edited::ButtonWithIcon(ICON_FA_USER_PLUS, "     Add Account", ImVec2(GetContentRegionMax().x - style->WindowPadding.x, 40), "Add Account / Login")) {
                                    showLoginPanel = true;
                                }
                                for (int xc = 0; xc < m_AccountInfoWidget.accounts.size(); xc++) {
                                    int xoxo = edited::AccountList(xc == accountNo, m_AccountInfoWidget.accounts[xc].m_szLoginID, ImVec2(GetContentRegionMax().x - style->WindowPadding.x, 40));
                                    if (xoxo) {
                                        accountNo = xc; // Update spoof_page accordingly
                                    }
                                    if (xoxo == 2) {
                                        errMsg[0] = '\0';
                                        //setTabs(99, true);
                                        showWaitLogin = true;
                                        if(SirinMode){
                                            Sirin_Logout();
                                            if (g_ConfigMgr.isLoggedIn) {
                                                g_ConfigMgr.Logout();
                                                GMMode = false;
                                            }
                                            m_AccountInfoWidget.OnLoginPressed(m_AccountInfoWidget.accounts[xc].m_szLoginID, m_AccountInfoWidget.accounts[xc].m_szPW);
                                        }else{
                                            strcpy(g_Main.m_szLoginID, m_AccountInfoWidget.accounts[xc].m_szLoginID);
                                            strcpy(g_Main.m_szPW, m_AccountInfoWidget.accounts[xc].m_szPW);
                                            g_Main.cmd_CryptKeyRequest();
                                        }
                                    }
                                }
                                if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape))) {
                                    accountNo = -1;
                                }
                            }
                            else {
                                char tmpErrMsg[256];

                                if (g_ConfigMgr.isLoggedIn || g_Main.m_dwAccountSerial != NULL) {
                                    bool showGMMode = false;
                                    if (SirinMode) {
                                        edited::TextCenter("Hello %s, Welcome Back !", g_ConfigMgr.GetUserName());
                                        if (g_ConfigMgr.GetCaptureStatus())
                                        {
                                            showGMMode = true;

                                            if (g_ConfigMgr.GetCaptureSerial())
                                            {
                                                edited::TextCenter("Logged in as serial: %d", g_ConfigMgr.GetCaptureSerial());
                                            }
                                            else
                                            {
                                                edited::TextCenter("Logged in as %s", g_ConfigMgr.GetCaptureName());
                                            }
                                        }
                                        edited::TextCenter("Cash balance : %d", g_ConfigMgr.GetCash());

                                        if (g_ConfigMgr.GetRemainPremium() > 0)
                                        {
                                            auto s = g_ConfigMgr.GetRemainPremium();
                                            auto d = s / 86400;
                                            s -= d * 86400;
                                            auto h = s / 3600;
                                            s -= h * 3600;
                                            auto m = s / 60;
                                            s -= m * 60;

                                            if (d > 0)
                                                edited::TextCenter("Premium ends in %d day(s)", d);
                                            else if (h > 0)
                                                edited::TextCenter("Premium ends in %d hour(s)", h);
                                            else if (m > 0)
                                                edited::TextCenter("Premium ends in %d minute(s)", m);
                                            else if (s > 0)
                                                edited::TextCenter("Premium ends in %d second(s)", s);
                                        }

                                        if (g_ConfigMgr.GetBillingType() == 1)
                                        {
                                            auto s = g_ConfigMgr.GetRemainBiling();
                                            auto d = s / 86400;
                                            s -= d * 86400;
                                            auto h = s / 3600;
                                            s -= h * 3600;
                                            auto m = s / 60;
                                            s -= m * 60;

                                            if (d > 0)
                                                edited::TextCenter("Subscription ends in %n day(d)", d);
                                            else if (h > 0)
                                                edited::TextCenter("Subscription ends in %d hour(s)", h);
                                            else if (m > 0)
                                                edited::TextCenter("Subscription ends in %d minute(s)", m);
                                            else if (s > 0)
                                                edited::TextCenter("Subscription ends in %d second(s)", s);
                                            else
                                                edited::TextCenter("Subscription expired!");
                                        }

                                        if (g_ConfigMgr.GetRemainBan() > 0)
                                        {
                                            auto s = g_ConfigMgr.GetRemainBan();
                                            auto d = s / 86400;
                                            s -= d * 86400;
                                            auto h = s / 3600;
                                            s -= h * 3600;
                                            auto m = s / 60;
                                            s -= m * 60;

                                            if (d > 0)
                                                edited::TextCenter("Ban ends in %d day(s)", d);
                                            else if (h > 0)
                                                edited::TextCenter("Ban ends in %d day(s)", h);
                                            else if (m > 0)
                                                edited::TextCenter("Ban ends in %d minute(s)", m);
                                            else if (s > 0)
                                                edited::TextCenter("Ban ends in %d second(s)", s);
                                        }
                                        if (showGMMode)
                                            edited::Checkbox("Preserve GM permissions", &GMMode);
                                    }
                                    else {
                                        edited::TextCenter("Hello %s, Welcome Back !", g_Main.m_szLoginID);
                                        saveAccounts(m_AccountInfoWidget.accounts, JSONACCLIST, enckey);
                                    }
                                    if (c_Patcher.PatchComplete) {
                                        if (edited::Button("START THE GAME", ImVec2(GetContentRegionMax().x - style->WindowPadding.x, 40))) {
                                            if (!c_Patcher.fileExists(".\\rf_online.bin")) {
                                                setErrMessage("Start Bin Error, Try Follow this :\n\n- Disable your Antivirus / Add to Antivirus Exclusion List\n\nRe-Open the Launcher", false, true);
                                            }
                                            else {
                                                if (SirinMode) {
                                                    Sirin_EnterWorld(true, GMMode);
                                                }
                                                else {
                                                    g_Main.cmd_SelectWorld(0);
                                                }
                                                setTabs(4, true);
                                            }
                                        }
                                    }

                                    if (findAccounts(g_ConfigMgr.GetUserName())) {
                                        if (edited::Button("Remove Saved Account", ImVec2(GetContentRegionMax().x - style->WindowPadding.x, 40))) {
                                            sprintf_s(tmpErrMsg, "Are You Sure Want To Remove Accounts %s", g_ConfigMgr.GetUserName());
                                            OpenPopup("RemoveAccounts");
                                        }
                                    }

                                    if (edited::Button("LOGOUT", ImVec2(GetContentRegionMax().x - style->WindowPadding.x, 40))) {
                                        if (SirinMode) {
                                            Sirin_Logout();
                                            g_ConfigMgr.Logout();
                                        }
                                        else {
                                            g_Main.cmd_LogOut();
                                        }
                                        GMMode = false;
                                        showLoginPanel = false;
                                        setTabs(0);
                                    }
                                }
                                else{
                                    static char login[13] = { "" };
                                    edited::InputTextEx("a", "Login", login, 16, ImVec2(GetContentRegionMax().x - style->WindowPadding.x, 40), NULL);

                                    static char password[13] = { "" };
                                    edited::InputTextEx("b", "Password", password, 16, ImVec2(GetContentRegionMax().x - style->WindowPadding.x, 40), ImGuiInputTextFlags_Password);
                                    static bool rememberAccounts = false;
                                    edited::Checkbox("Remember me", &rememberAccounts);

                                    if (edited::Button("LOGIN", ImVec2(GetContentRegionMax().x - style->WindowPadding.x, 40)) || IsKeyPressed(GetKeyIndex(ImGuiKey_Enter)) || IsKeyPressed(GetKeyIndex(ImGuiKey_KeypadEnter))) {
                                        if (strlen(login) > 0 && strlen(password) > 0)
                                        {
                                            errMsg[0] = '\0';
                                            //setTabs(99, true);
                                            showWaitLogin = true;
                                            if(SirinMode){
                                                if (g_ConfigMgr.isLoggedIn) {
                                                    Sirin_Logout();
                                                    g_ConfigMgr.Logout();
                                                    GMMode = false;
                                                }
                                                m_AccountInfoWidget.OnLoginPressed(login, password);
                                            }else{
                                                strcpy(g_Main.m_szLoginID, login);
                                                strcpy(g_Main.m_szPW, password);
                                                g_Main.cmd_CryptKeyRequest();
                                            }
                                            if (rememberAccounts) {
                                                addAccounts(login, password);
                                            }
                                            password[0] = '\0';
                                        }
                                        else
                                        {
                                            sprintf_s(tmpErrMsg, "Please Enter login and password!");
                                            PushBackNotify(3, tmpErrMsg);
                                            OpenPopup("ErrLogin");
                                        }
                                    }
                                    if (edited::Button("CANCEL", ImVec2(GetContentRegionMax().x - style->WindowPadding.x, 40))) {
                                        showLoginPanel = false;
                                    }
                                }

                                const ImVec2 label_size = CalcTextSize(tmpErrMsg, NULL, true);
                                ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                                ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                                if (ImGui::BeginPopupModal("ErrLogin", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
                                {
                                    edited::TextCenter(tmpErrMsg);
                                    edited::Separator_line();
                                    if (edited::Button("OK", ImVec2(GetContentRegionMax().x - style->WindowPadding.x, 40))) {
                                        ImGui::CloseCurrentPopup();
                                    }
                                    ImGui::SetItemDefaultFocus();
                                    ImGui::SameLine();
                                    ImGui::EndPopup();
                                }
                                if (ImGui::BeginPopupModal("RemoveAccounts", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
                                {
                                    edited::TextCenter(tmpErrMsg);
                                    edited::Separator_line();
                                    if (edited::Button("YES", ImVec2(GetContentRegionMax().x / 2 - style->WindowPadding.x, 40))) {
                                        if(removeAccounts(g_ConfigMgr.GetUserName()))
                                            saveAccounts(m_AccountInfoWidget.accounts, JSONACCLIST, enckey);
                                        ImGui::CloseCurrentPopup();
                                    }
                                    ImGui::SetItemDefaultFocus();
                                    ImGui::SameLine();
                                    if (edited::Button("NO", ImVec2(GetContentRegionMax().x / 2 - style->WindowPadding.x, 40))) {
                                        ImGui::CloseCurrentPopup();
                                    }
                                    ImGui::EndPopup();
                                }
                                //SetCursorPosX(GetCursorPosX() + (270 - CalcTextSize("Forgot your password? Restore").x) / 2);
                                //TextColored(ImColor(GetColorU32(c::text::text_hov)), "Forgot your password?");
                                //SameLine(0, 5);
                                //TextColored(ImColor(GetColorU32(c::accent)), "Restore?");
                            }
                        }
                    }
                    //ImVec2 mouse_delta = ImGui::GetIO().MouseDelta;
                    edited::ScrollWhenDragging(ImVec2(0.0f, -1.0f), 0);
                    edited::EndChild();
                    //SetCursorPosY(GetCursorPosY() - 50);
                    SetCursorPos(ImVec2(c::background::size.x / 3.5 + style->WindowPadding.x, 50));
                    edited::BeginChild("Container##2", ImVec2(c::background::size.x - (c::background::size.x / 3.5), c::background::size.y - 50), ImVec2(10, 10));
                    {
                        // Start Threading News
                        if (c_Patcher.Configured) {
                            if (!calledNews) {
                                c_News.newsURL = c_Patcher.newsURL;
                                c_News.TopPlayerURL = c_Patcher.TopPlayerURL;
                                c_News.playerAPI = c_Patcher.playerAPI;
                                calledNews = true;
                                c_News.RequestNews();
                            }
                            if (c_News.m_lNews) {
                                if (!calledRenderImgNews) {
                                    calledRenderImgNews = true;
                                    c_News.RequestTopPlayer();
                                    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)PrepareRenderImages, 0, 0, 0);
                                }
                            }
                            if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_F5)) && allowRefresh) {
                                calledRenderImgNews = false;
                                renderAllNews = false;
                                calledNews = false;
                                c_News.m_lNews = false;
                                c_News.ListNews.clear();
                                c_News.playersTopLevel.clear();
                                c_News.playersTopKill.clear();
                                c_News.playersTopCpt.clear();
                                allowRefresh = false;
                            }
                        }
                        static int budjang = 1;
                        BeginGroup();
                        {
                            if (edited::tabs_news(1 == budjang, "NEWS", ICON_FA_RSS, ImVec2(GetContentRegionMax().x / tabNewsSize - style->WindowPadding.x, 60)))
                                budjang = 1;
                            SameLine();
                            if (edited::tabs_news(2 == budjang, "UPDATES", ICON_FA_NEWSPAPER, ImVec2(GetContentRegionMax().x / tabNewsSize - style->WindowPadding.x, 60)))
                                budjang = 2;
                            SameLine();
                            if (edited::tabs_news(3 == budjang, "RANKING", ICON_FA_CROWN, ImVec2(GetContentRegionMax().x / tabNewsSize - style->WindowPadding.x, 60)))
                                budjang = 3;
                            SameLine();
                            if (edited::tabs_news(4 == budjang, "ACCOUNT", ICON_FA_USER_COG, ImVec2(GetContentRegionMax().x / tabNewsSize - style->WindowPadding.x, 60)))
                                budjang = 4;
                            SameLine();
                            if (edited::tabs_news(5 == budjang, "CONFIG", ICON_FA_USER_COG, ImVec2(GetContentRegionMax().x / tabNewsSize - style->WindowPadding.x, 60)))
                                budjang = 5;
                            SameLine();
#ifdef DEBUG
                            SameLine();
                            if (edited::tabs_news(6 == budjang, "DEBUG", ICON_FA_COGS, ImVec2(GetContentRegionMax().x / tabNewsSize - style->WindowPadding.x, 60)))
                                budjang = 6;
#endif
                        }
                        EndGroup();
                        int sCurPos = 80;
                        if (!c_Patcher.PatchComplete) {
                            sCurPos = 130;
                        }
                        edited::BeginChild("ContainerInside##1", ImVec2((GetContentRegionMax().x - style->WindowPadding.x), GetContentRegionMax().y - sCurPos), ImVec2(10, 10), true);
                        {
                            if (budjang == 1) {
                                if (!renderAllNews)
                                    edited::draw_wait_data();
                                else{
                                    c_News.DisplayNews(0);
                                }
                            }
                            if (budjang == 2) {
                                if (!renderAllNews)
                                    edited::draw_wait_data();
                                else{
                                    c_News.DisplayNews(1);
                                }
                            }
                            if (budjang == 3) {
                                if (!renderAllNews) {
                                    edited::draw_wait_data();
                                }else{
                                    static int stateOfRank = 0;
                                    float xPus = GetContentRegionMax().x / 3 - style->WindowPadding.x;
                                    if (edited::Button("Top Level", ImVec2(xPus, 40)))
                                        stateOfRank = 0;
                                    SameLine();
                                    if (edited::Button("Top Kill", ImVec2(xPus, 40)))
                                        stateOfRank = 1;
                                    SameLine();
                                    if (edited::Button("Top CPT", ImVec2(xPus, 40)))
                                        stateOfRank = 2;
                                    RankSystem(stateOfRank);
                                }
                            }
                            if (budjang == 4){
                                if(accountNo == -1 && (!g_ConfigMgr.isLoggedIn || !g_Main.m_dwAccountSerial))
                                    edited::draw_wait_data("Please Select Account First", c::red);
                                else{
                                    if (!m_AccountInfoWidget.accounts[accountNo].requested){
                                        m_AccountInfoWidget.accounts[accountNo].requested = true;
                                        std::string enchanted = c_News.playerAPI + "?serverID=" + std::to_string(selectedServer) + "&accID=" + toHex(c_Patcher.encrypt(m_AccountInfoWidget.accounts[accountNo].m_szLoginID, enckey));
                                        c_News.fetchNewsDataAsync(enchanted, 2, &m_AccountInfoWidget.accounts[accountNo]);
                                    }
                                    else {
                                        if (m_AccountInfoWidget.accounts[accountNo].LoadedData) {
                                            RenderCharInfo(&m_AccountInfoWidget.accounts[accountNo]);
                                        }
                                        else {
                                            edited::draw_wait_data("Getting Account Information");
                                        }
                                    }
                                }
                            }
                            if (budjang == 5)
                            {
                                edited::BeginChild("Container##1", ImVec2((c::background::size.x - 160) / 2.4, c::background::size.y - 350), ImVec2(14, 12.7), true);
                                {
                                    /* edited::comments("Information", "Graphic Cards", "When choosing a video card, it will always be an integrated adapter for laptops, use the laptop utility to work with a discrete adapter.");
                                     edited::Separator_line();*/
                                    Text("Graphic Cards");
                                    SameLine();
                                    //ImGui::GetContentRegionMax().x - (CalcTextSize(graphicCards[selectedGraph].c_str()).x + style->WindowPadding.x) + 10;
                                    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::GetStyle().ItemSpacing.x - CalcTextSize(graphicCards[selectedGraph].c_str()).x - GetFrameHeightWithSpacing() - style->WindowPadding.x); // Align the combo box to the right
                                    if (ImGui::BeginCombo("##GPU", selectedGraph >= 0 ? graphicCards[selectedGraph].c_str() : "Select", ImGuiComboFlags_WidthFitPreview))
                                    {
                                        for (int gc = 0; gc < graphicCards.size(); ++gc)
                                        {
                                            const bool isSelected = (selectedGraph == gc);
                                            if (ImGui::Selectable(graphicCards[gc].c_str(), isSelected))
                                            {
                                                selectedGraph = gc;

                                                // Extract resolution X and Y from the selected string
                                                //int resolutionX, resolutionY;
                                                //sscanf_s(graphicCards[i].c_str(), "%d x %d", &resolutionX, &resolutionY);
                                                char bero[255];
                                                sprintf_s(R3EngineConfig.RenderState.Adapter, "%s", graphicCards[gc].c_str());
                                                sprintf_s(bero, "GPU Set to %s !", graphicCards[gc].c_str());
                                                PushBackNotify(4, bero);
                                            }

                                            if (isSelected)
                                                ImGui::SetItemDefaultFocus();
                                        }

                                        ImGui::EndCombo();
                                    }
                                    edited::Separator_line();
                                    Text("Screen Resolution");
                                    SameLine();
                                    // Combo box for resolution selection
                                    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::GetStyle().ItemSpacing.x - CalcTextSize(resolutions[selectedRes].c_str()).x - GetFrameHeightWithSpacing() - style->WindowPadding.x); // Align the combo box to the right
                                    if (ImGui::BeginCombo("##Res", selectedRes >= 0 ? resolutions[selectedRes].c_str() : "Select", ImGuiComboFlags_WidthFitPreview))
                                    {
                                        for (int i = 0; i < resolutions.size(); ++i)
                                        {
                                            const bool isSelected = (selectedRes == i);
                                            if (ImGui::Selectable(resolutions[i].c_str(), isSelected))
                                            {
                                                selectedRes = i;
                                                // Extract resolution X and Y from the selected string
                                                char bero[255];
                                                sprintf_s(bero, "Resolution Set to %s from %d x %d !", resolutions[i].c_str(), R3EngineConfig.RenderState.ScreenXSize, R3EngineConfig.RenderState.ScreenYSize);
                                                sscanf_s(resolutions[i].c_str(), "%d x %d", &R3EngineConfig.RenderState.ScreenXSize, &R3EngineConfig.RenderState.ScreenYSize);
                                                PushBackNotify(4, bero);
                                            }

                                            if (isSelected)
                                                ImGui::SetItemDefaultFocus();
                                        }

                                        ImGui::EndCombo();
                                    }
                                    edited::Separator_line();
                                    ImGui::PushStyleColor(ImGuiCol_Border, c::element::stroke);
                                    ImGui::PushStyleColor(ImGuiCol_FrameBg, c::element::filling);

                                    Text("Texture Details");
                                    SameLine(0, 20);
                                    ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - CalcItemWidth()); // Align the combo box to the right
                                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3); // Align the combo box to the right
                                    ImGui::SliderInt("##TD", &R3EngineConfig.RenderState.TextureDetail, 0, 3, texture_cfg[R3EngineConfig.RenderState.TextureDetail], ImGuiSliderFlags_NoInput); // Use ImGuiSliderFlags_NoInput flag to disable CTRL+Click here.
                                    edited::Separator_line();

                                    Text("Dymamic Light");
                                    SameLine(0, 20);
                                    ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - CalcItemWidth()); // Align the combo box to the right
                                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3); // Align the combo box to the right
                                    ImGui::SliderInt("##DL", &R3EngineConfig.RenderState.DynamicLight, 0, 3, dynamiclight_cfg[R3EngineConfig.RenderState.DynamicLight], ImGuiSliderFlags_NoInput); // Use ImGuiSliderFlags_NoInput flag to disable CTRL+Click here.
                                    edited::Separator_line();

                                    Text("Glow Effect");
                                    SameLine(0, 20);
                                    ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - CalcItemWidth()); // Align the combo box to the right
                                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3); // Align the combo box to the right
                                    ImGui::SliderInt("##GE", &R3EngineConfig.RenderState.BBoShasi, 0, 2, gloweff_cfg[R3EngineConfig.RenderState.BBoShasi], ImGuiSliderFlags_NoInput); // Use ImGuiSliderFlags_NoInput flag to disable CTRL+Click here.
                                    edited::Separator_line();

                                    //Text("Shadow Details");
                                    //SameLine(0, 20);
                                    //ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - CalcItemWidth()); // Align the combo box to the right
                                    //ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3); // Align the combo box to the right
                                    //ImGui::SliderInt("##SD", &R3EngineConfig.RenderState.ShadowDetail, 0, 3, shadow_cfg[R3EngineConfig.RenderState.ShadowDetail], ImGuiSliderFlags_NoInput); // Use ImGuiSliderFlags_NoInput flag to disable CTRL+Click here.
                                    //edited::Separator_line();

                                    Text("Gamma");
                                    SameLine(0, 20);
                                    ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - CalcItemWidth()); // Align the combo box to the right
                                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3); // Align the combo box to the right
                                    ImGui::SliderFloat("##GM", &R3EngineConfig.RenderState.Gamma, 0.8f, 1.8f, "%.1f", ImGuiSliderFlags_NoInput);
                                    edited::Separator_line();
                                    Text("MSAA (Multisample Anti-Aliasing)");
                                    SameLine();
                                    int indexAA = std::log2(R3EngineConfig.RenderState.AntiAliasing);
                                    // Adjust the index according to your requirement
                                    if (indexAA > 4)
                                        indexAA = 4;
                                    else if (indexAA < 0)
                                        indexAA = 0;
                                    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::GetStyle().ItemSpacing.x - CalcTextSize(multiAA_cfg[R3EngineConfig.RenderState.AntiAliasing / 2]).x - GetFrameHeightWithSpacing() - style->WindowPadding.x); // Align the combo box to the right
                                    if (ImGui::BeginCombo("##MSAA", (R3EngineConfig.RenderState.AntiAliasing >= 0) ? multiAA_cfg[indexAA] : "Select", ImGuiComboFlags_WidthFitPreview))
                                    {
                                        for (int i = 0; i < 4; ++i)
                                        {
                                            const bool isSelected = (indexAA == i);
                                            if (ImGui::Selectable(multiAA_cfg[i], isSelected))
                                            {
                                                if (i == 0)
                                                    R3EngineConfig.RenderState.AntiAliasing = 0;
                                                else
                                                    R3EngineConfig.RenderState.AntiAliasing = static_cast<int>(std::pow(2, i));
                                            }

                                            if (isSelected)
                                                ImGui::SetItemDefaultFocus();
                                        }

                                        ImGui::EndCombo();
                                    }
                                    edited::Separator_line();
                                    ImGui::PopStyleColor(2);
                                    //ImGui::SetCursorPosY(GetCursorPosY() + 14);

                                    /*if (edited::Button("Restore to factory settings", ImVec2(GetContentRegionMax().x - style->WindowPadding.x, 30))) {

                                    }

                                    if (edited::Button("LOWEST", ImVec2((GetContentRegionMax().x - (style->WindowPadding.x + 20)) / 2, 30))) {

                                    }
                                    ImGui::SameLine(0, 20);
                                    if (edited::Button("BEST", ImVec2((GetContentRegionMax().x - (style->WindowPadding.x + 20)) / 2, 30))) {

                                    }
                                    */
                                }
                                edited::EndChild();

                                SameLine(0, 20);

                                edited::BeginChild("Container##2", ImVec2((c::background::size.x - 120) / 3, c::background::size.y - 450), ImVec2(13, 13), true);
                                {
                                    edited::Checkbox("Full Screen", &R3EngineConfig.RenderState.bFullScreen);
                                    edited::Separator_line();
                                    edited::Checkbox("HD Texture", &R3EngineConfig.RenderState.bDetailTexture);
                                    edited::Separator_line();
                                    edited::Checkbox("Mouse Acceleration", &R3EngineConfig.RenderState.bMouseAccelation);
                                    edited::Separator_line();
                                    edited::Checkbox("Sound Effect", &R3EngineConfig.Sound.Sound);
                                    edited::Separator_line();
                                    /*/if (R3EngineConfig.Sound.Sound) {
                                        Text("Sound Volume");
                                        SameLine(0, 20);
                                        ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - CalcItemWidth()); // Align the combo box to the right
                                        ImGui::SliderFloat("##SV", &R3EngineConfig.Sound.SoundVol, 0.0f, 1.0f, "%.1f", ImGuiSliderFlags_NoInput);
                                        edited::Separator_line();
                                    }*/

                                    edited::Checkbox("Background Music", &R3EngineConfig.Sound.Music);
                                    edited::Separator_line();




                                    /*if (R3EngineConfig.Sound.Music) {
                                        Text("Music Volume");
                                        SameLine(0, 20);
                                        ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - CalcItemWidth()); // Align the combo box to the right
                                        ImGui::SliderFloat("##MV", &R3EngineConfig.Sound.MusicVol, 0.0f, 1.0f, "%.1f", ImGuiSliderFlags_NoInput);
                                        edited::Separator_line();

                                        Text("Ambience Volume");
                                        SameLine(0, 20);
                                        ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - CalcItemWidth()); // Align the combo box to the right
                                        ImGui::SliderFloat("##AV", &R3EngineConfig.Sound.AmbVol, 0.0f, 1.0f, "%.1f", ImGuiSliderFlags_NoInput);

                                    }*/


                                    edited::EndChild();

                                    edited::BeginChild("Container##3", ImVec2((c::background::size.x - 160) / 2.4, c::background::size.y - 545), ImVec2(13, 9), true);
                                    {

                                        edited::info_bar("Launcher version:", "1.0.0.1");
                                        edited::Separator_line();

                                        float originalWidth = 672;
                                        float originalHeight = 124;



                                        // Calculate the image size to maintain its aspect ratio
                                        float wordmarkSizeX = wordmarkWidth * 0.3;
                                        float wordmarkSizeY = wordmarkHeight * 0.3;

                                        ImGui::Image(texture::wordmark, ImVec2(wordmarkSizeX, wordmarkSizeY));
                                        SameLine(313, 35);
                                        if (edited::Button("Contact", ImVec2(65, 30), "More info available in our discord server")) {
                                            // Coloque aqui o link para onde você deseja redirecionar
                                            std::string link = "https://discord.gg/9sCS5y2yy8";

                                            // Abrir o link no navegador padrão do sistema
                                            ShellExecuteA(NULL, "open", link.c_str(), NULL, NULL, SW_SHOWNORMAL);
                                        }
                                        edited::Separator_line();


                                        if (edited::Button("Save Configuration", ImVec2(GetContentRegionMax().x - style->WindowPadding.x, 40))) {
                                            if (SaveR3Config())
                                                PushBackNotify(4, "Game Configuration Saved!");
                                            else
                                                PushBackNotify(3, "Failed to Save Game Configuration !");
                                        }


                                    }
                                    edited::EndChild();

                                    SameLine(0, 20);
                                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 95);

                                    edited::BeginChild("Container##4", ImVec2((c::background::size.x - 120) / 3, c::background::size.y - 550), ImVec2(13, 12), true);


                                    Text("Server Select");
                                    ImGui::SameLine();
                                    if (c_Patcher.ServerListConfiguration.size() > 0) {
                                        if (selectedServer >= 0) {
                                            if (c_Patcher.ServerListConfiguration[selectedServer].ServerIcon == "logo_voidA") {
                                                ImGui::Image(texture::logo_voidA, ImVec2(32, 32));
                                                ImGui::SameLine();
                                            }
                                            else if (c_Patcher.ServerListConfiguration[selectedServer].ServerIcon == "logo_voidB") {
                                                ImGui::Image(texture::logo_voidB, ImVec2(32, 32));
                                                ImGui::SameLine();
                                            }
                                            else if (c_Patcher.ServerListConfiguration[selectedServer].ServerIcon == "logo_voidC") {
                                                ImGui::Image(texture::logo_voidC, ImVec2(32, 32));
                                                ImGui::SameLine();
                                            }
                                        }
                                        SameLine();
                                        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::GetStyle().ItemSpacing.x - CalcTextSize(c_Patcher.ServerListConfiguration[selectedServer].ServerName.c_str()).x - GetFrameHeightWithSpacing() - style->WindowPadding.x); // Align the combo box to the right
                                        if (ImGui::BeginCombo("##SLS", selectedServer >= 0 ? c_Patcher.ServerListConfiguration[selectedServer].ServerName.c_str() : "Select", ImGuiComboFlags_WidthFitPreview))
                                        {
                                            for (int sL = 0; sL < c_Patcher.ServerListConfiguration.size(); ++sL)
                                            {
                                                const bool isSelected = (selectedServer == sL);                                            
                                                // Verificar e exibir os ícones fenix_logo e ferir_logo
                                                if (c_Patcher.ServerListConfiguration[sL].ServerIcon == "logo_voidA" && texture::logo_voidA != nullptr) {
                                                    ImGui::Image(texture::logo_voidA, ImVec2(29, 29));
                                                    ImGui::SameLine();
                                                }
                                                else if (c_Patcher.ServerListConfiguration[sL].ServerIcon == "logo_voidB" && texture::logo_voidB != nullptr) {
                                                    ImGui::Image(texture::logo_voidB, ImVec2(29, 29));
                                                    ImGui::SameLine();
                                                }
                                                else if (c_Patcher.ServerListConfiguration[sL].ServerIcon == "logo_voidC" && texture::logo_voidC != nullptr) {
                                                    ImGui::Image(texture::logo_voidC, ImVec2(29, 29));
                                                    ImGui::SameLine();
                                                }
                                                if (ImGui::Selectable(c_Patcher.ServerListConfiguration[sL].ServerName.c_str(), isSelected))
                                                {
                                                    if (!c_Patcher.Updating) {
                                                        selectedServer = sL;
                                                        selectedGate = 0;
                                                        ApplyServerSettings();
                                                    }
                                                    else {
                                                        PushBackNotify(3, "You can't change server while updating..", 10000);
                                                    }
                                                }
                                                if (isSelected)
                                                    ImGui::SetItemDefaultFocus();
                                            }
                                            ImGui::EndCombo();
                                        }
                                        edited::Separator_line();
                                        if (c_Patcher.ServerListConfiguration[selectedServer].Gates.size() > 0) {
                                            Text("Select Gate");
                                            SameLine();
                                            char GName[255];
                                            sprintf_s(GName, "%s %d ms", c_Patcher.ServerListConfiguration[selectedServer].Gates[selectedGate].GateName.c_str(), c_Patcher.ServerListConfiguration[selectedServer].Gates[selectedGate].Ping);
                                            ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::GetStyle().ItemSpacing.x - CalcTextSize(GName).x - GetFrameHeightWithSpacing() - style->WindowPadding.x); // Align the combo box to the right
                                            if (ImGui::BeginCombo("##GLS", selectedGate >= 0 ? GName : "Select", ImGuiComboFlags_WidthFitPreview))
                                            {
                                                for (int gL = 0; gL < c_Patcher.ServerListConfiguration[selectedServer].Gates.size(); ++gL)
                                                {
                                                    const bool isSelected = (selectedGate == gL);
                                                    char tGName[255];
                                                    sprintf_s(tGName, "%s %d ms", c_Patcher.ServerListConfiguration[selectedServer].Gates[gL].GateName.c_str(), c_Patcher.ServerListConfiguration[selectedServer].Gates[gL].Ping);
                                                    if (ImGui::Selectable(tGName, isSelected))
                                                    {
                                                        selectedGate = gL;
                                                        ApplyServerSettings();
                                                    }
                                                    if (isSelected)
                                                        ImGui::SetItemDefaultFocus();
                                                }
                                                ImGui::EndCombo();
                                            }
                                            edited::Separator_line();
                                        }
                                    }
                                    else {
                                        edited::draw_wait_data("Getting Server Information");
                                    }

                                    //ImGui::SetCursorPosY(ImGui::GetContentRegionMax().y - GetFrameHeightWithSpacing());

                                    //Text("Game Language");
                                    //SameLine();
                                    //ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::GetStyle().ItemSpacing.x - CalcTextSize(language_cfg[selectedLanguage][1]).x - GetFrameHeightWithSpacing() - style->WindowPadding.x); // Align the combo box to the right
                                    //if (ImGui::BeginCombo("##LANG", (R3EngineConfig.Launcher.Language >= 0) ? language_cfg[selectedLanguage][1] : "Select", ImGuiComboFlags_WidthFitPreview))
                                    //{
                                    //    for (int i = 0; i < 11; ++i)
                                    //    {
                                    //        const bool isSelected = (selectedLanguage == i);
                                    //        if (ImGui::Selectable(language_cfg[i][1], isSelected))
                                    //        {
                                    //            selectedLanguage = i;
                                    //            R3EngineConfig.Launcher.Language = i;
                                    //            g_ConfigMgr.SetLanguage(language_cfg[selectedLanguage][0]);
                                    //            g_Main.SetLanguage(language_cfg[selectedLanguage][0]);
                                    //            char bero[255];
                                    //            sprintf_s(bero, "Language Set to %s", language_cfg[i][1]);
                                    //            PushBackNotify(4, bero);
                                    //        }

                                    //        if (isSelected)
                                    //            ImGui::SetItemDefaultFocus();
                                    //    }

                                    //    ImGui::EndCombo();
                                    //}

                                    edited::EndChild();

                                    ImGui::SetCursorPosX(ImGui::GetCursorPosY() - 72 );
                                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 93);
                                    edited::BeginChild("Container##5", ImVec2((c::background::size.x - 120) / 3, 80), ImVec2(13, 11), true);

                                    //edited::Separator_line();
                                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 18);
                                    Text("Download settings");                              
                                    SameLine(240, -20);
                                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 30);
                                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() -10);
                                    if (edited::page(2 == tabs, ICON_FA_DOWNLOAD, ImVec2(60, 45), "Download Settings")) tabs = 2;

                                    edited::EndChild();






                                    /*if (edited::Button("Confirm", ImVec2((GetContentRegionMax().x - (style->WindowPadding.x + 20)) / 2, 30))) {

                                    }
                                    ImGui::SameLine(0, 20);*/
                                    /*if (edited::Button("Save Configuration", ImVec2(GetContentRegionMax().x - style->WindowPadding.x, 40))) {
                                        if (SaveR3Config())
                                            PushBackNotify(4, "Game Configuration Saved!");
                                        else
                                            PushBackNotify(3, "Failed to Save Game Configuration !");
                                    }*/


                                }
                            }
#ifdef DEBUG
                            if (budjang == 6)
                            {
                                for (int cx = 0; cx < (sizeof(MenuList) / sizeof(MenuList[0])); cx++) {
                                    if (edited::Button(MenuList[cx], ImVec2(GetContentRegionMax().x - style->WindowPadding.x, 40))) {
                                        active_tab = cx;
                                        tabs = cx;
                                    }
                                }

                                if (edited::Button("Notif ImGUI", ImVec2(GetContentRegionMax().x - style->WindowPadding.x, 40)))
                                {
                                    ImGui::InsertNotification({ ImGuiToastType_Success, 3000, "Hello World! This is a success! %s",  "We can also format here:)" });
                                    ImGui::InsertNotification({ ImGuiToastType_Warning, 3000, "Hello World! This is a warning! %d", 0x1337 });
                                    ImGui::InsertNotification({ ImGuiToastType_Error, 3000, "Hello World! This is an error! 0x%X", 0xDEADBEEF });
                                    ImGui::InsertNotification({ ImGuiToastType_Info, 3000, "Hello World! This is an info!" });
                                    ImGui::InsertNotification({ ImGuiToastType_Info, 3000, "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation" });
                                    ImGuiToast toast(ImGuiToastType_Warning, 3000); // <-- content can also be passed here as above
                                    toast.set_title("Lorem ipsum dolor sit amet");
                                    ImGui::InsertNotification(toast);
                                }
                            }
#endif // DEBUG
                            edited::ScrollWhenDragging(ImVec2(0.0f, -1.0f), 0);
                        }
                        edited::EndChild();

                        if (!c_Patcher.PatchComplete) {
                            if (!c_Patcher.Patchchecked) {                                
                                if (!calledCheckUpdate) {
                                    calledCheckUpdate = true;
                                    c_Patcher.RequestUpdate();
                                }
                            }

                            /*if (edited::Button("Update", ImVec2(GetContentRegionMax().x - style->WindowPadding.x, 40)))
                            {
                                if (!c_Patcher.Patchchecked)
                                    c_Patcher.RequestUpdate();
                                else
                                    PushBackNotify(ImGuiToastType_Error, "Patch Failure, See the log or Contact Administrator", 1500);
                            }
                            */

                            if (c_Patcher.NeedUpdate) {
                                ImGui::SetCursorPosY(GetContentRegionMax().y - 45);
                                edited::ProgressDownload("Downloading Update", c_Patcher.curUpdate, c_Patcher.totalUpdate, c_Patcher.curUpdate, c_Patcher.totalUpdate, GetContentRegionMax().x / 2 - style->WindowPadding.x, 25, true, ImVec2((GetContentRegionMax().x / 2 - style->WindowPadding.x), 25));                                
                                ImGui::SetCursorPosX(GetContentRegionMax().x /2 + style->WindowPadding.x);
                                edited::ProgressDownload(c_Patcher.s_Progress, c_Patcher.curUpdate, c_Patcher.totalUpdate, c_Patcher.AUprogress, c_Patcher.AUdownSize, GetContentRegionMax().x / 2 - style->WindowPadding.x, 25, true, ImVec2((GetContentRegionMax().x / 2 - style->WindowPadding.x), 25));                                
                            }
                        }
                        /*
                        for (char cc = 'a'; cc <= 'z'; cc++) {
                            std::string str(1, cc); // Convert char to string
                            if (edited::IconButton(str.c_str(), ImVec2(50, 50), NULL));
                        }
                        */
                    }
                    edited::EndChild();
                }   
                else if (active_tab >= 1 && active_tab < 4)
                {
                    GetWindowDrawList()->AddRectFilled(pos, pos + ImVec2(c::background::size.x, 50), GetColorU32(c::element::panel), c::element::rounding, ImDrawFlags_RoundCornersTop);
                    GetWindowDrawList()->AddRectFilled(pos + ImVec2(0, 50), pos + ImVec2(100, c::background::size.y), GetColorU32(c::element::panel), c::element::rounding, ImDrawFlags_RoundCornersBottomLeft);

                    PushFont(font::font_awesome[0]);
                    GetWindowDrawList()->AddText(pos + (ImVec2(100, 50) - CalcTextSize(ICON_FA_TOOLS)) / 2, GetColorU32(c::accent), ICON_FA_TOOLS);
                    PopFont();

                    GetWindowDrawList()->AddText(pos + (ImVec2(c::background::size.x + 50, 48) - CalcTextSize(LauncherName)) / 2, GetColorU32(c::accent), LauncherName);

                    SetCursorPos(ImVec2((100 - 60) / 2, 70));

                    BeginGroup();
                    {
                       // if (edited::page(1 == tabs, ICON_FA_COGS, ImVec2(60, 60), "Game Settings")) tabs = 1;
                       // if (edited::page(2 == tabs, ICON_FA_DOWNLOAD, ImVec2(60, 60), "Download Settings")) tabs = 2;
                       // if (edited::page(3 == tabs, ICON_FA_GLOBE, ImVec2(60, 60), "Server Settings")) tabs = 3;
                        //if (edited::page(2 == tabs, ICON_FA_COMMENT_ALT, ImVec2(60, 60), "Comments")) tabs = 2;

                        ImGui::SetCursorPosY(ImGui::GetContentRegionMax().y - 80);
                        if (edited::page(0 == tabs, ICON_FA_DOOR_OPEN, ImVec2(60, 60), "Back to Main Menu")) tabs = 0;
                    }
                    EndGroup();

                    ImGui::SetCursorPos(ImVec2(120, 70));

                    if (active_tab == 1)
                    {
                        edited::BeginChild("Container##1", ImVec2((c::background::size.x - 160) / 2, c::background::size.y - 90), ImVec2(18, 18), true);
                        {
                            edited::comments("Information", "Graphic Cards", "When choosing a video card, it will always be an integrated adapter for laptops, use the laptop utility to work with a discrete adapter.");
                            edited::Separator_line();
                            Text("Graphic Cards");
                            SameLine();
                            //ImGui::GetContentRegionMax().x - (CalcTextSize(graphicCards[selectedGraph].c_str()).x + style->WindowPadding.x) + 10;
                            ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::GetStyle().ItemSpacing.x - CalcTextSize(graphicCards[selectedGraph].c_str()).x - GetFrameHeightWithSpacing() - style->WindowPadding.x); // Align the combo box to the right
                            if (ImGui::BeginCombo("##GPU", selectedGraph >= 0 ? graphicCards[selectedGraph].c_str() : "Select", ImGuiComboFlags_WidthFitPreview))
                            {
                                for (int gc = 0; gc < graphicCards.size(); ++gc)
                                {
                                    const bool isSelected = (selectedGraph == gc);
                                    if (ImGui::Selectable(graphicCards[gc].c_str(), isSelected))
                                    {
                                        selectedGraph = gc;

                                        // Extract resolution X and Y from the selected string
                                        //int resolutionX, resolutionY;
                                        //sscanf_s(graphicCards[i].c_str(), "%d x %d", &resolutionX, &resolutionY);
                                        char bero[255];
                                        sprintf_s(R3EngineConfig.RenderState.Adapter, "%s", graphicCards[gc].c_str());
                                        sprintf_s(bero, "GPU Set to %s !", graphicCards[gc].c_str());
                                        PushBackNotify(4, bero);
                                    }

                                    if (isSelected)
                                        ImGui::SetItemDefaultFocus();
                                }

                                ImGui::EndCombo();
                            }
                            edited::Separator_line();
                            Text("Screen Resolution");
                            SameLine();
                            // Combo box for resolution selection
                            ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::GetStyle().ItemSpacing.x - CalcTextSize(resolutions[selectedRes].c_str()).x - GetFrameHeightWithSpacing() - style->WindowPadding.x); // Align the combo box to the right
                            if (ImGui::BeginCombo("##Res", selectedRes >= 0 ? resolutions[selectedRes].c_str() : "Select", ImGuiComboFlags_WidthFitPreview))
                            {
                                for (int i = 0; i < resolutions.size(); ++i)
                                {
                                    const bool isSelected = (selectedRes == i);
                                    if (ImGui::Selectable(resolutions[i].c_str(), isSelected))
                                    {
                                        selectedRes = i;
                                        // Extract resolution X and Y from the selected string
                                        char bero[255];
                                        sprintf_s(bero, "Resolution Set to %s from %d x %d !", resolutions[i].c_str(), R3EngineConfig.RenderState.ScreenXSize, R3EngineConfig.RenderState.ScreenYSize);
                                        sscanf_s(resolutions[i].c_str(), "%d x %d", &R3EngineConfig.RenderState.ScreenXSize, &R3EngineConfig.RenderState.ScreenYSize);
                                        PushBackNotify(4, bero);
                                    }

                                    if (isSelected)
                                        ImGui::SetItemDefaultFocus();
                                }

                                ImGui::EndCombo();
                            }
                            edited::Separator_line();
                            ImGui::PushStyleColor(ImGuiCol_Border, c::element::stroke);
                            ImGui::PushStyleColor(ImGuiCol_FrameBg, c::element::filling);

                            Text("Texture Details");
                            SameLine(0, 20);
                            ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - CalcItemWidth()); // Align the combo box to the right
                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3); // Align the combo box to the right
                            ImGui::SliderInt("##TD", &R3EngineConfig.RenderState.TextureDetail, 0, 3, texture_cfg[R3EngineConfig.RenderState.TextureDetail], ImGuiSliderFlags_NoInput); // Use ImGuiSliderFlags_NoInput flag to disable CTRL+Click here.
                            edited::Separator_line();

                            Text("Dymamic Light");
                            SameLine(0, 20);
                            ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - CalcItemWidth()); // Align the combo box to the right
                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3); // Align the combo box to the right
                            ImGui::SliderInt("##DL", &R3EngineConfig.RenderState.DynamicLight, 0, 3, dynamiclight_cfg[R3EngineConfig.RenderState.DynamicLight], ImGuiSliderFlags_NoInput); // Use ImGuiSliderFlags_NoInput flag to disable CTRL+Click here.
                            edited::Separator_line();

                            Text("Glow Effect");
                            SameLine(0, 20);
                            ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - CalcItemWidth()); // Align the combo box to the right
                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3); // Align the combo box to the right
                            ImGui::SliderInt("##GE", &R3EngineConfig.RenderState.BBoShasi, 0, 2, gloweff_cfg[R3EngineConfig.RenderState.BBoShasi], ImGuiSliderFlags_NoInput); // Use ImGuiSliderFlags_NoInput flag to disable CTRL+Click here.
                            edited::Separator_line();

                            Text("Shadow Details");
                            SameLine(0, 20);
                            ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - CalcItemWidth()); // Align the combo box to the right
                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3); // Align the combo box to the right
                            ImGui::SliderInt("##SD", &R3EngineConfig.RenderState.ShadowDetail, 0, 3, shadow_cfg[R3EngineConfig.RenderState.ShadowDetail], ImGuiSliderFlags_NoInput); // Use ImGuiSliderFlags_NoInput flag to disable CTRL+Click here.
                            edited::Separator_line();

                            Text("Gamma");
                            SameLine(0, 20);
                            ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - CalcItemWidth()); // Align the combo box to the right
                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3); // Align the combo box to the right
                            ImGui::SliderFloat("##GM", &R3EngineConfig.RenderState.Gamma, 0.8f, 1.8f, "%.1f", ImGuiSliderFlags_NoInput);
                            edited::Separator_line();
                            //Text("Language");
                            //SameLine();
                            //ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::GetStyle().ItemSpacing.x - CalcTextSize(language_cfg[selectedLanguage][1]).x - GetFrameHeightWithSpacing() - style->WindowPadding.x); // Align the combo box to the right
                            //if (ImGui::BeginCombo("##LANG", (R3EngineConfig.Launcher.Language >= 0) ? language_cfg[selectedLanguage][1] : "Select", ImGuiComboFlags_WidthFitPreview))
                            //{
                            //    for (int i = 0; i < 11; ++i)
                            //    {
                            //        const bool isSelected = (selectedLanguage == i);
                            //        if (ImGui::Selectable(language_cfg[i][1], isSelected))
                            //        {
                            //            selectedLanguage = i;
                            //            R3EngineConfig.Launcher.Language = i;
                            //            g_ConfigMgr.SetLanguage(language_cfg[selectedLanguage][0]);
                            //            g_Main.SetLanguage(language_cfg[selectedLanguage][0]);
                            //            char bero[255];
                            //            sprintf_s(bero, "Language Set to %s", language_cfg[i][1]);
                            //            PushBackNotify(4, bero);
                            //        }

                            //        if (isSelected)
                            //            ImGui::SetItemDefaultFocus();
                            //    }

                            //    ImGui::EndCombo();
                            //}
                            edited::Separator_line();

                            if (SirinMode) {
                                Text("MSAA (Multisample Anti-Aliasing)");
                                SameLine();
                                int indexAA = std::log2(R3EngineConfig.RenderState.AntiAliasing);
                                // Adjust the index according to your requirement
                                if (indexAA > 4)
                                    indexAA = 4;
                                else if (indexAA < 0)
                                    indexAA = 0;
                                ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::GetStyle().ItemSpacing.x - CalcTextSize(multiAA_cfg[R3EngineConfig.RenderState.AntiAliasing / 2]).x - GetFrameHeightWithSpacing() - style->WindowPadding.x); // Align the combo box to the right
                                if (ImGui::BeginCombo("##MSAA", (R3EngineConfig.RenderState.AntiAliasing >= 0) ? multiAA_cfg[indexAA] : "Select", ImGuiComboFlags_WidthFitPreview))
                                {
                                    for (int i = 0; i < 4; ++i)
                                    {
                                        const bool isSelected = (indexAA == i);
                                        if (ImGui::Selectable(multiAA_cfg[i], isSelected))
                                        {
                                            if (i == 0)
                                                R3EngineConfig.RenderState.AntiAliasing = 0;
                                            else
                                                R3EngineConfig.RenderState.AntiAliasing = static_cast<int>(std::pow(2, i));
                                        }

                                        if (isSelected)
                                            ImGui::SetItemDefaultFocus();
                                    }

                                    ImGui::EndCombo();
                                }                            
                            }
                            ImGui::PopStyleColor(2);
                            //ImGui::SetCursorPosY(GetCursorPosY() + 14);
                            
                            /*if (edited::Button("Restore to factory settings", ImVec2(GetContentRegionMax().x - style->WindowPadding.x, 30))) {

                            }

                            if (edited::Button("LOWEST", ImVec2((GetContentRegionMax().x - (style->WindowPadding.x + 20)) / 2, 30))) {

                            }
                            ImGui::SameLine(0, 20);
                            if (edited::Button("BEST", ImVec2((GetContentRegionMax().x - (style->WindowPadding.x + 20)) / 2, 30))) {

                            }
                            */
                        }
                        edited::EndChild();

                        SameLine(0, 20);

                        edited::BeginChild("Container##2", ImVec2((c::background::size.x - 160) / 2, c::background::size.y - 90), ImVec2(18, 18), true);
                        {
                            edited::Checkbox("Full Screen", &R3EngineConfig.RenderState.bFullScreen);
                            edited::Separator_line();
                            edited::Checkbox("HD Texture", &R3EngineConfig.RenderState.bDetailTexture);
                            edited::Separator_line();
                            edited::Checkbox("Mouse Acceleration", &R3EngineConfig.RenderState.bMouseAccelation);
                            edited::Separator_line();
                            edited::Checkbox("Sound Effect", &R3EngineConfig.Sound.Sound);
                            edited::Separator_line();
                            if (R3EngineConfig.Sound.Sound) {
                                Text("Sound Volume");
                                SameLine(0, 20);
                                ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - CalcItemWidth()); // Align the combo box to the right
                                ImGui::SliderFloat("##SV", &R3EngineConfig.Sound.SoundVol, 0.0f, 1.0f, "%.1f", ImGuiSliderFlags_NoInput);
                                edited::Separator_line();
                            }

                            edited::Checkbox("Background Music", &R3EngineConfig.Sound.Music);
                            edited::Separator_line();

                            if (R3EngineConfig.Sound.Music) {
                                Text("Music Volume");
                                SameLine(0, 20);
                                ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - CalcItemWidth()); // Align the combo box to the right
                                ImGui::SliderFloat("##MV", &R3EngineConfig.Sound.MusicVol, 0.0f, 1.0f, "%.1f", ImGuiSliderFlags_NoInput);
                                edited::Separator_line();

                                Text("Ambience Volume");
                                SameLine(0, 20);
                                ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - CalcItemWidth()); // Align the combo box to the right
                                ImGui::SliderFloat("##AV", &R3EngineConfig.Sound.AmbVol, 0.0f, 1.0f, "%.1f", ImGuiSliderFlags_NoInput);
                                edited::Separator_line();
                            }

                            edited::info_bar("Launcher version:", "1.0.0.1");
                            edited::Separator_line();

                            ImGui::SetCursorPosY(ImGui::GetContentRegionMax().y - GetFrameHeightWithSpacing());

                            /*if (edited::Button("Confirm", ImVec2((GetContentRegionMax().x - (style->WindowPadding.x + 20)) / 2, 30))) {

                            }
                            ImGui::SameLine(0, 20);*/
                            if (edited::Button("Save Configuration", ImVec2(GetContentRegionMax().x - style->WindowPadding.x, 40))) {
                                if(SaveR3Config())
                                    PushBackNotify(4, "Game Configuration Saved!");
                                else
                                    PushBackNotify(3, "Failed to Save Game Configuration !");
                            }
                        }
                        edited::EndChild();
                    }
                    else if (active_tab == 2)
                    {
                        char tmpBf[256];
                        edited::BeginChild("Container##DLBL", ImVec2((c::background::size.x - 160) / 2, c::background::size.y - 90), ImVec2(18, 18), true);
                        {
                            edited::TextCenter("List Of Non Updated Files");
                            edited::Separator_line();
                            /*
                            edited::InputTextEx("e", "Submit your question", quest, 250, ImVec2(GetContentRegionMax().x - style->WindowPadding.x, 40), NULL);

                            static bool anonim = false;
                            edited::Checkbox("Send anonymously", &anonim);

                            if (edited::Button("Send", ImVec2(GetContentRegionMax().x - style->WindowPadding.x, 40))) {
                                if(strlen(quest) > 0)
                                    blacklistAutoUpdate.push_back(quest);
                            };*/
                            if (ImGui::BeginListBox("##ListBlackUpdate", ImVec2(-FLT_MIN, 14 * ImGui::GetTextLineHeightWithSpacing())))
                            {
                                for (int n = 0; n < c_Patcher.blacklistAutoUpdate.size(); n++)
                                {
                                    const bool is_selected = (blacklistcuridx == n);
                                    if (ImGui::Selectable(c_Patcher.blacklistAutoUpdate[n].c_str(), is_selected))
                                        blacklistcuridx = n;

                                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                                    {
                                        ImGui::OpenPopup("DeleteCFL");
                                    }

                                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                                    if (is_selected)
                                        ImGui::SetItemDefaultFocus();
                                }                            // Always center this window when appearing
                                ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                                ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

                                if (ImGui::BeginPopupModal("DeleteCFL", NULL, ImGuiWindowFlags_NoDecoration  | ImGuiWindowFlags_AlwaysAutoResize))
                                {
                                    char fName[125];
                                    sprintf_s(fName, "%s", c_Patcher.blacklistAutoUpdate[blacklistcuridx].c_str());
                                    //ImGui::Text("The Selected list will be removed list.\nAre you sure ?");
                                    edited::TextCenter("The Selected file will be removed list");
                                    edited::TextCenter("%s", fName);
                                    edited::TextCenter("Are You Sure ?");
                                    edited::Separator_line();
                                    if (edited::Button("OK", ImVec2(GetContentRegionMax().x / 2 - style->WindowPadding.x, 40))) {
                                        if (blacklistcuridx >= 0 && blacklistcuridx < c_Patcher.blacklistAutoUpdate.size()) {
                                            sprintf_s(tmpBf, "File %s has been removed From ignored List.", fName);
                                            c_Patcher.blacklistAutoUpdate.erase(c_Patcher.blacklistAutoUpdate.begin() + blacklistcuridx);
                                            PushBackNotify(1, tmpBf);
                                        }
                                        else {
                                            sprintf_s(tmpBf, "Failed to Remove %s From ignored List", c_Patcher.blacklistAutoUpdate[blacklistcuridx].c_str());
                                            PushBackNotify(3, tmpBf);
                                        }
                                        ImGui::CloseCurrentPopup();
                                    }
                                    ImGui::SetItemDefaultFocus();
                                    ImGui::SameLine();
                                    if (edited::Button("Cancel", ImVec2(GetContentRegionMax().x / 2 - style->WindowPadding.x, 40))) {
                                        ImGui::CloseCurrentPopup();
                                    }
                                    ImGui::EndPopup();
                                }

                                ImGui::EndListBox();
                            }
                            edited::Separator_line();
                            ImGui::SetCursorPosY(ImGui::GetContentRegionMax().y - GetFrameHeightWithSpacing());
                            if (edited::Button("File Browse", ImVec2(GetContentRegionMax().x - style->WindowPadding.x, 40))) {
                                // open file dialog when user clicks this button
                                fileDialog.SetPwd();
                                fileDialog.Open();
                            }

                            //if (ImGui::Button("Delete.."))
                                //ImGui::OpenPopup("Delete?");

                            //for (const auto& str : blacklistAutoUpdate) edited::AnotherText("AutoUpdate", "Disabled", str.c_str());
                        }
                        //edited::ScrollWhenDragging(ImVec2(0.0f, -1.0f), 0);
                        fileDialog.Display();

                        if (fileDialog.HasSelected())
                        {
                            std::string curSelFile = removeCurrentDirFromPath(fileDialog.GetSelected().string(), fileDialog.CurrentDir);
                            //PushBackNotify(1, tmpBf);
                            if (!c_Patcher.isExistorAllowedBlacklist(curSelFile)) {
                                sprintf_s(tmpBf, "File %s has been Added to Ignore List.", curSelFile.c_str());
                                c_Patcher.blacklistAutoUpdate.push_back(curSelFile);
                                PushBackNotify(1, tmpBf);
                            }
                            //std::cout << "Selected filename" << fileDialog.GetSelected().string() << std::endl;
                            curSelFile = '\0';
                            fileDialog.ClearSelected();
                        }

                        edited::EndChild();

                        SameLine(0, 20);

                        edited::BeginChild("Container##1", ImVec2((c::background::size.x - 160) / 2, c::background::size.y - 90), ImVec2(18, 18), true);
                        {
                            if (edited::Checkbox("Limit Download Speed", &c_Patcher.enableThrottle)) {
                                c_Patcher.setDownloadLimit();
                            }
                            edited::Separator_line();
                            if (c_Patcher.enableThrottle) {
                                Text("Limit Speed");
                                SameLine();
                                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 0,0 });
                                ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - CalcItemWidth() - ImGui::GetStyle().ItemSpacing.x - style->WindowPadding.x - style->FramePadding.x - 10); // Align the combo box to the right
                                static bool inputs_step = true;
                                ImGui::SetCursorPosY(ImGui::GetCursorPosY() - style->FramePadding.y); // Align the combo box to the right
                                //ImGui::InputInt("KB/s", &c_Patcher.throtleSpeedLimit);
                                if (ImGui::InputScalar("KB/s", ImGuiDataType_S64, &c_Patcher.throtleSpeedLimit, inputs_step ? &u64_kilobytes : NULL)) {
                                    c_Patcher.setDownloadLimit();
                                }
                                ImGui::PopStyleVar();
                                edited::Separator_line();
                                if (c_Patcher.throtleSpeedLimit <= 0)
                                    c_Patcher.throtleSpeedLimit = 1;
                            }
                            edited::comments("Notice", "Download Speed Limiter", "If you not save changes, the limit only applies to the current session. The next run will follow saved configuration.");
                            ImGui::SetCursorPosY(ImGui::GetContentRegionMax().y - GetFrameHeightWithSpacing());
                            if (edited::Button("Save Configuration", ImVec2(GetContentRegionMax().x - style->WindowPadding.x, 40))) {
                                if (SaveDownloadConfig()) {
                                    saveBlacklistList(JSONBLAU, c_Patcher.blacklistAutoUpdate);
                                    PushBackNotify(4, "Download Configuration Saved!");
                                }
                                else
                                    PushBackNotify(3, "Failed to Save Download Configuration !");
                            }
                        }
                        edited::EndChild();
                    }
                    else if (active_tab == 3) {
                        char tmpBf[256];
                        edited::BeginChild("Container##ServerSet", ImVec2((c::background::size.x - 140), c::background::size.y - 90), ImVec2(18, 18), true);
                        {
                            if (showServerList) {
                                Text("Server Select");
                                ImGui::SameLine();
                                if (c_Patcher.ServerListConfiguration.size() > 0) {
                                    if (selectedServer >= 0) {
                                        if (c_Patcher.ServerListConfiguration[selectedServer].ServerIcon == "logo_voidA") {
                                            ImGui::Image(texture::logo_voidA, ImVec2(29, 29));
                                            ImGui::SameLine();
                                        }
                                        else if (c_Patcher.ServerListConfiguration[selectedServer].ServerIcon == "logo_voidB") {
                                            ImGui::Image(texture::logo_voidB, ImVec2(29, 29));
                                            ImGui::SameLine();
                                        }
                                        else if (c_Patcher.ServerListConfiguration[selectedServer].ServerIcon == "logo_voidC") {
                                            ImGui::Image(texture::logo_voidC, ImVec2(29, 29));
                                            ImGui::SameLine();
                                        }
                                    }
                                    SameLine();
                                    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::GetStyle().ItemSpacing.x - CalcTextSize(c_Patcher.ServerListConfiguration[selectedServer].ServerName.c_str()).x - GetFrameHeightWithSpacing() - style->WindowPadding.x); // Align the combo box to the right
                                    if (ImGui::BeginCombo("##SLS", selectedServer >= 0 ? c_Patcher.ServerListConfiguration[selectedServer].ServerName.c_str() : "Select", ImGuiComboFlags_WidthFitPreview))
                                    {
                                        for (int sL = 0; sL < c_Patcher.ServerListConfiguration.size(); ++sL)
                                        {
                                            const bool isSelected = (selectedServer == sL);
                                            // Verificar e exibir os ícones fenix_logo e ferir_logo
                                            if (c_Patcher.ServerListConfiguration[sL].ServerIcon == "logo_voidA" && texture::logo_voidA != nullptr) {
                                                ImGui::Image(texture::logo_voidA, ImVec2(29, 29));
                                                ImGui::SameLine();
                                            }
                                            else if (c_Patcher.ServerListConfiguration[sL].ServerIcon == "logo_voidB" && texture::logo_voidB != nullptr) {
                                                ImGui::Image(texture::logo_voidB, ImVec2(29, 29));
                                                ImGui::SameLine();
                                            }
                                            else if (c_Patcher.ServerListConfiguration[sL].ServerIcon == "logo_voidC" && texture::logo_voidC != nullptr) {
                                                ImGui::Image(texture::logo_voidC, ImVec2(29, 29));
                                                ImGui::SameLine();
                                            }
                                            if (ImGui::Selectable(c_Patcher.ServerListConfiguration[sL].ServerName.c_str(), isSelected))
                                            {
                                                if (!c_Patcher.Updating) {
                                                    selectedServer = sL;
                                                    selectedGate = 0;
                                                    ApplyServerSettings();
                                                }
                                                else {
                                                    PushBackNotify(3, "You can't change server while updating..", 10000);
                                                }
                                            }
                                            if (isSelected)
                                                ImGui::SetItemDefaultFocus();
                                        }
                                        ImGui::EndCombo();
                                    }
                                    edited::Separator_line();
                                    if (c_Patcher.ServerListConfiguration[selectedServer].Gates.size() > 0) {
                                        Text("Select Gate");
                                        SameLine();
                                        char GName[255];
                                        sprintf_s(GName, "%s %d ms", c_Patcher.ServerListConfiguration[selectedServer].Gates[selectedGate].GateName.c_str(), c_Patcher.ServerListConfiguration[selectedServer].Gates[selectedGate].Ping);
                                        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::GetStyle().ItemSpacing.x - CalcTextSize(GName).x - GetFrameHeightWithSpacing() - style->WindowPadding.x); // Align the combo box to the right
                                        if (ImGui::BeginCombo("##GLS", selectedGate >= 0 ? GName : "Select", ImGuiComboFlags_WidthFitPreview))
                                        {
                                            for (int gL = 0; gL < c_Patcher.ServerListConfiguration[selectedServer].Gates.size(); ++gL)
                                            {
                                                const bool isSelected = (selectedGate == gL);
                                                char tGName[255];
                                                sprintf_s(tGName, "%s %d ms", c_Patcher.ServerListConfiguration[selectedServer].Gates[gL].GateName.c_str(), c_Patcher.ServerListConfiguration[selectedServer].Gates[gL].Ping);
                                                if (ImGui::Selectable(tGName, isSelected))
                                                {
                                                    selectedGate = gL;
                                                    ApplyServerSettings();
                                                }
                                                if (isSelected)
                                                    ImGui::SetItemDefaultFocus();
                                            }
                                            ImGui::EndCombo();
                                        }
                                        edited::Separator_line();
                                    }
                                }
                                else {
                                    SameLine();
                                    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - CalcTextSize(errMsg).x - style->WindowPadding.x); // Align the combo box to the right
                                    TextColored(c::red, errMsg);
                                }
                            }
                            else {
                                edited::draw_wait_data("Getting Server Information");
                            }
                        }
                        edited::EndChild();
                    }
                }
                else if (active_tab == 4)
                {
                    /*static float circle_size = 0.f;

                    circle_size = ImLerp(circle_size, active_tab == 3 ? 170.f : 0.f, ImGui::GetIO().DeltaTime * 3.f);

                    GetWindowDrawList()->AddCircle(pos + ImVec2(c::background::size) / 2, circle_size, GetColorU32(c::accent, 0.3), 100.f, 4.f);
                    GetWindowDrawList()->AddCircle(pos + ImVec2(c::background::size) / 2, circle_size * 2, GetColorU32(c::accent, 0.6), 100.f, 4.f);
                    GetWindowDrawList()->AddCircle(pos + ImVec2(c::background::size) / 2, circle_size * 3, GetColorU32(c::accent), 100.f, 4.f);
                    GetWindowDrawList()->AddCircle(pos + ImVec2(c::background::size) / 2, circle_size * 4, GetColorU32(c::accent), 100.f, 4.f);

                    if (circle_size > 169.f) circle_size = 0.f;
                    */

                    ImVec2 vecCenter = { ImGui::GetContentRegionMax() / 2 };
                    ImGui::SetCursorPosX(vecCenter.x - 500);
                    ImGui::SetCursorPosY(vecCenter.y - 500);
                    ImSpinner::SpinnerRotateSegmentsPulsar("SpinnerRotateSegmentsPulsar3", 500, 15, c::accent, 0.7f * velocity, 3, 3);
                    //ImSpinner::SpinnerSimpleArcFade("SpinnerSimpleArcFade",  13, 2, c::accent, 4* velocity);
                    GetWindowDrawList()->AddText(pos + (ImVec2(c::background::size) - CalcTextSize(LoadingGame) - ImVec2(0, 20)) / 2, GetColorU32(c::accent), LoadingGame);
                    GetWindowDrawList()->AddText(pos + (ImVec2(c::background::size) - CalcTextSize(LoadingGameText2) + ImVec2(0, 20)) / 2, GetColorU32(c::accent, 0.3), LoadingGameText2);
#ifdef DEBUG
                    ImGui::SetCursorPosY(ImGui::GetContentRegionMax().y + style->WindowPadding.y - 40);
                    if (edited::Button("Back", ImVec2((GetContentRegionMax().x), 40))) {
                        errMsg[0] = '\0';
                        setTabs(0, true);
                    };
#endif // DEBUG

                }
#ifdef DEBUG

                else if (active_tab == 6) {
                    edited::BeginChild("Container##0", ImVec2(c::background::size.x / 4, c::background::size.y - 100), ImVec2(18, 18), false);
                    {
                        edited::TextCenter("GOBLOK");
                    }
                    edited::EndChild();
                    ImVec2 vecCenter = { ImGui::GetContentRegionMax() / 2 };
                    ImGui::SetCursorPosX(vecCenter.x - 50);
                    ImGui::SetCursorPosY(vecCenter.y - 50);
                    ImSpinner::SpinnerIngYang("SpinnerIngYangR", 50, 10, true, 0.1f, c::accent, c::text::text_active, 4 * velocity, IM_PI * 0.8f);
                    //ImSpinner::SpinnerRotateSegmentsPulsar("SpinnerRotateSegmentsPulsar3",50, 10, c::accent, 1.1f* velocity, 3, 3);
                    //ImSpinner::SpinnerRotateSegmentsPulsar("SpinnerRotateSegmentsPulsar3", 500, 15, c::accent, 1.1f * velocity, 4, 2);
                    ImGui::SetCursorPosY(ImGui::GetContentRegionMax().y + style->WindowPadding.y - 40);
                    if (edited::Button("Back", ImVec2((GetContentRegionMax().x), 40))) {
                        errMsg[0] = '\0';
                        setTabs(0, true);
                    };
                }
#endif // DEBUG
                else if (active_tab == -1 || critError) // Error Information
                {
                    ImGui::SetCursorPos(ImVec2(0, 0));
                    static float alpha_text = 1.f;
                    if (LoginSuccess) {

                    }
                    else {
                        if (errMsg[0] == '\0') {
                            sprintf_s(errMsg, "No response from login server / Unknown Error");
                        }
                        static float circle_size = 0.f;
                        circle_size = ImLerp(circle_size, active_tab == -1 ? 170.f : 0.f, ImGui::GetIO().DeltaTime * 3.f);

                        GetWindowDrawList()->AddCircle(pos + ImVec2(c::background::size) / 2, circle_size, GetColorU32(c::red, 0.3), 100.f, 4.f);
                        GetWindowDrawList()->AddCircle(pos + ImVec2(c::background::size) / 2, circle_size * 2, GetColorU32(c::red, 0.6), 100.f, 4.f);
                        GetWindowDrawList()->AddCircle(pos + ImVec2(c::background::size) / 2, circle_size * 3, GetColorU32(c::red), 100.f, 4.f);
                        GetWindowDrawList()->AddCircle(pos + ImVec2(c::background::size) / 2, circle_size * 4, GetColorU32(c::red), 100.f, 4.f);

                        ImGui::PushFont(font::lexend_regular);
                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha_text);
                        ImGui::GetForegroundDrawList()->
                            AddText(ImVec2(
                                (ImGui::GetContentRegionMax().x - ImGui::CalcTextSize(errMsg).x) / 2,
                                (ImGui::GetContentRegionMax().y - ImGui::CalcTextSize(errMsg).y) / 2),
                                ImGui::GetColorU32(c::red), errMsg);
                        ImGui::PopStyleVar();
                        ImGui::PopFont();

                        if (circle_size > 169.f) circle_size = 0.f;
                        ImGui::SetCursorPosY(ImGui::GetContentRegionMax().y + style->WindowPadding.y - 40);
                        if (!critError) {
                            if (edited::Button("Back", ImVec2((GetContentRegionMax().x), 40))) {
                                errMsg[0] = '\0';
                                setTabs(0, true);
                                showWaitLogin = false;
                            };
                        }
                    }

                }

                PopStyleVar();
                if (tabs == 0) {
                    SetCursorPos(ImVec2(c::background::size.x - 80, 0));
                    if (edited::IconButtonFA(ICON_FA_WINDOW_MINIMIZE, ImVec2(50, 50), "Minimize", NULL)) {
                        PostMessage(hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
                    }

                  //  SetCursorPos(ImVec2(c::background::size.x - 80, 0));
                //    if (edited::IconButtonFA(ICON_FA_COG, ImVec2(50, 50), "Configuration", NULL)) {
                 //       tabs = 1;
              //      }
                }

                SetCursorPos(ImVec2(c::background::size.x - 50, 0));
                if (edited::IconButtonFA(ICON_FA_TIMES, ImVec2(50, 50), "Exit", NULL)) {
                    g_Main.ReleaseMainThread();
                    c_Patcher.ReleaseThread();
                    c_News.ReleaseThread();
                    ShowWindow(hwnd, SW_HIDE);
                    PostQuitMessage(0);
                }

                edited::draw_circles_and_lines(GetColorU32(c::accent, 0.2f));
                move_window();

            }

            ImGui::End();

            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, c::element::filling);
            ImGui::RenderNotifications();
            ImGui::PopStyleVar(2); // Don't forget to Pop()
            ImGui::PopStyleColor(1);

        }
        EndFrame();
        //Render();

        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color.x * clear_color.w * 255.0f), (int)(clear_color.y * clear_color.w * 255.0f), (int)(clear_color.z * clear_color.w * 255.0f), (int)(clear_color.w * 255.0f));
        //g_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
        g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_COLORVALUE(clear_color.x, clear_color.y, clear_color.z, clear_color.w), 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0)
        {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }
        HRESULT result = g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);

        // Handle loss of D3D9 device
        if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) {
            ResetDevice();
        }
    }

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions
bool CreateDeviceD3D(HWND hWnd)
{
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr)
        return false;

    // Create the D3DDevice
    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN; // Need to use an explicit format with alpha if needing per-pixel alpha composition.
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
    //g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED, &g_d3dpp, &g_pd3dDevice) < 0)
        return false;

    return true;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = nullptr; }
}

void ResetDevice()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (hr == D3DERR_INVALIDCALL)
        IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}