#include <gtk/gtk.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <regex>
#include <algorithm>

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
};

class AboutDialog {
private:
    GtkWidget* window;
    GtkWidget* vbox;
    GtkWidget* image;
    GtkWidget* title_label;
    GtkWidget* info_grid;
    GtkWidget* more_info_button;
    GtkWidget* keybinds_button;
    SystemInfo sysInfo;

    static void on_more_info_clicked(GtkWidget* widget, gpointer data) {
        AboutDialog* dialog = static_cast<AboutDialog*>(data);
        dialog->launchElySettings();
    }

    static void on_keybinds_clicked(GtkWidget* widget, gpointer data) {
        AboutDialog* dialog = static_cast<AboutDialog*>(data);
        dialog->launchKeybinds();
    }

    void launchElySettings() {
        system("elysettings &");
        gtk_widget_destroy(window);
        gtk_main_quit();
    }

    void launchKeybinds() {
        system("ElysiaOSKeybinds &");
        gtk_widget_destroy(window);
        gtk_main_quit();
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
            .info-label {
                font-family: ElysiaOSNew12;
                font-size: 11px;
                color: #6d6d70;
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
            .more-info-button {
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
            .info-grid {
                margin: 5px 20px;
                padding: 8px 0px;
            }
        )";

        std::string theme = sysInfo.getGtkTheme();
        if (theme == "ElysiaOS-HoC") {
            css += R"(
                .more-info-button {
                    background: linear-gradient(to right, #7077bd 0%, #b1c9ec 100%);
                }
                .more-info-button:hover {
                    background: linear-gradient(to right, #8791d1 0%, #c6dcf2 100%);
                }
                .more-info-button:active {
                    background: linear-gradient(to right, #5f66a9 0%, #9fb6d8 100%);
                }
            )";
        } else if (theme == "ElysiaOS") {
            css += R"(
                .more-info-button {
                    background: linear-gradient(to right, #e5a7c6 0%, #edcee3 100%);
                }
                .more-info-button:hover {
                    background: linear-gradient(to right, #edcee3 0%, #f5e5f0 100%);
                }
                .more-info-button:active {
                    background: linear-gradient(to right, #d495b8 0%, #e5a7c6 100%);
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
    AboutDialog() {
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window), "About This PC");
        gtk_window_set_default_size(GTK_WINDOW(window), 290, 495);
        gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
        setupStyles();

        vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_container_add(GTK_CONTAINER(window), vbox);

        image = gtk_image_new();
        GError* error = NULL;
        GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file_at_scale("~/Elysia/assets/assets/desktop.png", 64, 64, TRUE, &error);
        if (error != NULL) {
            pixbuf = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(), "computer", 64, GTK_ICON_LOOKUP_USE_BUILTIN, NULL);
            g_error_free(error);
        }
        if (pixbuf) {
            gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);
            g_object_unref(pixbuf);
        }
        gtk_style_context_add_class(gtk_widget_get_style_context(image), "system-image");
        gtk_box_pack_start(GTK_BOX(vbox), image, FALSE, FALSE, 0);

        title_label = gtk_label_new("About This Computer");
        gtk_style_context_add_class(gtk_widget_get_style_context(title_label), "title-label");
        gtk_box_pack_start(GTK_BOX(vbox), title_label, FALSE, FALSE, 5);

        info_grid = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(info_grid), 3);
        gtk_grid_set_column_spacing(GTK_GRID(info_grid), 8);
        gtk_widget_set_halign(info_grid, GTK_ALIGN_CENTER);
        gtk_style_context_add_class(gtk_widget_get_style_context(info_grid), "info-grid");
        gtk_box_pack_start(GTK_BOX(vbox), info_grid, TRUE, TRUE, 0);

        addInfoRow("CPU", sysInfo.getCPU(), 0);
        addInfoRow("GPU", sysInfo.getGPU(), 1);
        addInfoRow("Linux Version", sysInfo.getLinuxVersion(), 2);
        addInfoRow("PC Name", sysInfo.getPCName(), 3);

        // New Keybinds Button
        keybinds_button = gtk_button_new_with_label("Keybinds");
        gtk_style_context_add_class(gtk_widget_get_style_context(keybinds_button), "more-info-button");
        gtk_widget_set_halign(keybinds_button, GTK_ALIGN_CENTER);
        g_signal_connect(keybinds_button, "clicked", G_CALLBACK(on_keybinds_clicked), this);
        gtk_box_pack_start(GTK_BOX(vbox), keybinds_button, FALSE, FALSE, 0);

        // More Info Button (below keybinds)
        more_info_button = gtk_button_new_with_label("More Info");
        gtk_style_context_add_class(gtk_widget_get_style_context(more_info_button), "more-info-button");
        gtk_widget_set_halign(more_info_button, GTK_ALIGN_CENTER);
        g_signal_connect(more_info_button, "clicked", G_CALLBACK(on_more_info_clicked), this);
        gtk_box_pack_start(GTK_BOX(vbox), more_info_button, FALSE, FALSE, 0);

        // Footer
        GtkWidget* copyright =
            gtk_label_new("TM and © 2025 ElysiaOS.\nAll Rights Reserved.");
        gtk_style_context_add_class(gtk_widget_get_style_context(copyright), "copyright-label");
        gtk_label_set_justify(GTK_LABEL(copyright), GTK_JUSTIFY_CENTER);
        gtk_box_pack_end(GTK_BOX(vbox), copyright, FALSE, FALSE, 0);

        g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
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
        if (truncated_value.length() > 35)
            truncated_value = truncated_value.substr(0, 32) + "...";
        gtk_label_set_text(GTK_LABEL(value_widget), truncated_value.c_str());

        gtk_grid_attach(GTK_GRID(info_grid), label_widget, 0, row, 1, 1);
        gtk_grid_attach(GTK_GRID(info_grid), value_widget, 1, row, 1, 1);
    }

public:
    void show() {
        gtk_widget_show_all(window);
    }
};

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);
    AboutDialog dialog;
    dialog.show();
    gtk_main();
    return 0;
}
