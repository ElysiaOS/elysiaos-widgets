#include <gtk/gtk.h>
#include <gtk-layer-shell.h>
#include <cairo.h>
#include <cmath>
#include <string>
#include <memory>
#include <thread>
#include <chrono>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <regex>
#include <algorithm>
#include <cctype>
#include <vector>
#include <array>
#include <unordered_map>
#include <sys/stat.h>
#include <ctime>

// Forward declarations
class CalendarWidget;
class MusicWidget;

// Global theme detection (shared)
static bool g_use_hoc_theme = false;
static bool g_theme_detected = false;

// Theme detection helper
void detect_global_theme() {
    if (g_theme_detected) return;
    
    FILE* pipe = popen("gsettings get org.gnome.desktop.interface gtk-theme 2>/dev/null", "r");
    if (pipe) {
        char buffer[256];
        std::string result;
        if (fgets(buffer, sizeof(buffer), pipe)) {
            result = buffer;
        }
        pclose(pipe);
        
        // Remove quotes and whitespace
        while (!result.empty() && (result.back() == '\n' || result.back() == '\r' || result.back() == ' ')) {
            result.pop_back();
        }
        if (!result.empty() && ((result.front() == '\'' && result.back() == '\'') || 
                               (result.front() == '"' && result.back() == '"'))) {
            result = result.substr(1, result.size() - 2);
        }
        g_use_hoc_theme = (result == "ElysiaOS-HoC");
    }
    g_theme_detected = true;
}

class PlayerctlInterface {
public:
    struct TrackInfo {
        std::string title = "No Media Playing";
        std::string artist = "Unknown Artist";
        std::string album = "Unknown Album";
        std::string art_url = "";
        double duration = 0.0;
        double position = 0.0;
        std::string status = "Stopped";
        bool has_media = false;
    };
    
    PlayerctlInterface() {
        // defer active player discovery until first query
    }
    
    TrackInfo get_metadata_snapshot() {
        TrackInfo info;
        
        std::string fmt = "metadata --format '{{playerName}}|{{status}}|{{title}}|{{artist}}|{{album}}|{{mpris:length}}|{{position}}|{{mpris:artUrl}}' 2>/dev/null";
        std::string combined = run_command(std::string("playerctl ") + fmt);
        if (combined.empty()) {
            info.has_media = false;
            return info;
        }
        
        std::array<std::string, 8> parts{};
        size_t start = 0; int idx = 0;
        while (idx < 8) {
            size_t pos = combined.find('|', start);
            std::string token = (pos == std::string::npos) ? combined.substr(start) : combined.substr(start, pos - start);
            parts[idx++] = token;
            if (pos == std::string::npos) break;
            start = pos + 1;
        }
        for (; idx < 8; ++idx) parts[idx] = "";
        
        if (!parts[0].empty()) active_player = parts[0];
        info.status = parts[1];
        info.title = parts[2];
        info.artist = parts[3];
        info.album = parts[4];
        
        if (!parts[5].empty()) {
            try {
                double us = std::stod(parts[5]);
                if (us > 0) info.duration = us / 1000000.0;
            } catch (...) {}
        }
        if (!parts[6].empty()) {
            try {
                double us = std::stod(parts[6]);
                if (us >= 0) info.position = us / 1000000.0;
            } catch (...) {}
        }
        
        if (info.duration <= 0) {
            std::string duration_fmt = run_command(make_playerctl_cmd("metadata --format '{{ duration(mpris:length) }}' 2>/dev/null"));
            if (!duration_fmt.empty() && duration_fmt.find("{{") == std::string::npos) {
                info.duration = parse_duration_string(duration_fmt);
            }
        }
        
        info.art_url = parts[7];
        if (!info.art_url.empty() && info.art_url != "{{ mpris:artUrl }}") {
            if (info.art_url.substr(0, 7) == "file://") {
                info.art_url = info.art_url.substr(7);
            }
        } else {
            info.art_url = "";
        }
        
        if (info.title.empty() || info.title == "{{ title }}") info.title = "Unknown Title";
        if (info.artist.empty() || info.artist == "{{ artist }}") info.artist = "Unknown Artist";
        if (info.album.empty() || info.album == "{{ album }}") info.album = "Unknown Album";
        
        if (info.duration < 0) info.duration = 0;
        if (info.position < 0) info.position = 0;
        if (info.duration > 0 && info.position > info.duration + 1.0) {
            info.position = info.duration;
        }
        
        info.has_media = !info.status.empty() && info.status != "No players found";
        
        return info;
    }

    double get_position_seconds() {
        std::string position_str;
        if (!active_player.empty()) {
            position_str = run_command(make_playerctl_cmd("position 2>/dev/null"));
        } else {
            position_str = run_command("playerctl position 2>/dev/null");
        }
        if (!position_str.empty()) {
            return parse_position_string(position_str);
        }
        return -1.0;
    }
    
    void play_pause() {
        run_command(make_playerctl_cmd("play-pause 2>/dev/null"));
    }
    
    void next_track() {
        run_command(make_playerctl_cmd("next 2>/dev/null"));
    }
    
    void previous_track() {
        run_command(make_playerctl_cmd("previous 2>/dev/null"));
    }
    
    void set_position(double seconds) {
        std::string cmd = make_playerctl_cmd(std::string("position ") + std::to_string(seconds) + " 2>/dev/null");
        run_command(cmd);
    }
    
private:
    std::string active_player;
    std::unordered_map<std::string, double> last_player_position_sec;
    
    struct PlayerSnapshot {
        std::string name;
        std::string status;
        double position_sec = 0.0;
        double duration_sec = 0.0;
        bool valid = false;
    };
 
    static std::string escape_single_quotes(const std::string &s) {
        std::string out;
        out.reserve(s.size() + 8);
        for (char c : s) {
            if (c == '\'') out += "'\\''"; else out += c;
        }
        return out;
    }
 
    std::string make_playerctl_cmd(const std::string &subcommand) {
        if (active_player.empty()) {
            return std::string("playerctl ") + subcommand;
        }
        return std::string("playerctl -p '") + escape_single_quotes(active_player) + "' " + subcommand;
    }
 
    void refresh_active_player() {
        std::string list = run_command("playerctl -l 2>/dev/null");
        if (list.empty()) {
            active_player.clear();
            return;
        }
        std::vector<std::string> players;
        {
            std::stringstream ss(list);
            std::string line;
            while (std::getline(ss, line)) {
                if (!line.empty()) players.push_back(line);
            }
        }
        
        std::vector<PlayerSnapshot> snapshots;
        snapshots.reserve(players.size());
        for (const auto &p : players) {
            PlayerSnapshot snap;
            snap.name = p;
            std::string resp = run_command(std::string("playerctl -p '") + escape_single_quotes(p) + "' metadata --format '{{status}}|{{position}}|{{mpris:length}}' 2>/dev/null");
            if (resp.empty()) { snapshots.push_back(snap); continue; }
            size_t a = resp.find('|');
            size_t b = (a == std::string::npos) ? std::string::npos : resp.find('|', a + 1);
            std::string st = (a == std::string::npos) ? resp : resp.substr(0, a);
            std::string pos_us = (a == std::string::npos || b == std::string::npos) ? std::string("") : resp.substr(a + 1, b - a - 1);
            std::string len_us = (b == std::string::npos) ? std::string("") : resp.substr(b + 1);
            snap.status = st;
            try { if (!pos_us.empty()) snap.position_sec = std::stod(pos_us) / 1000000.0; } catch (...) { snap.position_sec = 0.0; }
            try { if (!len_us.empty()) snap.duration_sec = std::stod(len_us) / 1000000.0; } catch (...) { snap.duration_sec = 0.0; }
            snap.valid = true;
            snapshots.push_back(snap);
        }
        
        std::string previous = active_player;
        if (!previous.empty()) {
            for (const auto &s : snapshots) {
                if (s.name == previous && (s.status == "Playing" || s.status == "Paused")) {
                    last_player_position_sec[s.name] = s.position_sec;
                    return;
                }
            }
        }
        
        std::vector<PlayerSnapshot> playing;
        for (const auto &s : snapshots) if (s.valid && s.status == "Playing") playing.push_back(s);
        
        if (playing.size() == 1) {
            active_player = playing[0].name;
            last_player_position_sec[active_player] = playing[0].position_sec;
            return;
        }
        
        if (playing.size() > 1) {
            double best_delta = -1e9;
            std::string best;
            for (const auto &s : playing) {
                double last = (last_player_position_sec.count(s.name) ? last_player_position_sec[s.name] : s.position_sec);
                double delta = s.position_sec - last;
                if (delta < 0) delta = 1.0;
                if (delta > best_delta) { best_delta = delta; best = s.name; }
            }
            if (!best.empty()) {
                active_player = best;
                last_player_position_sec[active_player] = 0.0;
                return;
            }
        }
        
        for (const auto &s : snapshots) {
            if (s.valid && s.status == "Paused") {
                active_player = s.name;
                last_player_position_sec[active_player] = s.position_sec;
                return;
            }
        }
        
        for (const auto &s : snapshots) {
            if (s.valid) {
                active_player = s.name;
                last_player_position_sec[active_player] = s.position_sec;
                return;
            }
        }
        
        active_player.clear();
    }
 
    double parse_duration_string(const std::string& duration_str) {
        std::regex time_regex(R"((?:(\d+):)?(\d+):(\d+))");
        std::smatch matches;
        
        if (std::regex_search(duration_str, matches, time_regex)) {
            double total_seconds = 0.0;
            
            if (matches[1].matched) {
                total_seconds += std::stod(matches[1]) * 3600;
            }
            
            total_seconds += std::stod(matches[2]) * 60;
            total_seconds += std::stod(matches[3]);
            
            return total_seconds;
        }
        
        try {
            return std::stod(duration_str);
        } catch (...) {
            return 0.0;
        }
    }

    double parse_position_string(const std::string& position_str) {
        double from_colon = parse_duration_string(position_str);
        if (from_colon > 0.0) {
            return from_colon;
        }
        
        std::string cleaned;
        cleaned.reserve(position_str.size());
        for (char c : position_str) {
            if (std::isdigit(static_cast<unsigned char>(c)) || c == '.' || c == ',') {
                cleaned.push_back(c);
            } else if (std::isspace(static_cast<unsigned char>(c)) || std::isalpha(static_cast<unsigned char>(c))) {
                break;
            }
        }
        if (cleaned.empty()) {
            return 0.0;
        }
        std::replace(cleaned.begin(), cleaned.end(), ',', '.');
        try {
            return std::stod(cleaned);
        } catch (...) {
            return 0.0;
        }
    }
     
    std::string run_command(const std::string& cmd) {
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return "";
        
        char buffer[1024];
        std::string result;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
        pclose(pipe);
        
        while (!result.empty() && (result.back() == '\n' || result.back() == '\r' || result.back() == ' ')) {
            result.pop_back();
        }
        
        return result;
    }
};

// Optimized calendar with cached rendering
class CalendarWidget {
private:
    GtkWidget *calendar_container;
    GtkWidget *header_box;
    GtkWidget *month_year_label;
    GtkWidget *grid;
    GtkWidget *prev_month_button;
    GtkWidget *next_month_button;
    
    std::time_t current_time;
    struct tm current_date;
    
    // Cache for avoiding unnecessary rebuilds
    int cached_month = -1;
    int cached_year = -1;
    
public:
    CalendarWidget() {
        current_time = std::time(nullptr);
        current_date = *std::localtime(&current_time);
        setup_ui();
        update_calendar();
    }
    
    GtkWidget* get_widget() {
        return calendar_container;
    }
    
private:
    void setup_ui() {
        calendar_container = gtk_event_box_new();
        g_signal_connect(calendar_container, "draw", G_CALLBACK(on_draw_static), this);
        
        GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_widget_set_margin_start(main_vbox, 16);
        gtk_widget_set_margin_end(main_vbox, 16);
        gtk_widget_set_margin_top(main_vbox, 12);
        gtk_widget_set_margin_bottom(main_vbox, 16);
        gtk_container_add(GTK_CONTAINER(calendar_container), main_vbox);
        
        header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_widget_set_margin_bottom(header_box, 12);
        
        prev_month_button = gtk_button_new();
        GtkWidget *prev_icon = gtk_label_new("‹");
        gtk_widget_set_name(prev_icon, "nav-icon");
        gtk_container_add(GTK_CONTAINER(prev_month_button), prev_icon);
        gtk_button_set_relief(GTK_BUTTON(prev_month_button), GTK_RELIEF_NONE);
        gtk_widget_set_name(prev_month_button, "nav-button");
        g_signal_connect(prev_month_button, "clicked", G_CALLBACK(on_prev_month_static), this);
        gtk_box_pack_start(GTK_BOX(header_box), prev_month_button, FALSE, FALSE, 0);
        
        month_year_label = gtk_label_new("");
        gtk_widget_set_name(month_year_label, "month-year-label");
        gtk_widget_set_hexpand(month_year_label, TRUE);
        gtk_widget_set_halign(month_year_label, GTK_ALIGN_CENTER);
        gtk_box_pack_start(GTK_BOX(header_box), month_year_label, TRUE, TRUE, 0);
        
        next_month_button = gtk_button_new();
        GtkWidget *next_icon = gtk_label_new("›");
        gtk_widget_set_name(next_icon, "nav-icon");
        gtk_container_add(GTK_CONTAINER(next_month_button), next_icon);
        gtk_button_set_relief(GTK_BUTTON(next_month_button), GTK_RELIEF_NONE);
        gtk_widget_set_name(next_month_button, "nav-button");
        g_signal_connect(next_month_button, "clicked", G_CALLBACK(on_next_month_static), this);
        gtk_box_pack_start(GTK_BOX(header_box), next_month_button, FALSE, FALSE, 0);
        
        gtk_box_pack_start(GTK_BOX(main_vbox), header_box, FALSE, FALSE, 0);
        
        grid = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(grid), 4);
        gtk_grid_set_column_spacing(GTK_GRID(grid), 4);
        gtk_widget_set_halign(grid, GTK_ALIGN_CENTER);
        gtk_box_pack_start(GTK_BOX(main_vbox), grid, TRUE, TRUE, 0);
    }
    
    void update_calendar() {
        // Skip rebuild if month/year unchanged
        if (cached_month == current_date.tm_mon && cached_year == current_date.tm_year) {
            return;
        }
        cached_month = current_date.tm_mon;
        cached_year = current_date.tm_year;
        
        // Clear existing grid children
        GList *children = gtk_container_get_children(GTK_CONTAINER(grid));
        for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
            gtk_widget_destroy(GTK_WIDGET(iter->data));
        }
        g_list_free(children);
        
        // Update month/year label
        static const char* months[] = {
            "JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY", "JUNE",
            "JULY", "AUGUST", "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"
        };
        char date_str[64];
        snprintf(date_str, sizeof(date_str), "%s %d", months[current_date.tm_mon], current_date.tm_year + 1900);
        gtk_label_set_text(GTK_LABEL(month_year_label), date_str);
        
        // Add day headers
        static const char* days[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
        for (int i = 0; i < 7; i++) {
            GtkWidget *day_header = gtk_label_new(days[i]);
            gtk_widget_set_name(day_header, "day-header");
            gtk_widget_set_size_request(day_header, 32, 20);
            gtk_grid_attach(GTK_GRID(grid), day_header, i, 0, 1, 1);
        }
        
        // Calculate first day of month
        struct tm first_day = current_date;
        first_day.tm_mday = 1;
        mktime(&first_day);
        
        // Get current date for highlighting
        std::time_t now = std::time(nullptr);
        struct tm today = *std::localtime(&now);
        
        // Calculate days in month
        struct tm next_month = current_date;
        next_month.tm_mon++;
        if (next_month.tm_mon > 11) {
            next_month.tm_mon = 0;
            next_month.tm_year++;
        }
        next_month.tm_mday = 1;
        mktime(&next_month);
        
        struct tm last_day = next_month;
        last_day.tm_mday = 0;
        mktime(&last_day);
        int days_in_month = last_day.tm_mday;
        
        // Fill in the calendar
        int row = 1;
        int col = first_day.tm_wday;
        
        // Previous month's trailing days
        if (col > 0) {
            struct tm prev_month = current_date;
            prev_month.tm_mon--;
            if (prev_month.tm_mon < 0) {
                prev_month.tm_mon = 11;
                prev_month.tm_year--;
            }
            prev_month.tm_mday = 1;
            mktime(&prev_month);
            
            struct tm prev_last = prev_month;
            prev_last.tm_mon++;
            if (prev_last.tm_mon > 11) {
                prev_last.tm_mon = 0;
                prev_last.tm_year++;
            }
            prev_last.tm_mday = 0;
            mktime(&prev_last);
            
            int prev_days = prev_last.tm_mday;
            for (int i = col - 1; i >= 0; i--) {
                int day = prev_days - i;
                GtkWidget *day_button = gtk_button_new_with_label(std::to_string(day).c_str());
                gtk_widget_set_name(day_button, "day-other");
                gtk_widget_set_size_request(day_button, 32, 28);
                gtk_button_set_relief(GTK_BUTTON(day_button), GTK_RELIEF_NONE);
                gtk_grid_attach(GTK_GRID(grid), day_button, col - 1 - i, row, 1, 1);
            }
        }
        
        // Current month's days
        for (int day = 1; day <= days_in_month; day++) {
            if (col >= 7) {
                col = 0;
                row++;
            }
            
            GtkWidget *day_button = gtk_button_new_with_label(std::to_string(day).c_str());
            gtk_widget_set_size_request(day_button, 32, 28);
            gtk_button_set_relief(GTK_BUTTON(day_button), GTK_RELIEF_NONE);
            
            bool is_today = (current_date.tm_year == today.tm_year && 
                           current_date.tm_mon == today.tm_mon && 
                           day == today.tm_mday);
            
            if (is_today) {
                gtk_widget_set_name(day_button, "day-today");
            } else {
                gtk_widget_set_name(day_button, "day-current");
            }
            
            gtk_grid_attach(GTK_GRID(grid), day_button, col, row, 1, 1);
            col++;
        }
        
        // Next month's leading days
        while (col < 7 && row <= 6) {
            int day = col - (7 - (days_in_month + first_day.tm_wday - 1) % 7) + days_in_month;
            if (day > days_in_month) {
                day = day - days_in_month;
                GtkWidget *day_button = gtk_button_new_with_label(std::to_string(day).c_str());
                gtk_widget_set_name(day_button, "day-other");
                gtk_widget_set_size_request(day_button, 32, 28);
                gtk_button_set_relief(GTK_BUTTON(day_button), GTK_RELIEF_NONE);
                gtk_grid_attach(GTK_GRID(grid), day_button, col, row, 1, 1);
            }
            col++;
            if (col >= 7) {
                col = 0;
                row++;
            }
        }
        
        gtk_widget_show_all(calendar_container);
    }
    
    static gboolean on_draw_static(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
        CalendarWidget *self = static_cast<CalendarWidget*>(user_data);
        return self->on_draw(widget, cr);
    }
    
    gboolean on_draw(GtkWidget *widget, cairo_t *cr) {
        GtkAllocation allocation;
        gtk_widget_get_allocation(widget, &allocation);
        
        double width = allocation.width;
        double height = allocation.height;
        double radius = 14.0;
        
        cairo_new_sub_path(cr);
        cairo_arc(cr, radius, radius, radius, M_PI, 3 * M_PI / 2);
        cairo_arc(cr, width - radius, radius, radius, 3 * M_PI / 2, 0);
        cairo_arc(cr, width - radius, height - radius, radius, 0, M_PI / 2);
        cairo_arc(cr, radius, height - radius, radius, M_PI / 2, M_PI);
        cairo_close_path(cr);
        
        // Drop shadow
        cairo_save(cr);
        cairo_new_sub_path(cr);
        cairo_arc(cr, radius+2, radius+2, radius, M_PI, 3 * M_PI / 2);
        cairo_arc(cr, width - radius + 2, radius + 2, radius, 3 * M_PI / 2, 0);
        cairo_arc(cr, width - radius + 2, height - radius + 2, radius, 0, M_PI / 2);
        cairo_arc(cr, radius + 2, height - radius + 2, radius, M_PI / 2, M_PI);
        cairo_close_path(cr);
        cairo_set_source_rgba(cr, 0, 0, 0, 0.18);
        cairo_fill(cr);
        cairo_restore(cr);

        // Card background
        cairo_new_sub_path(cr);
        cairo_arc(cr, radius, radius, radius, M_PI, 3 * M_PI / 2);
        cairo_arc(cr, width - radius, radius, radius, 3 * M_PI / 2, 0);
        cairo_arc(cr, width - radius, height - radius, radius, 0, M_PI / 2);
        cairo_arc(cr, radius, height - radius, radius, M_PI / 2, M_PI);
        cairo_close_path(cr);

        // Theme gradient
        cairo_pattern_t *gradient = cairo_pattern_create_linear(0, 0, width, height);
        if (g_use_hoc_theme) {
            cairo_pattern_add_color_stop_rgba(gradient, 0.0, 0.694, 0.788, 0.925, 0.60);
            cairo_pattern_add_color_stop_rgba(gradient, 1.0, 0.439, 0.467, 0.741, 0.45);
        } else {
            cairo_pattern_add_color_stop_rgba(gradient, 0.0, 0.933, 0.698, 0.812, 0.90);
            cairo_pattern_add_color_stop_rgba(gradient, 1.0, 1.0, 0.94, 0.98, 0.16);
        }
        cairo_set_source(cr, gradient);
        cairo_fill_preserve(cr);

        cairo_set_source_rgba(cr, 1, 1, 1, 0.65);
        cairo_set_line_width(cr, 1.2);
        cairo_stroke(cr);

        cairo_pattern_destroy(gradient);
        
        return FALSE;
    }
    
    static void on_prev_month_static(GtkButton *button, gpointer user_data) {
        CalendarWidget *self = static_cast<CalendarWidget*>(user_data);
        self->on_prev_month();
    }
    
    static void on_next_month_static(GtkButton *button, gpointer user_data) {
        CalendarWidget *self = static_cast<CalendarWidget*>(user_data);
        self->on_next_month();
    }
    
    void on_prev_month() {
        current_date.tm_mon--;
        if (current_date.tm_mon < 0) {
            current_date.tm_mon = 11;
            current_date.tm_year--;
        }
        mktime(&current_date);
        update_calendar();
    }
    
    void on_next_month() {
        current_date.tm_mon++;
        if (current_date.tm_mon > 11) {
            current_date.tm_mon = 0;
            current_date.tm_year++;
        }
        mktime(&current_date);
        update_calendar();
    }
};

class MusicWidget {
private:
    GtkWidget *window;
    GtkWidget *card_container;
    GtkWidget *main_box;
    GtkWidget *root_vbox;
    GtkWidget *album_overlay;
    GtkWidget *album_canvas;
    GtkWidget *title_label;
    GtkWidget *artist_label;
    GtkWidget *progress_bar;
    GtkWidget *current_time_label;
    GtkWidget *total_time_label;
    GtkWidget *play_button;
    GtkWidget *prev_button;
    GtkWidget *next_button;
    
    std::unique_ptr<PlayerctlInterface> media_interface;
    PlayerctlInterface::TrackInfo current_track;
    
    guint update_timer_id = 0;
    guint rotate_timer_id = 0;
    GdkPixbuf *default_album_art = nullptr;
    GdkPixbuf *current_album_art = nullptr;
    GdkPixbuf *disc_pixbuf = nullptr;
    std::string last_art_url = "";
    
    bool is_dragging_progress = false;
    std::string last_title = "";
    std::string last_artist = "";
    double last_duration = 0.0;
    
    // Smooth UI position tracking
    double displayed_position = 0.0;
    std::chrono::steady_clock::time_point last_tick_time = std::chrono::steady_clock::now();
    double last_backend_position = -1.0;
    int stagnant_ticks = 0;
    bool backend_position_suspicious = false;
    bool indeterminate_progress = false;

    // Sizes for disc and inner art
    int disc_size = 64;
    int art_size = 48;

    // Metadata refresh scheduling
    std::chrono::steady_clock::time_point last_metadata_refresh = std::chrono::steady_clock::time_point::min();
    bool has_metadata = false;

    // Rotation state
    double disc_angle_rad = 0.0;
    double art_angle_rad = 0.0;
    double disc_deg_per_sec = 36.0;
    double art_deg_per_sec = 54.0;

    // Calendar widget
    std::unique_ptr<CalendarWidget> calendar_widget;
    
    // Performance optimizations - cached pixbufs and surfaces
    cairo_surface_t *card_surface = nullptr;
    bool surface_needs_update = true;

    // JSON cache helpers
    static std::string escape_json(const std::string &s) {
        std::string out;
        out.reserve(s.size() + 8);
        for (char c : s) {
            switch (c) {
                case '"': out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20) {
                        char buf[7];
                        snprintf(buf, sizeof(buf), "\\u%04x", c);
                        out += buf;
                    } else {
                        out += c;
                    }
            }
        }
        return out;
    }

    void save_cached_metadata(const std::string &title, const std::string &artist) {
        const char *path = "/tmp/music_widget.json";
        std::ofstream ofs(path, std::ios::out | std::ios::trunc);
        if (!ofs.good()) return;
        std::time_t now = std::time(nullptr);
        ofs << "{\n"
            << "  \"title\": \"" << escape_json(title) << "\",\n"
            << "  \"artist\": \"" << escape_json(artist) << "\",\n"
            << "  \"updated\": " << static_cast<long long>(now) << "\n"
            << "}\n";
    }

    void load_cached_metadata() {
        const char *path = "/tmp/music_widget.json";
        std::ifstream ifs(path);
        if (!ifs.good()) return;
        std::stringstream buffer;
        buffer << ifs.rdbuf();
        std::string content = buffer.str();
        std::smatch m;
        std::string cached_title;
        std::string cached_artist;
        try {
            std::regex title_re("\\\"title\\\"\\s*:\\s*\\\"(.*?)\\\"");
            std::regex artist_re("\\\"artist\\\"\\s*:\\s*\\\"(.*?)\\\"");
            if (std::regex_search(content, m, title_re) && m.size() > 1) {
                cached_title = m[1].str();
            }
            if (std::regex_search(content, m, artist_re) && m.size() > 1) {
                cached_artist = m[1].str();
            }
        } catch (...) { return; }
        if (!cached_title.empty()) {
            gtk_label_set_text(GTK_LABEL(title_label), cached_title.c_str());
        }
        if (!cached_artist.empty()) {
            gtk_label_set_text(GTK_LABEL(artist_label), cached_artist.c_str());
        }
    }
 
public:
    MusicWidget() {
        
        // Detect theme once at startup
        detect_global_theme();
        
        // Defer media interface creation to first update to avoid blocking startup
        // media_interface will be created on demand in update_track_info()
        
        setup_window();
        setup_ui();
        apply_styles();
        
        // Start update timer with reduced frequency
        start_update_timer();
        
        // Defer first metadata update
        g_idle_add([](gpointer user_data) -> gboolean {
            MusicWidget *self = static_cast<MusicWidget*>(user_data);
            self->update_track_info();
            return G_SOURCE_REMOVE;
        }, this);
    }
    
    ~MusicWidget() {
        if (update_timer_id > 0) {
            g_source_remove(update_timer_id);
        }
        if (rotate_timer_id > 0) {
            g_source_remove(rotate_timer_id);
        }
        if (default_album_art) {
            g_object_unref(default_album_art);
        }
        if (current_album_art) {
            g_object_unref(current_album_art);
        }
        if (disc_pixbuf) {
            g_object_unref(disc_pixbuf);
        }
        if (card_surface) {
            cairo_surface_destroy(card_surface);
        }
    }
    
    void setup_window() {
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window), "Music Widget");
        gtk_window_set_default_size(GTK_WINDOW(window), 280, 80);
        gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
        gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
 
        gtk_layer_init_for_window(GTK_WINDOW(window));
        gtk_layer_set_layer(GTK_WINDOW(window), GTK_LAYER_SHELL_LAYER_OVERLAY);
        gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
        gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
        gtk_layer_set_anchor(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_RIGHT, FALSE);
        gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_TOP, 10);
        gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, 0);
 
        GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(window));
        GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
        if (visual) {
            gtk_widget_set_visual(window, visual);
        }
 
        gtk_widget_set_app_paintable(window, TRUE);
        g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    }
    
    void setup_ui() {
        // Main vertical container
        GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        root_vbox = main_vbox;
        gtk_container_add(GTK_CONTAINER(window), main_vbox);
        
        // Music widget card
        card_container = gtk_event_box_new();
        g_signal_connect(card_container, "draw", G_CALLBACK(on_draw_static), this);
        gtk_widget_add_events(card_container, GDK_BUTTON_PRESS_MASK);
        g_signal_connect(card_container, "button-press-event", G_CALLBACK(on_button_press_static), this);
        gtk_box_pack_start(GTK_BOX(main_vbox), card_container, FALSE, FALSE, 0);
        g_signal_connect(card_container, "size-allocate", G_CALLBACK(on_card_size_allocate_static), this);

        // Main horizontal container
        main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_widget_set_margin_start(main_box, 12);
        gtk_widget_set_margin_end(main_box, 12);
        gtk_widget_set_margin_top(main_box, 8);
        gtk_widget_set_margin_bottom(main_box, 10);
        gtk_container_add(GTK_CONTAINER(card_container), main_box);
        
        // Album disc + art
        album_overlay = gtk_overlay_new();
        gtk_widget_set_size_request(album_overlay, disc_size, disc_size);
        gtk_widget_set_halign(album_overlay, GTK_ALIGN_START);
        gtk_widget_set_valign(album_overlay, GTK_ALIGN_CENTER);

        album_canvas = gtk_drawing_area_new();
        gtk_widget_set_size_request(album_canvas, disc_size, disc_size);
        g_signal_connect(album_canvas, "draw", G_CALLBACK(on_album_draw_static), this);
        gtk_container_add(GTK_CONTAINER(album_overlay), album_canvas);
        
        // Defer loading disc and default art to avoid blocking startup
        g_idle_add([](gpointer user_data) -> gboolean {
            MusicWidget *self = static_cast<MusicWidget*>(user_data);
            self->load_disc_base_image();
            self->create_default_album_art();
            if (self->album_canvas) gtk_widget_queue_draw(self->album_canvas);
            return G_SOURCE_REMOVE;
        }, this);

        gtk_box_pack_start(GTK_BOX(main_box), album_overlay, FALSE, FALSE, 0);
        
        // Content container
        GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_widget_set_valign(content_box, GTK_ALIGN_CENTER);
        
        // Song title
        title_label = gtk_label_new("No Media Playing");
        gtk_label_set_xalign(GTK_LABEL(title_label), 0.0);
        gtk_widget_set_name(title_label, "title-label");
        gtk_label_set_ellipsize(GTK_LABEL(title_label), PANGO_ELLIPSIZE_END);
        gtk_label_set_max_width_chars(GTK_LABEL(title_label), 30);
        gtk_box_pack_start(GTK_BOX(content_box), title_label, FALSE, FALSE, 0);
        
        // Artist name
        artist_label = gtk_label_new("Unknown Artist");
        gtk_label_set_xalign(GTK_LABEL(artist_label), 0.0);
        gtk_widget_set_name(artist_label, "artist-label");
        gtk_label_set_ellipsize(GTK_LABEL(artist_label), PANGO_ELLIPSIZE_END);
        gtk_label_set_max_width_chars(GTK_LABEL(artist_label), 30);
        gtk_box_pack_start(GTK_BOX(content_box), artist_label, FALSE, FALSE, 0);

        // Pre-populate from cache for instant content
        load_cached_metadata();
        
        // Progress section
        GtkWidget *progress_section = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
        gtk_widget_set_margin_top(progress_section, 6);
        
        GtkWidget *progress_container = gtk_event_box_new();
        gtk_widget_set_size_request(progress_container, 170, 16);
        
        progress_bar = gtk_progress_bar_new();
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0.0);
        gtk_widget_set_size_request(progress_bar, 170, 5);
        gtk_widget_set_name(progress_bar, "music-progress");
        gtk_widget_set_valign(progress_bar, GTK_ALIGN_CENTER);
        
        gtk_container_add(GTK_CONTAINER(progress_container), progress_bar);
        
        gtk_widget_add_events(progress_container, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK);
        g_signal_connect(progress_container, "button-press-event", G_CALLBACK(on_progress_press_static), this);
        g_signal_connect(progress_container, "button-release-event", G_CALLBACK(on_progress_release_static), this);
        g_signal_connect(progress_container, "motion-notify-event", G_CALLBACK(on_progress_motion_static), this);
        
        gtk_box_pack_start(GTK_BOX(progress_section), progress_container, FALSE, FALSE, 0);
        
        // Time labels
        GtkWidget *time_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        
        current_time_label = gtk_label_new("0:00");
        gtk_label_set_xalign(GTK_LABEL(current_time_label), 0.0);
        gtk_widget_set_name(current_time_label, "time-label");
        gtk_box_pack_start(GTK_BOX(time_box), current_time_label, FALSE, FALSE, 0);
        
        total_time_label = gtk_label_new("0:00");
        gtk_label_set_xalign(GTK_LABEL(total_time_label), 1.0);
        gtk_widget_set_name(total_time_label, "time-label");
        gtk_box_pack_end(GTK_BOX(time_box), total_time_label, FALSE, FALSE, 0);
        
        gtk_box_pack_start(GTK_BOX(progress_section), time_box, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(content_box), progress_section, FALSE, FALSE, 0);
        
        gtk_box_pack_start(GTK_BOX(main_box), content_box, TRUE, TRUE, 0);
        
        // Control buttons
        GtkWidget *controls_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        gtk_widget_set_valign(controls_box, GTK_ALIGN_CENTER);
        
        prev_button = gtk_button_new();
        GtkWidget *prev_icon = gtk_label_new("⏮");
        gtk_widget_set_name(prev_icon, "control-icon");
        gtk_container_add(GTK_CONTAINER(prev_button), prev_icon);
        gtk_button_set_relief(GTK_BUTTON(prev_button), GTK_RELIEF_NONE);
        gtk_widget_set_name(prev_button, "control-button");
        g_signal_connect(prev_button, "clicked", G_CALLBACK(on_previous_static), this);
        gtk_box_pack_start(GTK_BOX(controls_box), prev_button, FALSE, FALSE, 0);
        
        play_button = gtk_button_new();
        GtkWidget *play_icon = gtk_label_new("▶");
        gtk_widget_set_name(play_icon, "control-icon");
        gtk_container_add(GTK_CONTAINER(play_button), play_icon);
        gtk_button_set_relief(GTK_BUTTON(play_button), GTK_RELIEF_NONE);
        gtk_widget_set_name(play_button, "play-button");
        g_signal_connect(play_button, "clicked", G_CALLBACK(on_play_pause_static), this);
        gtk_box_pack_start(GTK_BOX(controls_box), play_button, FALSE, FALSE, 0);
        
        next_button = gtk_button_new();
        GtkWidget *next_icon = gtk_label_new("⏭");
        gtk_widget_set_name(next_icon, "control-icon");
        gtk_container_add(GTK_CONTAINER(next_button), next_icon);
        gtk_button_set_relief(GTK_BUTTON(next_button), GTK_RELIEF_NONE);
        gtk_widget_set_name(next_button, "control-button");
        g_signal_connect(next_button, "clicked", G_CALLBACK(on_next_static), this);
        gtk_box_pack_start(GTK_BOX(controls_box), next_button, FALSE, FALSE, 0);
        
        gtk_box_pack_end(GTK_BOX(main_box), controls_box, FALSE, FALSE, 0);
        
        // Defer calendar widget creation to after initial show
        g_idle_add([](gpointer user_data) -> gboolean {
            MusicWidget *self = static_cast<MusicWidget*>(user_data);
            self->calendar_widget = std::make_unique<CalendarWidget>();
            gtk_box_pack_start(GTK_BOX(self->root_vbox), self->calendar_widget->get_widget(), FALSE, FALSE, 0);
            gtk_widget_show_all(self->root_vbox);
            return G_SOURCE_REMOVE;
        }, this);
    }
    
    void create_default_album_art() {
        if (default_album_art) return; // Already created
        
        int size = art_size;
        cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
        cairo_t *cr = cairo_create(surface);

        cairo_set_source_rgba(cr, 0, 0, 0, 0);
        cairo_paint(cr);

        double radius = size / 2.0;
        cairo_arc(cr, radius, radius, radius, 0, 2 * M_PI);
        cairo_clip(cr);

        cairo_pattern_t *gradient = cairo_pattern_create_linear(0, 0, size, size);
        cairo_pattern_add_color_stop_rgba(gradient, 0, 0.98, 0.82, 0.93, 1.0);
        cairo_pattern_add_color_stop_rgba(gradient, 1, 1.0, 0.97, 1.0, 1.0);
        cairo_set_source(cr, gradient);
        cairo_paint(cr);

        cairo_pattern_destroy(gradient);

        default_album_art = gdk_pixbuf_get_from_surface(surface, 0, 0, size, size);

        cairo_destroy(cr);
        cairo_surface_destroy(surface);
    }

    GdkPixbuf* create_circular_pixbuf(GdkPixbuf* source_pixbuf, int size) {
        int source_width = gdk_pixbuf_get_width(source_pixbuf);
        int source_height = gdk_pixbuf_get_height(source_pixbuf);

        double scale = std::min((double)size / source_width, (double)size / source_height);
        int scaled_width = std::max(1, (int)std::round(source_width * scale));
        int scaled_height = std::max(1, (int)std::round(source_height * scale));

        GdkPixbuf* scaled = gdk_pixbuf_scale_simple(source_pixbuf, scaled_width, scaled_height, GDK_INTERP_BILINEAR);

        cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
        cairo_t* cr = cairo_create(surface);

        cairo_set_source_rgba(cr, 0, 0, 0, 0);
        cairo_paint(cr);

        double r = size / 2.0;
        cairo_arc(cr, r, r, r, 0, 2 * M_PI);
        cairo_clip(cr);

        int offset_x = (size - scaled_width) / 2;
        int offset_y = (size - scaled_height) / 2;
        gdk_cairo_set_source_pixbuf(cr, scaled, offset_x, offset_y);
        cairo_paint(cr);

        g_object_unref(scaled);
        GdkPixbuf* result = gdk_pixbuf_get_from_surface(surface, 0, 0, size, size);
        cairo_destroy(cr);
        cairo_surface_destroy(surface);
        return result;
    }

    void load_disc_base_image() {
        if (disc_pixbuf) return; // Already loaded
        
        const char* home = g_get_home_dir();
        std::string path = std::string(home ? home : "") + "/.config/Elysia/assets/disc.png";
        GError* error = NULL;
        GdkPixbuf* pix = gdk_pixbuf_new_from_file_at_scale(path.c_str(), disc_size, disc_size, TRUE, &error);
        if (pix) {
            disc_pixbuf = pix;
        } else {
            if (error) g_error_free(error);
        }
    }
    
    void load_album_art(const std::string& /*art_url*/) {
        static time_t last_check_time = 0;
        const char* fixed_path = "/tmp/cover_waybar.png";
        
        struct stat file_stat;
        if (stat(fixed_path, &file_stat) != 0) {
            if (current_album_art) {
                g_object_unref(current_album_art);
                current_album_art = nullptr;
                if (album_canvas) gtk_widget_queue_draw(album_canvas);
            }
            last_art_url = "";
            return;
        }
        
        // Only check file modification time if enough time has passed
        if (file_stat.st_mtime <= last_check_time) {
            return;
        }
        last_check_time = file_stat.st_mtime;
        
        std::string file_id = std::string(fixed_path) + "_" + std::to_string(file_stat.st_mtime);
        if (last_art_url == file_id) {
            return;
        }
        
        last_art_url = file_id;

        GError *error = NULL;
        GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale(fixed_path, art_size, art_size, TRUE, &error);
        if (pixbuf) {
            GdkPixbuf *circle_pixbuf = create_circular_pixbuf(pixbuf, art_size);
            if (current_album_art) g_object_unref(current_album_art);
            current_album_art = circle_pixbuf;
            if (album_canvas) gtk_widget_queue_draw(album_canvas);
            g_object_unref(pixbuf);
        } else {
            if (error) g_error_free(error);
            if (current_album_art) {
                g_object_unref(current_album_art);
                current_album_art = nullptr;
            }
            if (album_canvas) gtk_widget_queue_draw(album_canvas);
        }
    }
    
    void update_track_info() {
        using clock = std::chrono::steady_clock;
        auto now = clock::now();
        
        // Lazy-create media interface to avoid blocking startup
        if (!media_interface) {
            media_interface = std::unique_ptr<PlayerctlInterface>(new PlayerctlInterface());
            last_metadata_refresh = now;
            return;
        }
        
        // Reduced metadata refresh frequency
        bool refresh_meta = (!has_metadata) || 
            (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_metadata_refresh).count() >= 1500);
        
        if (refresh_meta) {
            PlayerctlInterface::TrackInfo meta = media_interface->get_metadata_snapshot();
            bool track_changed = (meta.title != last_title || meta.artist != last_artist || meta.duration != last_duration);
            
            current_track.status = meta.status;
            current_track.title = meta.title;
            current_track.artist = meta.artist;
            current_track.album = meta.album;
            current_track.duration = meta.duration;
            current_track.art_url = meta.art_url;
            current_track.has_media = meta.has_media;

            if (track_changed) {
                last_title = current_track.title;
                last_artist = current_track.artist;
                last_duration = current_track.duration;
                
                if (current_track.status == "Playing" && current_track.duration > 0 && current_track.position >= current_track.duration - 0.25) {
                    displayed_position = 0.0;
                    backend_position_suspicious = true;
                    stagnant_ticks = 10;
                    last_backend_position = current_track.position;
                } else {
                    displayed_position = current_track.position;
                }
                last_tick_time = now;
                if (!backend_position_suspicious) {
                    last_backend_position = current_track.position;
                    stagnant_ticks = 0;
                }
                
                gtk_label_set_text(GTK_LABEL(title_label), current_track.title.c_str());
                gtk_label_set_text(GTK_LABEL(artist_label), current_track.artist.c_str());
                if (current_track.has_media) {
                    save_cached_metadata(current_track.title, current_track.artist);
                }
                
                // Load album art with less frequency checking
                load_album_art(current_track.art_url);
            }
            has_metadata = true;
            last_metadata_refresh = now;
        } else {
            // Still check album art occasionally but less frequently
            static int art_check_counter = 0;
            if (++art_check_counter >= 3) { // Every 3rd update
                art_check_counter = 0;
                load_album_art(current_track.art_url);
            }
        }

        // Quick position refresh
        double pos = media_interface->get_position_seconds();
        if (pos >= 0) current_track.position = pos;
        
        // Update play button
        GtkWidget *child = gtk_bin_get_child(GTK_BIN(play_button));
        if (child) {
            gtk_container_remove(GTK_CONTAINER(play_button), child);
        }
        
        bool is_playing = (current_track.status == "Playing");
        GtkWidget *icon = gtk_label_new(is_playing ? "⏸" : "▶");
        gtk_widget_set_name(icon, "control-icon");
        gtk_container_add(GTK_CONTAINER(play_button), icon);
        gtk_widget_show_all(play_button);

        // Optimize rotation timer
        if (is_playing) {
            if (rotate_timer_id == 0) {
                rotate_timer_id = g_timeout_add(100, [](gpointer data)->gboolean{ // Reduced to 100ms
                    MusicWidget *self = static_cast<MusicWidget*>(data);
                    double dt = 0.1; // 100ms
                    self->disc_angle_rad += (self->disc_deg_per_sec * (M_PI/180.0)) * dt;
                    self->art_angle_rad  += (self->art_deg_per_sec  * (M_PI/180.0)) * dt;
                    if (self->disc_angle_rad > 2*M_PI) self->disc_angle_rad = fmod(self->disc_angle_rad, 2*M_PI);
                    if (self->art_angle_rad  > 2*M_PI) self->art_angle_rad  = fmod(self->art_angle_rad,  2*M_PI);
                    if (self->album_canvas) gtk_widget_queue_draw(self->album_canvas);
                    return TRUE;
                }, this);
            }
        } else {
            if (rotate_timer_id != 0) { g_source_remove(rotate_timer_id); rotate_timer_id = 0; }
        }
        
        // Position stagnation detection
        if (is_playing && !is_dragging_progress) {
            if (last_backend_position >= 0.0 && std::fabs(current_track.position - last_backend_position) < 0.05) {
                stagnant_ticks += 1;
            } else {
                stagnant_ticks = 0;
            }
            last_backend_position = current_track.position;
            if (current_track.duration > 0 && current_track.position >= current_track.duration - 0.25) {
                backend_position_suspicious = true;
            }
        } else if (!is_playing) {
            backend_position_suspicious = false;
            stagnant_ticks = 0;
        }
 
        // Smooth position update
        if (!is_dragging_progress) {
            if (is_playing) {
                std::chrono::duration<double> dt = now - last_tick_time;
                if (!backend_position_suspicious && std::abs(current_track.position - displayed_position) > 1.0) {
                    displayed_position = current_track.position;
                } else {
                    displayed_position += dt.count();
                }
            } else {
                displayed_position = current_track.position;
            }
            if (current_track.duration > 0) {
                displayed_position = std::min(std::max(0.0, displayed_position), current_track.duration);
            } else {
                displayed_position = std::max(0.0, displayed_position);
            }
        }
        last_tick_time = now;
 
        // Update progress
        indeterminate_progress = backend_position_suspicious || current_track.duration <= 0.0;
        if (indeterminate_progress) {
            gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progress_bar));
            gtk_progress_bar_set_pulse_step(GTK_PROGRESS_BAR(progress_bar), 0.05);
        } else {
            double progress = 0.0;
            if (current_track.duration > 0) {
                progress = displayed_position / current_track.duration;
                progress = std::max(0.0, std::min(1.0, progress));
            }
            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), progress);
        }
        
        // Format time efficiently
        auto format_time = [](double seconds) -> std::string {
            if (seconds < 0) seconds = 0;
            int mins = (int)seconds / 60;
            int secs = (int)seconds % 60;
            char buffer[16];
            snprintf(buffer, sizeof(buffer), "%d:%02d", mins, secs);
            return std::string(buffer);
        };
        
        if (!is_dragging_progress) {
            gtk_label_set_text(GTK_LABEL(current_time_label), format_time(displayed_position).c_str());
        }
        if (indeterminate_progress) {
            gtk_label_set_text(GTK_LABEL(total_time_label), "--:--");
        } else {
            gtk_label_set_text(GTK_LABEL(total_time_label), format_time(current_track.duration).c_str());
        }
    }
    
    void apply_styles() {
        static bool styles_applied = false;
        if (styles_applied) return; // Apply only once
        styles_applied = true;
        
        GtkCssProvider *provider = gtk_css_provider_new();
        std::string css;
        if (g_use_hoc_theme) {
            css = R"(
                window { background-color: rgba(0,0,0,0); }
                #title-label { font-family: ElysiaOSNew12; font-size: 13px; font-weight: 600; color: #2a2a2a; margin-bottom: 2px; }
                #artist-label { font-family: ElysiaOSNew12; font-size: 11px; font-weight: 400; color: #6a6a6a; }
                #music-progress { background-color: rgba(255,255,255,0.4); border-radius: 2px; cursor: pointer; }
                #music-progress progress { background: linear-gradient(90deg, #7077bd 0%, #b1c9ec 100%); border-radius: 2px; border: none; }
                #time-label { font-family: ElysiaOSNew12; font-size: 10px; font-weight: 500; color: #8a8a8a; }
                #control-button { background-color: rgba(255,255,255,0.15); border-radius: 16px; border: none; min-width: 32px; min-height: 32px; padding: 0; transition: all 0.2s ease; }
                #play-button { background-color: rgba(112,119,189,0.2); border-radius: 20px; border: none; min-width: 42px; min-height: 42px; padding: 0; transition: all 0.2s ease; }
                #control-button:hover, #play-button:hover { background-color: rgba(255,255,255,0.25); transform: scale(1.04); }
                #play-button:hover { background-color: rgba(112,119,189,0.3); }
                #control-icon { font-size: 14px; color: #4a4a4a; font-weight: bold; }
                #month-year-label { font-family: ElysiaOSNew12; font-size: 13px; font-weight: 700; color: #2a2a2a; letter-spacing: 0.5px; }
                #day-header { font-family: ElysiaOSNew12; font-size: 9px; font-weight: 600; color: #7077bd; letter-spacing: 0.3px; }
                #day-current { font-family: ElysiaOSNew12; font-size: 11px; font-weight: 500; color: #3a3a3a; background: rgba(255,255,255,0.1); border-radius: 6px; border: none; transition: all 0.2s ease; }
                #day-today { font-family: ElysiaOSNew12; font-size: 11px; font-weight: 700; color: #ffffff; background: linear-gradient(135deg, #7077bd 0%, #b1c9ec 100%); border-radius: 6px; border: none; transition: all 0.2s ease; }
                #day-other { font-family: ElysiaOSNew12; font-size: 11px; font-weight: 400; color: #aaaaaa; background: transparent; border: none; }
                #day-current:hover, #day-today:hover { transform: scale(1.05); background-color: rgba(112,119,189,0.2); }
                #nav-button { background-color: rgba(255,255,255,0.15); border-radius: 12px; border: none; min-width: 28px; min-height: 28px; padding: 0; transition: all 0.2s ease; }
                #nav-button:hover { background-color: rgba(255,255,255,0.25); transform: scale(1.05); }
                #nav-icon { font-size: 16px; color: #4a4a4a; font-weight: bold; }
            )";
        } else {
            css = R"(
                window { background-color: rgba(0,0,0,0); }
                #title-label { font-family: ElysiaOSNew12; font-size: 13px; font-weight: 600; color: #2a2a2a; margin-bottom: 2px; }
                #artist-label { font-family: ElysiaOSNew12; font-size: 11px; font-weight: 400; color: #6a6a6a; }
                #music-progress { background-color: rgba(255,255,255,0.4); border-radius: 2px; cursor: pointer; }
                #music-progress progress { background: linear-gradient(90deg, #ff6bb3 0%, #ffc4e1 100%); border-radius: 2px; border: none; }
                #time-label { font-family: ElysiaOSNew12; font-size: 10px; font-weight: 500; color: #8a8a8a; }
                #control-button { background-color: rgba(255,255,255,0.15); border-radius: 16px; border: none; min-width: 32px; min-height: 32px; padding: 0; transition: all 0.2s ease; }
                #play-button { background-color: rgba(255,105,180,0.2); border-radius: 20px; border: none; min-width: 42px; min-height: 42px; padding: 0; transition: all 0.2s ease; }
                #control-button:hover, #play-button:hover { background-color: rgba(255,255,255,0.25); transform: scale(1.04); }
                #play-button:hover { background-color: rgba(255,105,180,0.3); }
                #control-icon { font-size: 14px; color: #4a4a4a; font-weight: bold; }
                #month-year-label { font-family: ElysiaOSNew12; font-size: 13px; font-weight: 700; color: #2a2a2a; letter-spacing: 0.5px; }
                #day-header { font-family: ElysiaOSNew12; font-size: 9px; font-weight: 600; color: #ff6bb3; letter-spacing: 0.3px; }
                #day-current { font-family: ElysiaOSNew12; font-size: 11px; font-weight: 500; color: #3a3a3a; background: rgba(255,255,255,0.1); border-radius: 6px; border: none; transition: all 0.2s ease; }
                #day-today { font-family: ElysiaOSNew12; font-size: 11px; font-weight: 700; color: #ffffff; background: linear-gradient(135deg, #ff6bb3 0%, #ffc4e1 100%); border-radius: 6px; border: none; transition: all 0.2s ease; }
                #day-other { font-family: ElysiaOSNew12; font-size: 11px; font-weight: 400; color: #aaaaaa; background: transparent; border: none; }
                #day-current:hover, #day-today:hover { transform: scale(1.05); background-color: rgba(255,105,180,0.2); }
                #nav-button { background-color: rgba(255,255,255,0.15); border-radius: 12px; border: none; min-width: 28px; min-height: 28px; padding: 0; transition: all 0.2s ease; }
                #nav-button:hover { background-color: rgba(255,255,255,0.25); transform: scale(1.05); }
                #nav-icon { font-size: 16px; color: #4a4a4a; font-weight: bold; }
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
    
    static gboolean on_draw_static(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
        MusicWidget *self = static_cast<MusicWidget*>(user_data);
        return self->on_draw(widget, cr);
    }
    
    gboolean on_draw(GtkWidget *widget, cairo_t *cr) {
        GtkAllocation allocation;
        gtk_widget_get_allocation(widget, &allocation);
        
        double width = allocation.width;
        double height = allocation.height;
        double radius = 14.0;
        
        // Create rounded rectangle background
        cairo_new_sub_path(cr);
        cairo_arc(cr, radius, radius, radius, M_PI, 3 * M_PI / 2);
        cairo_arc(cr, width - radius, radius, radius, 3 * M_PI / 2, 0);
        cairo_arc(cr, width - radius, height - radius, radius, 0, M_PI / 2);
        cairo_arc(cr, radius, height - radius, radius, M_PI / 2, M_PI);
        cairo_close_path(cr);
        
        // Drop shadow
        cairo_save(cr);
        cairo_new_sub_path(cr);
        cairo_arc(cr, radius+2, radius+2, radius, M_PI, 3 * M_PI / 2);
        cairo_arc(cr, width - radius + 2, radius + 2, radius, 3 * M_PI / 2, 0);
        cairo_arc(cr, width - radius + 2, height - radius + 2, radius, 0, M_PI / 2);
        cairo_arc(cr, radius + 2, height - radius + 2, radius, M_PI / 2, M_PI);
        cairo_close_path(cr);
        cairo_set_source_rgba(cr, 0, 0, 0, 0.18);
        cairo_fill(cr);
        cairo_restore(cr);

        // Card background
        cairo_new_sub_path(cr);
        cairo_arc(cr, radius, radius, radius, M_PI, 3 * M_PI / 2);
        cairo_arc(cr, width - radius, radius, radius, 3 * M_PI / 2, 0);
        cairo_arc(cr, width - radius, height - radius, radius, 0, M_PI / 2);
        cairo_arc(cr, radius, height - radius, radius, M_PI / 2, M_PI);
        cairo_close_path(cr);

        // Theme gradient
        cairo_pattern_t *gradient = cairo_pattern_create_linear(0, 0, width, height);
        if (g_use_hoc_theme) {
            cairo_pattern_add_color_stop_rgba(gradient, 0.0, 0.694, 0.788, 0.925, 0.60);
            cairo_pattern_add_color_stop_rgba(gradient, 1.0, 0.439, 0.467, 0.741, 0.45);
        } else {
            cairo_pattern_add_color_stop_rgba(gradient, 0.0, 0.933, 0.698, 0.812, 0.90);
            cairo_pattern_add_color_stop_rgba(gradient, 1.0, 1.0, 0.94, 0.98, 0.56);
        }
        cairo_set_source(cr, gradient);
        cairo_fill_preserve(cr);

        cairo_set_source_rgba(cr, 1, 1, 1, 0.65);
        cairo_set_line_width(cr, 1.2);
        cairo_stroke(cr);

        cairo_pattern_destroy(gradient);
        
        return FALSE;
    }
    
    // Event handlers (optimized for minimal processing)
    static gboolean on_button_press_static(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
        MusicWidget *self = static_cast<MusicWidget*>(user_data);
        if (event->button == 1) {
            gtk_window_begin_move_drag(GTK_WINDOW(self->window), event->button, event->x_root, event->y_root, event->time);
        }
        return TRUE;
    }
    
    static gboolean on_progress_press_static(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
        MusicWidget *self = static_cast<MusicWidget*>(user_data);
        return self->on_progress_press(widget, event);
    }
    
    static gboolean on_progress_release_static(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
        MusicWidget *self = static_cast<MusicWidget*>(user_data);
        return self->on_progress_release(widget, event);
    }
    
    static gboolean on_progress_motion_static(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
        MusicWidget *self = static_cast<MusicWidget*>(user_data);
        return self->on_progress_motion(widget, event);
    }
    
    gboolean on_progress_press(GtkWidget *widget, GdkEventButton *event) {
        if (event->button == 1 && current_track.duration > 0) {
            is_dragging_progress = true;
            update_progress_from_position(widget, event->x);
        }
        return TRUE;
    }
    
    gboolean on_progress_release(GtkWidget *widget, GdkEventButton *event) {
        if (event->button == 1 && is_dragging_progress) {
            is_dragging_progress = false;
            update_progress_from_position(widget, event->x);
            
            double progress = get_progress_from_position(widget, event->x);
            double new_position = progress * current_track.duration;
            media_interface->set_position(new_position);
            displayed_position = new_position;
            last_tick_time = std::chrono::steady_clock::now();
        }
        return TRUE;
    }
    
    gboolean on_progress_motion(GtkWidget *widget, GdkEventMotion *event) {
        if (is_dragging_progress && current_track.duration > 0) {
            update_progress_from_position(widget, event->x);
        }
        return TRUE;
    }
    
    double get_progress_from_position(GtkWidget *widget, double x) {
        GtkAllocation allocation;
        gtk_widget_get_allocation(widget, &allocation);
        
        double progress = x / allocation.width;
        return std::max(0.0, std::min(1.0, progress));
    }
    
    void update_progress_from_position(GtkWidget *widget, double x) {
        double progress = get_progress_from_position(widget, x);
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), progress);
        
        double preview_time = progress * current_track.duration;
        char buffer[16];
        int mins = (int)preview_time / 60;
        int secs = (int)preview_time % 60;
        snprintf(buffer, sizeof(buffer), "%d:%02d", mins, secs);
        gtk_label_set_text(GTK_LABEL(current_time_label), buffer);
    }
    
    static void on_play_pause_static(GtkButton *button, gpointer user_data) {
        MusicWidget *self = static_cast<MusicWidget*>(user_data);
        self->on_play_pause();
    }
    
    static void on_next_static(GtkButton *button, gpointer user_data) {
        MusicWidget *self = static_cast<MusicWidget*>(user_data);
        self->on_next();
    }
    
    static void on_previous_static(GtkButton *button, gpointer user_data) {
        MusicWidget *self = static_cast<MusicWidget*>(user_data);
        self->on_previous();
    }
    
    void on_play_pause() {
        media_interface->play_pause();
        g_timeout_add(200, [](gpointer user_data) -> gboolean {
            MusicWidget *self = static_cast<MusicWidget*>(user_data);
            self->update_track_info();
            return FALSE;
        }, this);
    }
    
    void on_next() {
        media_interface->next_track();
        g_timeout_add(500, [](gpointer user_data) -> gboolean {
            MusicWidget *self = static_cast<MusicWidget*>(user_data);
            self->update_track_info();
            return FALSE;
        }, this);
    }
    
    void on_previous() {
        media_interface->previous_track();
        g_timeout_add(500, [](gpointer user_data) -> gboolean {
            MusicWidget *self = static_cast<MusicWidget*>(user_data);
            self->update_track_info();
            return FALSE;
        }, this);
    }
    
    static gboolean update_timer_callback_static(gpointer user_data) {
        MusicWidget *self = static_cast<MusicWidget*>(user_data);
        self->update_track_info();
        return TRUE;
    }
    
    void start_update_timer() {
        // Increased interval to 500ms for better performance
        update_timer_id = g_timeout_add(500, update_timer_callback_static, this);
    }
    
    void show() {
        gtk_widget_show_all(window);
    }

    // Optimized album canvas draw with reduced redraws
    static gboolean on_album_draw_static(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
        MusicWidget *self = static_cast<MusicWidget*>(user_data);
        return self->on_album_draw(widget, cr);
    }

    gboolean on_album_draw(GtkWidget *widget, cairo_t *cr) {
        GtkAllocation alloc; 
        gtk_widget_get_allocation(widget, &alloc);
        double cx = alloc.width / 2.0;
        double cy = alloc.height / 2.0;

        // Draw rotating disc
        if (disc_pixbuf) {
            int w = gdk_pixbuf_get_width(disc_pixbuf);
            int h = gdk_pixbuf_get_height(disc_pixbuf);
            cairo_save(cr);
            cairo_translate(cr, cx, cy);
            cairo_rotate(cr, disc_angle_rad);
            gdk_cairo_set_source_pixbuf(cr, disc_pixbuf, -w/2.0, -h/2.0);
            cairo_paint(cr);
            cairo_restore(cr);
        }

        // Draw rotating album art
        GdkPixbuf *art = current_album_art ? current_album_art : default_album_art;
        if (art) {
            int w = gdk_pixbuf_get_width(art);
            int h = gdk_pixbuf_get_height(art);
            cairo_save(cr);
            cairo_translate(cr, cx, cy);
            cairo_rotate(cr, art_angle_rad);
            gdk_cairo_set_source_pixbuf(cr, art, -w/2.0, -h/2.0);
            cairo_paint(cr);
            cairo_restore(cr);
        }
        return FALSE;
    }

    void update_left_margin_for_center(int content_width) {
        GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(window));
        int monitor = gdk_screen_get_primary_monitor(screen);
        if (monitor < 0) monitor = 0;
        GdkRectangle geom; 
        gdk_screen_get_monitor_geometry(screen, monitor, &geom);
        int left = (geom.width - content_width) / 2;
        if (left < 0) left = 0;
        gtk_layer_set_margin(GTK_WINDOW(window), GTK_LAYER_SHELL_EDGE_LEFT, left);
    }

    static void on_card_size_allocate_static(GtkWidget *widget, GdkRectangle *allocation, gpointer user_data) {
        MusicWidget *self = static_cast<MusicWidget*>(user_data);
        self->update_left_margin_for_center(allocation->width);
    }
};

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    MusicWidget widget;
    widget.show();
    
    gtk_main();
    
    return 0;
}