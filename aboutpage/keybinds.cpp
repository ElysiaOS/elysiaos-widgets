#include <gtk/gtk.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

struct Keybind {
    std::string shortcut;
    std::string description;
};

class SystemInfo {
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

    std::string getGtkTheme() {
        std::string theme = executeCommand("gsettings get org.gnome.desktop.interface gtk-theme");
        theme.erase(std::remove(theme.begin(), theme.end(), '\''), theme.end());
        theme.erase(theme.find_last_not_of(" \t\r\n") + 1);
        return theme;
    }
};

class KeybindsDialog {
private:
    GtkWidget* window;
    GtkWidget* vbox;
    GtkWidget* image;
    GtkWidget* title_label;
    GtkWidget* scrolled_window;
    GtkWidget* keybinds_grid;
    GtkWidget* back_button;
    SystemInfo sysInfo;
    std::vector<Keybind> keybinds;

    static void on_back_clicked(GtkWidget* widget, gpointer data) {
        KeybindsDialog* dialog = static_cast<KeybindsDialog*>(data);
        dialog->goBack();
    }

    void goBack() {
        system("about_system &");
        gtk_widget_destroy(window);
        gtk_main_quit();
    }

    void setupKeybinds() {
        keybinds = {
            {"SUPER + Q", "Close focused window"},
            {"SUPER + SPACE", "Launch Application manager"},
            {"SUPER + T", "Terminal"},
            {"ALT + TAB", "Brings up the workspaces switcher"},
            {"CTRL + SPACE", "Change Language"},
            {"SUPER + L", "Lock your screen Hyprlock"},
            {"SUPER + M", "Powermenu"},
            {"SUPER + [0-9]", "Switch workspaces"},
            {"SUPER + SHIFT + S", "Workspaces viewer Hyprspace"},
            {"SUPER + W", "Opens Swaync Notification"},
            {"SUPER + TAB", "EWW Widget for system info"},
            {"SUPER + SHIFT + W", "Launches Wallpapers menu"},
            {"SUPER + SHIFT + M", "Exit Hyprland altogether"},
            {"SUPER + V", "Toggle float a window"},
            {"SUPER + D", "Launch text editor VSCODE"},
            {"SUPER + E", "Launch File manager Thunar"},
            {"SUPER + O", "Launch Floorp Browser"},
            {"PRINTSC", "Take a full screenshot"},
            {"SUPER + S", "Take a region screenshot"},
            {"F1", "MUTE Volume"},
            {"F6", "Lower Brightness"},
            {"F7", "Higher Brightness"},
            {"Fn + F2", "Lower Volume"},
            {"Fn + F3", "Higher Volume"},
            {"Fn + F4", "MUTE Microphone"}
        };
    }

    void setupStyles() {
        GtkCssProvider* provider = gtk_css_provider_new();
        std::string css = R"(
            window {
                background: rgba(255, 255, 255, 0.3);
                border-radius: 12px;
                border: 1px solid #c0c0c0;
                box-shadow: 0 8px 25px rgba(0, 0, 0, 0.25), 0 0 1px rgba(0, 0, 0, 0.5);
            }
            .title-label {
                font-family: ElysiaOSNew12;
                font-size: 22px;
                font-weight: 600;
                color: #1d1d1f;
                margin: 8px 0px 12px 0px;
                text-shadow: 0 1px 0 rgba(255, 255, 255, 0.8);
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
                color: #6d6d70;
                margin: 2px 0px 2px 8px;
                font-weight: 400;
                text-shadow: 0 1px 0 rgba(255, 255, 255, 0.7);
            }
            .back-button {
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
                color: #8e8e93;
                margin: 12px 20px 16px 20px;
                text-shadow: 0 1px 0 rgba(255, 255, 255, 0.6);
                opacity: 0.8;
            }
            .keybinds-grid {
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
                color: #8e8e93;
                margin: 8px 20px 8px 20px;
                text-shadow: 0 1px 0 rgba(255, 255, 255, 0.6);
                opacity: 0.8;
                font-style: italic;
            }
        )";

        std::string theme = sysInfo.getGtkTheme();
        if (theme == "ElysiaOS-HoC") {
            css += R"(
                .back-button {
                    background: linear-gradient(to right, #7077bd 0%, #b1c9ec 100%);
                }
                .back-button:hover {
                    background: linear-gradient(to right, #8791d1 0%, #c6dcf2 100%);
                }
                .back-button:active {
                    background: linear-gradient(to right, #5f66a9 0%, #9fb6d8 100%);
                }
                .keybind-shortcut {
                    background: linear-gradient(to right, rgba(112, 119, 189, 0.2) 0%, rgba(177, 201, 236, 0.3) 100%);
                    border: 1px solid rgba(112, 119, 189, 0.4);
                    color: #5f66a9;
                }
            )";
        } else if (theme == "ElysiaOS") {
            css += R"(
                .back-button {
                    background: linear-gradient(to right, #e5a7c6 0%, #edcee3 100%);
                }
                .back-button:hover {
                    background: linear-gradient(to right, #edcee3 0%, #f5e5f0 100%);
                }
                .back-button:active {
                    background: linear-gradient(to right, #d495b8 0%, #e5a7c6 100%);
                }
                .keybind-shortcut {
                    background: linear-gradient(to right, rgba(229, 167, 198, 0.2) 0%, rgba(237, 206, 227, 0.3) 100%);
                    border: 1px solid rgba(229, 167, 198, 0.4);
                    color: #d495b8;
                }
            )";
        }

        gtk_css_provider_load_from_data(provider, css.c_str(), -1, NULL);
        gtk_style_context_add_provider_for_screen(
            gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
        g_object_unref(provider);
    }

public:
    KeybindsDialog() {
        setupKeybinds();
        
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window), "ElysiaOS Keybinds");
        gtk_window_set_default_size(GTK_WINDOW(window), 420, 580);
        gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
        setupStyles();

        vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_container_add(GTK_CONTAINER(window), vbox);

        // Icon/Image
        image = gtk_image_new();
        GError* error = NULL;
        GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file_at_scale("~/Elysia/assets/assets/keyboard.png", 64, 64, TRUE, &error);
        if (error != NULL) {
            pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), "preferences-desktop-keyboard", 64, GTK_ICON_LOOKUP_USE_BUILTIN, NULL);
            g_error_free(error);
        }
        if (pixbuf) {
            gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);
            g_object_unref(pixbuf);
        }
        gtk_style_context_add_class(gtk_widget_get_style_context(image), "system-image");
        gtk_box_pack_start(GTK_BOX(vbox), image, FALSE, FALSE, 0);

        // Title
        title_label = gtk_label_new("ElysiaOS Keybinds");
        gtk_style_context_add_class(gtk_widget_get_style_context(title_label), "title-label");
        gtk_box_pack_start(GTK_BOX(vbox), title_label, FALSE, FALSE, 5);

        // Scrolled window for keybinds
        scrolled_window = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), 
                                     GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
        gtk_widget_set_size_request(scrolled_window, -1, 350);
        gtk_style_context_add_class(gtk_widget_get_style_context(scrolled_window), "scrolled-window");

        // Grid for keybinds
        keybinds_grid = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(keybinds_grid), 4);
        gtk_grid_set_column_spacing(GTK_GRID(keybinds_grid), 12);
        gtk_widget_set_halign(keybinds_grid, GTK_ALIGN_CENTER);
        gtk_style_context_add_class(gtk_widget_get_style_context(keybinds_grid), "keybinds-grid");

        // Add keybinds to grid
        for (size_t i = 0; i < keybinds.size(); ++i) {
            addKeybindRow(keybinds[i].shortcut, keybinds[i].description, static_cast<int>(i));
        }

        gtk_container_add(GTK_CONTAINER(scrolled_window), keybinds_grid);
        gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

        // Tip label
        GtkWidget* tip_label = gtk_label_new("💡 Keep this open for quick reference!");
        gtk_style_context_add_class(gtk_widget_get_style_context(tip_label), "tip-label");
        gtk_label_set_justify(GTK_LABEL(tip_label), GTK_JUSTIFY_CENTER);
        gtk_box_pack_start(GTK_BOX(vbox), tip_label, FALSE, FALSE, 0);

        // Back Button
        back_button = gtk_button_new_with_label("← Close");
        gtk_style_context_add_class(gtk_widget_get_style_context(back_button), "back-button");
        gtk_widget_set_halign(back_button, GTK_ALIGN_CENTER);
        g_signal_connect(back_button, "clicked", G_CALLBACK(on_back_clicked), this);
        gtk_box_pack_start(GTK_BOX(vbox), back_button, FALSE, FALSE, 0);

        // Footer
        GtkWidget* copyright = gtk_label_new("TM and © 2025 ElysiaOS.\nAll Rights Reserved.");
        gtk_style_context_add_class(gtk_widget_get_style_context(copyright), "copyright-label");
        gtk_label_set_justify(GTK_LABEL(copyright), GTK_JUSTIFY_CENTER);
        gtk_box_pack_end(GTK_BOX(vbox), copyright, FALSE, FALSE, 0);

        g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    }

private:
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
    }
};

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);
    KeybindsDialog dialog;
    dialog.show();
    gtk_main();
    return 0;
}