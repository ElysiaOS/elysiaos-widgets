// about_system_async.cpp
#include <gtk/gtk.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <regex>
#include <thread>
#include "translations.h"

enum class ViewMode {
    ABOUT,
    KEYBINDS
};

struct Keybind {
    std::string shortcut;
    std::string descriptionKey;
};

class SystemInfo {
private:
    std::string readFile(const std::string& path) {
        std::ifstream file(path);
        std::string content;
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                content += line + "\n";
            }
            file.close();
        }
        return content;
    }

public:
    std::string executeCommand(const std::string& command) {
        std::string result;
        FILE* pipe = popen(command.c_str(), "r");
        if (pipe) {
            char buffer[128];
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                result += buffer;
            }
            pclose(pipe);
        }
        return result;
    }

    std::string getCPU() {
        std::string cpuinfo = readFile("/proc/cpuinfo");
        std::regex modelRegex("model name\\s*:\\s*(.*)");
        std::smatch match;
        if (std::regex_search(cpuinfo, match, modelRegex)) {
            std::string fullName = match[1].str();
            size_t atPos = fullName.find(" @");
            if (atPos != std::string::npos) {
                fullName = fullName.substr(0, atPos);
            }
            size_t withPos = fullName.find(" with ");
            if (withPos != std::string::npos) {
                fullName = fullName.substr(0, withPos);
            }
            return fullName;
        }
        return "Unknown CPU";
    }

    std::string getGPU() {
        std::string result = executeCommand("lspci | grep -i vga");
        if (!result.empty()) {
            size_t controllerPos = result.find("VGA compatible controller: ");
            if (controllerPos != std::string::npos) {
                result = result.substr(controllerPos + 27);
                result.erase(result.find_last_not_of(" \t\r\n") + 1);
                size_t colonPos = result.find(":");
                if (colonPos != std::string::npos) {
                    result = result.substr(0, colonPos);
                }
                size_t bracketStart = result.find("[");
                size_t bracketEnd = result.find("]");
                if (bracketStart != std::string::npos && bracketEnd != std::string::npos && bracketEnd > bracketStart) {
                    result = result.substr(bracketEnd + 1);
                    result.erase(0, result.find_first_not_of(" \t"));
                }
                return result;
            }
        }
        return "Unknown GPU";
    }

    std::string getLinuxVersion() {
        std::string kernel = executeCommand("uname -r");
        kernel.erase(kernel.find_last_not_of(" \t\r\n") + 1);
        if (!kernel.empty()) {
            return "Linux " + kernel;
        }
        return "Unknown Linux";
    }

    std::string getPCName() {
        std::string result = executeCommand("hostnamectl | grep 'Hardware Model' | cut -d':' -f2");
        if (!result.empty()) {
            size_t start = result.find_first_not_of(" \t");
            if (start != std::string::npos) {
                result = result.substr(start);
            }
            result.erase(result.find_last_not_of(" \t\r\n") + 1);
            if (!result.empty()) {
                return result;
            }
        }
        std::string dmiProduct = readFile("/sys/devices/virtual/dmi/id/product_name");
        if (!dmiProduct.empty()) {
            dmiProduct.erase(dmiProduct.find_last_not_of(" \t\r\n") + 1);
            if (dmiProduct != "To be filled by O.E.M." && !dmiProduct.empty()) {
                return dmiProduct;
            }
        }
        std::string hostname = executeCommand("hostname");
        hostname.erase(hostname.find_last_not_of(" \t\r\n") + 1);
        return hostname.empty() ? "Unknown PC" : hostname;
    }

    std::string getGtkTheme() {
        std::string theme = executeCommand("gsettings get org.gnome.desktop.interface gtk-theme");
        theme.erase(std::remove(theme.begin(), theme.end(), '\''), theme.end());
        theme.erase(theme.find_last_not_of(" \t\r\n") + 1);
        return theme;
    }

    std::string getOSName() {
        std::string osRelease = readFile("/etc/os-release");
        std::regex nameRegex("NAME=\"(.*)\"");
        std::smatch match;
        if (std::regex_search(osRelease, match, nameRegex)) {
            return match[1].str();
        }
        return "Unknown OS";
    }

    std::string getPackageCount() {
        std::string result = executeCommand("pacman -Qq | wc -l");
        result.erase(result.find_last_not_of(" \t\r\n") + 1);
        if (!result.empty()) {
            return result;
        }
        return "0";
    }
};

class SystemInfoDialog {
private:
    GtkWidget* window;
    GtkWidget* vbox;
    GtkWidget* image;
    GtkWidget* title_label;
    GtkWidget* info_grid;
    GtkWidget* keybinds_grid;
    GtkWidget* scrolled_window;
    GtkWidget* keybinds_button;
    GtkWidget* about_button;
    GtkWidget* more_info_button;
    GtkWidget* tip_label;
    SystemInfo sysInfo;
    Translations translations;
    std::vector<Keybind> keybinds;
    ViewMode currentView;

    // store value widgets so we can update them later from the background worker
    std::vector<GtkWidget*> info_value_widgets;

    // Helper struct for g_idle_add updates
    struct InfoUpdate {
        std::vector<std::string> values;
        SystemInfoDialog* dialog;
    };

    struct ThemeUpdate {
        std::string theme;
        SystemInfoDialog* dialog;
    };

    static gboolean applyInfoUpdate(gpointer data) {
        InfoUpdate* upd = static_cast<InfoUpdate*>(data);
        SystemInfoDialog* dlg = upd->dialog;
        // Expect values length == info_value_widgets size
        for (size_t i = 0; i < upd->values.size() && i < dlg->info_value_widgets.size(); ++i) {
            std::string v = upd->values[i];
            // truncate exactly like original addInfoRow
            if (v.length() > 35) v = v.substr(0, 32) + "...";
            gtk_label_set_text(GTK_LABEL(dlg->info_value_widgets[i]), v.c_str());
        }
        delete upd;
        return FALSE;
    }

    static gboolean applyThemeUpdate(gpointer data) {
        ThemeUpdate* t = static_cast<ThemeUpdate*>(data);
        SystemInfoDialog* dlg = t->dialog;
        std::string theme = t->theme;
        // Append theme-specific CSS as in original code
        GtkCssProvider* provider = gtk_css_provider_new();
        std::string css;
        if (theme == "ElysiaOS-HoC") {
            css += R"(
                .action-button {
                    background: linear-gradient(to right, #7077bd 0%, #b1c9ec 100%);
                }
                .action-button:hover {
                    background: linear-gradient(to right, #8791d1 0%, #c6dcf2 100%);
                }
                .action-button:active {
                    background: linear-gradient(to right, #5f66a9 0%, #9fb6d8 100%);
                }
                .keybind-shortcut {
                    background: linear-gradient(to right, rgba(112, 119, 189, 0.5) 0%, rgba(177, 201, 236, 0.7) 100%);
                    border: 1px solid rgba(112, 119, 189, 0.4);
                    color: black;
                }
            )";
        } else if (theme == "ElysiaOS") {
            css += R"(
                .action-button {
                    background: linear-gradient(to right, #e5a7c6 0%, #edcee3 100%);
                }
                .action-button:hover {
                    background: linear-gradient(to right, #edcee3 0%, #f5e5f0 100%);
                }
                .action-button:active {
                    background: linear-gradient(to right, #d495b8 0%, #e5a7c6 100%);
                }
                .keybind-shortcut {
                    background: linear-gradient(to right, rgba(229, 167, 198, 0.2) 0%, rgba(237, 206, 227, 0.3) 100%);
                    border: 1px solid rgba(229, 167, 198, 0.4);
                    color: #d495b8;
                }
            )";
        }
        if (!css.empty()) {
            gtk_css_provider_load_from_data(provider, css.c_str(), -1, NULL);
            gtk_style_context_add_provider_for_screen(
                gdk_screen_get_default(),
                GTK_STYLE_PROVIDER(provider),
                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
            );
        }
        g_object_unref(provider);
        delete t;
        return FALSE;
    }

    // start background thread to fetch CPU/GPU/OS/etc and schedule apply on main thread
    void fetchSystemInfoInBackground() {
        std::thread([this]() {
            InfoUpdate* upd = new InfoUpdate();
            upd->dialog = this;
            upd->values.reserve(6);
            std::string cpu = sysInfo.getCPU();
            if (cpu.empty()) cpu = translations.get("unknown_cpu");
            upd->values.push_back(cpu);

            std::string gpu = sysInfo.getGPU();
            if (gpu.empty()) gpu = translations.get("unknown_gpu");
            upd->values.push_back(gpu);

            std::string linux = sysInfo.getLinuxVersion();
            if (linux.empty()) linux = translations.get("unknown_linux");
            upd->values.push_back(linux);

            std::string pc = sysInfo.getPCName();
            if (pc.empty()) pc = translations.get("unknown_pc");
            upd->values.push_back(pc);

            std::string osn = sysInfo.getOSName();
            if (osn.empty()) osn = translations.get("unknown_os");
            upd->values.push_back(osn);

            std::string pkgs = sysInfo.getPackageCount();
            if (pkgs.empty()) pkgs = "0";
            upd->values.push_back(pkgs);

            // schedule UI update on main thread
            g_idle_add(applyInfoUpdate, upd);
        }).detach();
    }

    // start background thread to fetch theme and apply theme-specific CSS
    void fetchThemeAndApply() {
        std::thread([this]() {
            std::string theme = sysInfo.getGtkTheme();
            if (!theme.empty()) {
                ThemeUpdate* t = new ThemeUpdate();
                t->dialog = this;
                t->theme = theme;
                g_idle_add(applyThemeUpdate, t);
            }
        }).detach();
    }

    static void on_keybinds_clicked(GtkWidget* widget, gpointer data) {
        SystemInfoDialog* dialog = static_cast<SystemInfoDialog*>(data);
        dialog->switchToKeybinds();
    }

    static void on_about_clicked(GtkWidget* widget, gpointer data) {
        SystemInfoDialog* dialog = static_cast<SystemInfoDialog*>(data);
        dialog->switchToAbout();
    }

    static void on_more_info_clicked(GtkWidget* widget, gpointer data) {
        SystemInfoDialog* dialog = static_cast<SystemInfoDialog*>(data);
        dialog->launchElySettings();
    }

    void launchElySettings() {
        system("elysettings &");
        gtk_widget_destroy(window);
        gtk_main_quit();
    }

    void setupKeybinds() {
        keybinds = {
            {"SUPER + Q", "close_window"},
            {"SUPER + SPACE", "launch_app_manager"},
            {"SUPER + T", "terminal"},
            {"ALT + TAB", "workspace_switcher"},
            {"CTRL + SPACE", "change_language"},
            {"SUPER + L", "lock_screen"},
            {"SUPER + M", "powermenu"},
            {"SUPER + [0-9]", "switch_workspaces"},
            {"SUPER + SHIFT + S", "workspaces_viewer"},
            {"SUPER + W", "notification"},
            {"SUPER + TAB", "system_info_widget"},
            {"SUPER + SHIFT + W", "wallpapers_menu"},
            {"SUPER + SHIFT + M", "exit_hyprland"},
            {"SUPER + V", "toggle_float"},
            {"SUPER + D", "text_editor"},
            {"SUPER + E", "file_manager"},
            {"SUPER + O", "browser"},
            {"PRINTSC", "full_screenshot"},
            {"SUPER + S", "region_screenshot"},
            {"F1", "mute_volume"},
            {"F6", "lower_brightness"},
            {"F7", "higher_brightness"},
            {"Fn + F2", "lower_volume"},
            {"Fn + F3", "higher_volume"},
            {"Fn + F4", "mute_microphone"}
        };
    }

    void setupStyles() {
        // Apply the large base CSS immediately (same content as your original)
        GtkCssProvider* provider = gtk_css_provider_new();
        std::string css = R"(
            window {
                background: rgba(255, 255, 255, 0.3);
                border-radius: 21px;
                border: 1px solid #c0c0c0;
                box-shadow: 0 8px 25px rgba(0, 0, 0, 0.25), 0 0 1px rgba(0, 0, 0, 0.8);
            }
            .title-label {
                font-family: ElysiaOSNew12;
                font-size: 22px;
                font-weight: 600;
                color: #1d1d1f;
                margin: 8px 0px 12px 0px;
                text-shadow: 0 1px 0 rgba(255, 255, 255, 0.8);
            }
            .info-label {
                font-family: ElysiaOSNew12;
                font-size: 11px;
                color: #0f0f00;
                margin: 1px 8px 1px 0px;
                font-weight: 400;
                text-shadow: 0 1px 0 rgba(255, 255, 255, 0.7);
            }
            .info-value {
                font-family: ElysiaOSNew12;
                font-size: 11px;
                color: #1d1d1f;
                margin: 1px 0px 1px 8px;
                font-weight: 500;
                text-shadow: 0 1px 0 rgba(255, 255, 255, 0.7);
            }
            .keybind-shortcut {
                font-family: ElysiaOSNew12;
                font-size: 11px;
                color: #1d1d1f;
                margin: 2px 8px 2px 0px;
                font-weight: 600;
                text-shadow: 0 1px 0 rgba(255, 255, 255, 0.7);
                background: rgba(255, 255, 255, 0.4);
                border: 1px solid #d0d0d0;
                border-radius: 4px;
                padding: 4px 8px;
            }
            .keybind-description {
                font-family: ElysiaOSNew12;
                font-size: 11px;
                color: #0f0f00;
                margin: 2px 0px 2px 8px;
                font-weight: 400;
                text-shadow: 0 1px 0 rgba(255, 255, 255, 0.7);
            }
            .action-button {
                color: #1d1d1f;
                font-weight: bold;
                border: none;
                padding: 4px 10px;
                letter-spacing: 0.5px;
                border-radius: 8px;
                font-size: 12px;
                margin: 8px 0px 4px 0px;
                font-family: ElysiaOSNew12;
                min-width: 100px;
                min-height: 16px;
            }
            .system-image {
                margin: 22px 0px 18px 0px;
                opacity: 0.95;
            }
            .copyright-label {
                font-family: ElysiaOSNew12;
                font-size: 9px;
                color: #0f0f00;
                margin: 2px 20px 4px 20px;  /* Significantly reduced margins to move closer to buttons */
                text-shadow: 0 1px 0 rgba(255, 255, 255, 0.6);
                opacity: 0.8;
            }
            .info-grid, .keybinds-grid {
                margin: 5px 20px;
                padding: 8px 0px;
            }
            .scrolled-window {
                background: transparent;
                border: none;
            }
            .scrolled-window scrollbar {
                background: transparent;
            }
            .scrolled-window scrollbar slider {
                background: rgba(0, 0, 0, 0.3);
                border-radius: 6px;
                min-width: 8px;
            }
            .scrolled-window scrollbar slider:hover {
                background: rgba(0, 0, 0, 0.5);
            }
            .tip-label {
                font-family: ElysiaOSNew12;
                font-size: 10px;
                color: #0f0f00;
                margin: 8px 20px 8px 20px;
                text-shadow: 0 1px 0 rgba(255, 255, 255, 0.6);
                opacity: 0.8;
                font-style: italic;
            }
        )";
        gtk_css_provider_load_from_data(provider, css.c_str(), -1, NULL);
        gtk_style_context_add_provider_for_screen(
            gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
        g_object_unref(provider);

        // Now fetch theme in background and apply overrides later so setupStyles doesn't block
        fetchThemeAndApply();
    }

    void updateWindowForView() {
        if (currentView == ViewMode::ABOUT) {
            gtk_window_set_title(GTK_WINDOW(window), translations.get("about_title").c_str());
            gtk_window_resize(GTK_WINDOW(window), 270, 90);
            gtk_widget_set_size_request(window, 270, 90);  // Explicitly set size request
            gtk_label_set_text(GTK_LABEL(title_label), translations.get("about_computer").c_str());
            
            // Show about elements
            gtk_widget_show(info_grid);
            gtk_widget_show(more_info_button);
            gtk_widget_show(keybinds_button);
            
            // Hide keybinds elements
            gtk_widget_hide(scrolled_window);
            if (tip_label) gtk_widget_hide(tip_label);
            gtk_widget_hide(about_button);
            
            // Update button text
            gtk_button_set_label(GTK_BUTTON(keybinds_button), translations.get("keybinds_button").c_str());

        } else { // KEYBINDS
            gtk_window_set_title(GTK_WINDOW(window), translations.get("keybinds_title").c_str());
            gtk_window_resize(GTK_WINDOW(window), 290, 500);
            gtk_widget_set_size_request(window, 290, 500);  // Explicitly set size request
            gtk_label_set_text(GTK_LABEL(title_label), translations.get("elysia_keybinds").c_str());
            
            // Hide about elements
            gtk_widget_hide(info_grid);
            gtk_widget_hide(more_info_button);
            gtk_widget_hide(keybinds_button);
            
            // Show keybinds elements
            gtk_widget_show(scrolled_window);
            gtk_widget_show(about_button);
            if (tip_label) {
                gtk_widget_show(tip_label);
                gtk_label_set_text(GTK_LABEL(tip_label), translations.get("tip_message").c_str());
            }
            
            // Update button text
            gtk_button_set_label(GTK_BUTTON(about_button), translations.get("about_button").c_str());
        }
    }

public:
    SystemInfoDialog() : currentView(ViewMode::ABOUT) {
        setupKeybinds();
        
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window), translations.get("about_title").c_str());
        gtk_window_set_default_size(GTK_WINDOW(window), 270, 90);
        gtk_widget_set_size_request(window, 270, 90);  // Explicitly set size request
        gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
        setupStyles();

        vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_container_add(GTK_CONTAINER(window), vbox);

        // Icon/Image (kept identical, including your ~ path usage)
        image = gtk_image_new();
        GError* error = NULL;
        GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file_at_scale("~/Elysia/assets/assets/desktop.png", 64, 64, TRUE, &error);
        if (error != NULL) {
            pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), "distributor-logo-elysiaos", 64, GTK_ICON_LOOKUP_USE_BUILTIN, NULL);
            g_error_free(error);
        }
        if (pixbuf) {
            gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);
            g_object_unref(pixbuf);
        }
        gtk_style_context_add_class(gtk_widget_get_style_context(image), "system-image");
        gtk_box_pack_start(GTK_BOX(vbox), image, FALSE, TRUE, 0);

        // Title
        title_label = gtk_label_new(translations.get("about_computer").c_str());
        gtk_style_context_add_class(gtk_widget_get_style_context(title_label), "title-label");
        gtk_box_pack_start(GTK_BOX(vbox), title_label, FALSE, FALSE, 1);

        // About info grid
        info_grid = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(info_grid), 3);
        gtk_grid_set_column_spacing(GTK_GRID(info_grid), 8);
        gtk_widget_set_halign(info_grid, GTK_ALIGN_CENTER);
        gtk_style_context_add_class(gtk_widget_get_style_context(info_grid), "info-grid");
        gtk_box_pack_start(GTK_BOX(vbox), info_grid, FALSE, FALSE, 0);

        // IMPORTANT: don't call sysInfo.get* here (was blocking). Add placeholders and update later.
        addInfoRow(translations.get("cpu_label"), "...", 0);
        addInfoRow(translations.get("gpu_label"), "...", 1);
        addInfoRow(translations.get("linux_version_label"), "...", 2);
        addInfoRow(translations.get("pc_name_label"), "...", 3);
        addInfoRow(translations.get("os_label"), "...", 4);
        addInfoRow(translations.get("packages_label"), "...", 5);

        // Scrolled window for keybinds (initially hidden)
        scrolled_window = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), 
                                     GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
        gtk_widget_set_size_request(scrolled_window, -200, 260);
        gtk_style_context_add_class(gtk_widget_get_style_context(scrolled_window), "scrolled-window");

        // Grid for keybinds
        keybinds_grid = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(keybinds_grid), 4);
        gtk_grid_set_column_spacing(GTK_GRID(keybinds_grid), 12);
        gtk_widget_set_halign(keybinds_grid, GTK_ALIGN_CENTER);
        gtk_style_context_add_class(gtk_widget_get_style_context(keybinds_grid), "keybinds-grid");

        // Add keybinds to grid
        for (size_t i = 0; i < keybinds.size(); ++i) {
            addKeybindRow(keybinds[i].shortcut, translations.get(keybinds[i].descriptionKey), static_cast<int>(i));
        }

        gtk_container_add(GTK_CONTAINER(scrolled_window), keybinds_grid);
        gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

        // Tip label (initially hidden)
        tip_label = gtk_label_new(translations.get("tip_message").c_str());
        gtk_style_context_add_class(gtk_widget_get_style_context(tip_label), "tip-label");
        gtk_label_set_justify(GTK_LABEL(tip_label), GTK_JUSTIFY_CENTER);
        gtk_box_pack_start(GTK_BOX(vbox), tip_label, FALSE, FALSE, 0);

        // Create a button box to keep buttons and copyright together
        GtkWidget* button_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_box_pack_start(GTK_BOX(vbox), button_box, FALSE, FALSE, 0);

        // Keybinds Button
        keybinds_button = gtk_button_new_with_label(translations.get("keybinds_button").c_str());
        gtk_style_context_add_class(gtk_widget_get_style_context(keybinds_button), "action-button");
        gtk_widget_set_halign(keybinds_button, GTK_ALIGN_CENTER);
        g_signal_connect(keybinds_button, "clicked", G_CALLBACK(on_keybinds_clicked), this);
        gtk_box_pack_start(GTK_BOX(button_box), keybinds_button, FALSE, FALSE, 0);

        // About Button (initially hidden)
        about_button = gtk_button_new_with_label(translations.get("about_button").c_str());
        gtk_style_context_add_class(gtk_widget_get_style_context(about_button), "action-button");
        gtk_widget_set_halign(about_button, GTK_ALIGN_CENTER);
        g_signal_connect(about_button, "clicked", G_CALLBACK(on_about_clicked), this);
        gtk_box_pack_start(GTK_BOX(button_box), about_button, FALSE, FALSE, 0);

        // More Info Button
        more_info_button = gtk_button_new_with_label(translations.get("more_info_button").c_str());
        gtk_style_context_add_class(gtk_widget_get_style_context(more_info_button), "action-button");
        gtk_widget_set_halign(more_info_button, GTK_ALIGN_CENTER);
        g_signal_connect(more_info_button, "clicked", G_CALLBACK(on_more_info_clicked), this);
        gtk_box_pack_start(GTK_BOX(button_box), more_info_button, FALSE, FALSE, 0);

        // Footer
        GtkWidget* copyright = gtk_label_new(translations.get("copyright").c_str());
        gtk_style_context_add_class(gtk_widget_get_style_context(copyright), "copyright-label");
        gtk_label_set_justify(GTK_LABEL(copyright), GTK_JUSTIFY_CENTER);
        gtk_box_pack_start(GTK_BOX(button_box), copyright, FALSE, FALSE, 0);

        g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
        
        // Set initial view
        updateWindowForView();

        // Start the background fetch only after window is constructed.
        // The actual call to start the fetch is done from show() to ensure UI is visible first.
    }

    void switchToKeybinds() {
        currentView = ViewMode::KEYBINDS;
        
        // Temporarily make window larger
        gtk_window_resize(GTK_WINDOW(window), 290, 500);
        gtk_widget_set_size_request(window, 290, 500);  // Explicitly set size request
        
        // Update image for keybinds view
        GError* error = NULL;
        GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file_at_scale("~/Elysia/assets/assets/keyboard.png", 64, 64, TRUE, &error);
        if (error != NULL) {
            pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), "distributor-logo-elysiaos", 64, GTK_ICON_LOOKUP_USE_BUILTIN, NULL);
            g_error_free(error);
        }
        if (pixbuf) {
            gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);
            g_object_unref(pixbuf);
        }
        
        updateWindowForView();
    }

    void switchToAbout() {
        currentView = ViewMode::ABOUT;
        
        // Temporarily make window larger when switching to about
        gtk_window_resize(GTK_WINDOW(window), 290, 110);
        gtk_widget_set_size_request(window, 290, 110);  // Explicitly set size request
        
        // Update image for about view
        GError* error = NULL;
        GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file_at_scale("~/Elysia/assets/assets/desktop12.png", 64, 64, TRUE, &error);
        if (error != NULL) {
            pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), "distributor-logo-elysiaos", 64, GTK_ICON_LOOKUP_USE_BUILTIN, NULL);
            g_error_free(error);
        }
        if (pixbuf) {
            gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);
            g_object_unref(pixbuf);
        }
        
        updateWindowForView();
    }

private:
    void addInfoRow(const std::string& label, const std::string& value, int row) {
        GtkWidget* label_widget = gtk_label_new(label.c_str());
        GtkWidget* value_widget = gtk_label_new(value.c_str());

        gtk_style_context_add_class(gtk_widget_get_style_context(label_widget), "info-label");
        gtk_style_context_add_class(gtk_widget_get_style_context(value_widget), "info-value");

        gtk_widget_set_halign(label_widget, GTK_ALIGN_END);
        gtk_widget_set_halign(value_widget, GTK_ALIGN_START);

        std::string truncated_value = value;
        if (truncated_value.length() > 35) {
            truncated_value = truncated_value.substr(0, 32) + "...";
        }
        gtk_label_set_text(GTK_LABEL(value_widget), truncated_value.c_str());

        gtk_grid_attach(GTK_GRID(info_grid), label_widget, 0, row, 1, 1);
        gtk_grid_attach(GTK_GRID(info_grid), value_widget, 1, row, 1, 1);

        // store the pointer so we can update it later from the background thread
        info_value_widgets.push_back(value_widget);
    }

    void addKeybindRow(const std::string& shortcut, const std::string& description, int row) {
        GtkWidget* shortcut_widget = gtk_label_new(shortcut.c_str());
        GtkWidget* description_widget = gtk_label_new(description.c_str());

        gtk_style_context_add_class(gtk_widget_get_style_context(shortcut_widget), "keybind-shortcut");
        gtk_style_context_add_class(gtk_widget_get_style_context(description_widget), "keybind-description");

        gtk_widget_set_halign(shortcut_widget, GTK_ALIGN_END);
        gtk_widget_set_halign(description_widget, GTK_ALIGN_START);

        // Truncate long descriptions
        std::string truncated_description = description;
        if (truncated_description.length() > 45) {
            truncated_description = truncated_description.substr(0, 42) + "...";
        }
        gtk_label_set_text(GTK_LABEL(description_widget), truncated_description.c_str());

        gtk_grid_attach(GTK_GRID(keybinds_grid), shortcut_widget, 0, row, 1, 1);
        gtk_grid_attach(GTK_GRID(keybinds_grid), description_widget, 1, row, 1, 1);
    }

public:
    void show() {
        gtk_widget_show_all(window);
        
        // Initially hide keybinds elements
        if (currentView == ViewMode::ABOUT) {
            gtk_widget_hide(scrolled_window);
            gtk_widget_hide(tip_label);
            gtk_widget_hide(about_button);
        } else {
            gtk_widget_hide(info_grid);
            gtk_widget_hide(more_info_button);
            gtk_widget_hide(keybinds_button);
        }

        // Now that the UI is visible, start fetching system info in background
        fetchSystemInfoInBackground();
    }
};

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);
    SystemInfoDialog dialog;
    dialog.show();
    gtk_main();
    return 0;
}
