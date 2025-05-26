#include <Windows.h>
#include <string>
#include <sstream>
#include <cstdint>
#include <tuple>
#include <optional>
#include <vector>

[[noreturn]] void exit(std::wstring const& message) {
    MessageBoxW(NULL, message.c_str(), L"autorestool error", MB_ICONERROR);
    std::exit(EXIT_FAILURE);
}

[[noreturn]] void exit_win32(wchar_t const* func) {
    auto const last_error = GetLastError();

    std::wstringstream message_builder { L"" };
    message_builder << func << ": ";

    wchar_t* message = nullptr;
    if (FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        last_error,
        0,
        reinterpret_cast<LPWSTR>(&message),
        0,
        NULL
    )) {
        message_builder << message;
        LocalFree(message);
    } else {
        message_builder << ": ???";
    }

    exit(message_builder.str());
}

inline void assert_win32(bool value, wchar_t const* func) {
    if (!value) {
        exit_win32(func);
    }
}

enum class Mode {
    INVALID,
    LIST,
    RUN
};

struct Config {
    Mode mode = Mode::INVALID;
    std::wstring adapter {};
    std::wstring program {};
    std::wstring dir {};
    std::uint32_t width = 0;
    std::uint32_t height = 0;

    [[nodiscard]] bool is_invalid() const {
        if (mode == Mode::INVALID) {
            return true;
        }

        if (mode == Mode::LIST) {
            return false;
        }
        
        return program.empty() || width == 0 || height == 0;
    }
};

std::optional<DEVMODEW> get_current_mode_opt(wchar_t const* adapter_name) {
    DEVMODEW device_mode = { .dmSize = sizeof(DEVMODEW) };

    if (EnumDisplaySettingsExW(adapter_name, ENUM_CURRENT_SETTINGS, &device_mode, 0)) {
        return device_mode;
    }

    return {};
}

std::optional<DEVMODEW> get_current_mode_opt(Config const& config) {
    return get_current_mode_opt(config.adapter.c_str());
}

DEVMODEW get_current_mode(wchar_t const* adapter_name) {
    auto const mode_opt = get_current_mode_opt(adapter_name);
    if (!mode_opt.has_value()) {
        std::wstringstream message { L"" };
        message << "Failed to get current mode for " << adapter_name << ", check if adapter name is correct";
        exit(message.str());
    }

    return mode_opt.value();
}

DEVMODEW get_current_mode(Config const& config) {
    return get_current_mode(config.adapter.c_str());
}

std::tuple<DWORD, DWORD> get_current_resolution(Config const& config) {
    auto const mode = get_current_mode(config);
    return { mode.dmPelsWidth, mode.dmPelsHeight };
}

void change_resolution(Config const& config, DWORD width, DWORD height) {
    DEVMODEW device_mode = get_current_mode(config);
    device_mode.dmPelsWidth = width;
    device_mode.dmPelsHeight = height;
    device_mode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

    LONG result = ChangeDisplaySettingsExW(config.adapter.c_str(), &device_mode, NULL, 0, NULL);
    assert_win32(result == DISP_CHANGE_SUCCESSFUL, L"ChangeDisplaySettingsExW");
}

void launch_and_wait(Config const& config) {
    STARTUPINFOW startup_info = {
        .cb = sizeof(STARTUPINFOW),
    };
    PROCESS_INFORMATION process_info = {};

    CreateProcessW(
        config.program.c_str(),
        nullptr,
        nullptr,
        nullptr,
        FALSE,
        DETACHED_PROCESS,
        nullptr,
        config.dir.empty() ? nullptr : config.dir.c_str(),
        &startup_info,
        &process_info
    );

    WaitForSingleObject(process_info.hProcess, INFINITE);
    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);
}

std::vector<DISPLAY_DEVICEW> get_display_adapters() {
    DISPLAY_DEVICEW display_device = { .cb = sizeof(DISPLAY_DEVICEW) };

    std::vector<DISPLAY_DEVICEW> adapters {};

    DWORD adapter_num = 0;
    while (EnumDisplayDevicesW(NULL, adapter_num, &display_device, 0)) {
        adapter_num++;
        adapters.push_back(display_device);
    }

    return adapters;
}

std::wstring find_primary_adapter() {
    for (auto const& adapter : get_display_adapters()) {
        if (adapter.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) {
            return adapter.DeviceName;
        }
    }

    exit(L"Failed to find primary adapter, manually specify one with --adapter");
}

void do_list(Config const& config) {

    std::wstringstream message { L"" };

    for (auto const& adapter : get_display_adapters()) {
        message
            << adapter.DeviceName
            << " - " << adapter.DeviceString;

        auto const mode = get_current_mode_opt(adapter.DeviceName);
        if (mode.has_value()) {
            message
                << " - " << mode.value().dmPelsWidth << "x" << mode.value().dmPelsHeight
                << " " << mode.value().dmDisplayFrequency << "hz";
        }

        message << std::endl;
    }

    MessageBoxW(NULL, message.str().c_str(), L"autorestool: list", MB_ICONINFORMATION);
}

void do_run(Config const& config) {
    auto const [original_width, original_height] = get_current_resolution(config);
    change_resolution(config, config.width, config.height);
    launch_and_wait(config);
    change_resolution(config, original_width, original_height);
}


Config parse_config() {
    Config config = {};

    int argc;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    assert_win32(argv != nullptr, L"CommandLineToArgvW");

    if (argc >= 2) {
        std::wstring const& mode = argv[1];
        if (mode == L"list") {
            config.mode = Mode::LIST;
        } else if (mode == L"run") {
            config.mode = Mode::RUN;
        }
    }

    for (int i = 2; i < argc; i += 2) {
        std::wstring const& name = argv[i];
        std::wstring const& value = argv[i + 1];

        if (name == L"--adapter") {
            config.adapter = value;
        } else if (name == L"--program") {
            config.program = value;
        } else if (name == L"--dir") {
            config.dir = value;
        } else if (name == L"--width") {
            config.width = std::stoi(value);
        } else if (name == L"--height") {
            config.height = std::stoi(value);
        }
    }

    if (config.is_invalid()) {
        exit(L"Invalid arguments, check README");
    }

    if (config.adapter.empty()) {
        config.adapter = find_primary_adapter();
    }

    return config;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    Config const config = parse_config();

    if (config.mode == Mode::LIST) {
        do_list(config);
    } else if (config.mode == Mode::RUN) {
        do_run(config);
    }

    return EXIT_SUCCESS;
}
