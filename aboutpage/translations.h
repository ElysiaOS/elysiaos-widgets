#ifndef TRANSLATIONS_H
#define TRANSLATIONS_H

#include <string>
#include <map>
#include <cstdlib>

enum class Language {
    ENGLISH,
    JAPANESE,
    CHINESE,
    VIETNAMESE,
    RUSSIAN,
    FRENCH,
    SPANISH,
    INDONESIAN
};

class Translations {
private:
    Language currentLanguage;
    std::map<std::string, std::map<Language, std::string>> translations;

    void initializeTranslations() {
        // Window titles
        translations["about_title"] = {
            {Language::ENGLISH, "About This PC"},
            {Language::JAPANESE, "このPCについて"},
            {Language::CHINESE, "关于此电脑"},
            {Language::VIETNAMESE, "Giới thiệu về PC này"},
            {Language::RUSSIAN, "Об этом ПК"},
            {Language::FRENCH, "À propos de ce PC"},
            {Language::SPANISH, "Acerca de esta PC"},
            {Language::INDONESIAN, "Tentang PC ini"}
        };

        translations["keybinds_title"] = {
            {Language::ENGLISH, "ElysiaOS Keybinds"},
            {Language::JAPANESE, "ElysiaOSキーバインド"},
            {Language::CHINESE, "ElysiaOS快捷键"},
            {Language::VIETNAMESE, "Phím tắt ElysiaOS"},
            {Language::RUSSIAN, "Горячие клавиши ElysiaOS"},
            {Language::FRENCH, "Raccourcis ElysiaOS"},
            {Language::SPANISH, "Atajos de ElysiaOS"},
            {Language::INDONESIAN, "Tombol Pintas ElysiaOS"}
        };

        translations["system_info_title"] = {
            {Language::ENGLISH, "System Information"},
            {Language::JAPANESE, "システム情報"},
            {Language::CHINESE, "系统信息"},
            {Language::VIETNAMESE, "Thông tin hệ thống"},
            {Language::RUSSIAN, "Информация о системе"},
            {Language::FRENCH, "Informations système"},
            {Language::SPANISH, "Información del sistema"},
            {Language::INDONESIAN, "Informasi sistem"}
        };

        // Buttons
        translations["more_info_button"] = {
            {Language::ENGLISH, "More Info"},
            {Language::JAPANESE, "詳細情報"},
            {Language::CHINESE, "更多信息"},
            {Language::VIETNAMESE, "Thêm thông tin"},
            {Language::RUSSIAN, "Подробнее"},
            {Language::FRENCH, "Plus d'infos"},
            {Language::SPANISH, "Más información"},
            {Language::INDONESIAN, "Info lebih lanjut"}
        };

        translations["keybinds_button"] = {
            {Language::ENGLISH, "Keybinds"},
            {Language::JAPANESE, "キーバインド"},
            {Language::CHINESE, "快捷键"},
            {Language::VIETNAMESE, "Phím tắt"},
            {Language::RUSSIAN, "Горячие клавиши"},
            {Language::FRENCH, "Raccourcis"},
            {Language::SPANISH, "Atajos"},
            {Language::INDONESIAN, "Tombol pintas"}
        };

        translations["back_button"] = {
            {Language::ENGLISH, "← Back"},
            {Language::JAPANESE, "← 戻る"},
            {Language::CHINESE, "← 返回"},
            {Language::VIETNAMESE, "← Quay lại"},
            {Language::RUSSIAN, "← Назад"},
            {Language::FRENCH, "← Retour"},
            {Language::SPANISH, "← Atrás"},
            {Language::INDONESIAN, "← Kembali"}
        };

        translations["close_button"] = {
            {Language::ENGLISH, "← Close"},
            {Language::JAPANESE, "← 閉じる"},
            {Language::CHINESE, "← 关闭"},
            {Language::VIETNAMESE, "← Đóng"},
            {Language::RUSSIAN, "← Закрыть"},
            {Language::FRENCH, "← Fermer"},
            {Language::SPANISH, "← Cerrar"},
            {Language::INDONESIAN, "← Tutup"}
        };

        translations["about_button"] = {
            {Language::ENGLISH, "About"},
            {Language::JAPANESE, "このPCについて"},
            {Language::CHINESE, "关于"},
            {Language::VIETNAMESE, "Giới thiệu"},
            {Language::RUSSIAN, "О программе"},
            {Language::FRENCH, "À propos"},
            {Language::SPANISH, "Acerca de"},
            {Language::INDONESIAN, "Tentang"}
        };

        // System info labels
        translations["cpu_label"] = {
            {Language::ENGLISH, "CPU"},
            {Language::JAPANESE, "CPU"},
            {Language::CHINESE, "处理器"},
            {Language::VIETNAMESE, "CPU"},
            {Language::RUSSIAN, "Процессор"},
            {Language::FRENCH, "CPU"},
            {Language::SPANISH, "CPU"},
            {Language::INDONESIAN, "CPU"}
        };

        translations["gpu_label"] = {
            {Language::ENGLISH, "GPU"},
            {Language::JAPANESE, "GPU"},
            {Language::CHINESE, "显卡"},
            {Language::VIETNAMESE, "GPU"},
            {Language::RUSSIAN, "Видеокарта"},
            {Language::FRENCH, "GPU"},
            {Language::SPANISH, "GPU"},
            {Language::INDONESIAN, "GPU"}
        };

        translations["linux_version_label"] = {
            {Language::ENGLISH, "Linux Version"},
            {Language::JAPANESE, "Linuxバージョン"},
            {Language::CHINESE, "Linux版本"},
            {Language::VIETNAMESE, "Phiên bản Linux"},
            {Language::RUSSIAN, "Версия Linux"},
            {Language::FRENCH, "Version Linux"},
            {Language::SPANISH, "Versión de Linux"},
            {Language::INDONESIAN, "Versi Linux"}
        };

        translations["pc_name_label"] = {
            {Language::ENGLISH, "PC Name"},
            {Language::JAPANESE, "PC名"},
            {Language::CHINESE, "电脑名称"},
            {Language::VIETNAMESE, "Tên PC"},
            {Language::RUSSIAN, "Имя ПК"},
            {Language::FRENCH, "Nom du PC"},
            {Language::SPANISH, "Nombre del PC"},
            {Language::INDONESIAN, "Nama PC"}
        };

        translations["os_label"] = {
            {Language::ENGLISH, "OS"},
            {Language::JAPANESE, "OS"},
            {Language::CHINESE, "操作系统"},
            {Language::VIETNAMESE, "Hệ điều hành"},
            {Language::RUSSIAN, "ОС"},
            {Language::FRENCH, "OS"},
            {Language::SPANISH, "SO"},
            {Language::INDONESIAN, "OS"}
        };

        translations["packages_label"] = {
            {Language::ENGLISH, "Packages"},
            {Language::JAPANESE, "パッケージ"},
            {Language::CHINESE, "软件包"},
            {Language::VIETNAMESE, "Gói phần mềm"},
            {Language::RUSSIAN, "Пакеты"},
            {Language::FRENCH, "Paquets"},
            {Language::SPANISH, "Paquetes"},
            {Language::INDONESIAN, "Paket"}
        };

        translations["about_computer"] = {
            {Language::ENGLISH, "About This Computer"},
            {Language::JAPANESE, "このコンピュータについて"},
            {Language::CHINESE, "关于这台电脑"},
            {Language::VIETNAMESE, "Giới thiệu về máy tính này"},
            {Language::RUSSIAN, "Об этом компьютере"},
            {Language::FRENCH, "À propos de cet ordinateur"},
            {Language::SPANISH, "Acerca de esta computadora"},
            {Language::INDONESIAN, "Tentang komputer ini"}
        };

        translations["elysia_keybinds"] = {
            {Language::ENGLISH, "ElysiaOS Keybinds"},
            {Language::JAPANESE, "ElysiaOSキーバインド"},
            {Language::CHINESE, "ElysiaOS快捷键"},
            {Language::VIETNAMESE, "Phím tắt ElysiaOS"},
            {Language::RUSSIAN, "Горячие клавиши ElysiaOS"},
            {Language::FRENCH, "Raccourcis ElysiaOS"},
            {Language::SPANISH, "Atajos de ElysiaOS"},
            {Language::INDONESIAN, "Tombol Pintas ElysiaOS"}
        };

        // Keybind descriptions
        translations["close_window"] = {
            {Language::ENGLISH, "Close focused window"},
            {Language::JAPANESE, "フォーカスされたウィンドウを閉じる"},
            {Language::CHINESE, "关闭焦点窗口"},
            {Language::VIETNAMESE, "Đóng cửa sổ đang được chọn"},
            {Language::RUSSIAN, "Закрыть активное окно"},
            {Language::FRENCH, "Fermer la fenêtre active"},
            {Language::SPANISH, "Cerrar ventana activa"},
            {Language::INDONESIAN, "Tutup jendela yang aktif"}
        };

        translations["launch_app_manager"] = {
            {Language::ENGLISH, "Launch Application manager"},
            {Language::JAPANESE, "アプリケーションマネージャーを起動"},
            {Language::CHINESE, "启动应用程序管理器"},
            {Language::VIETNAMESE, "Khởi động trình quản lý ứng dụng"},
            {Language::RUSSIAN, "Запустить диспетчер приложений"},
            {Language::FRENCH, "Lancer le gestionnaire d'applications"},
            {Language::SPANISH, "Abrir gestor de aplicaciones"},
            {Language::INDONESIAN, "Luncurkan pengelola aplikasi"}
        };

        translations["terminal"] = {
            {Language::ENGLISH, "Terminal"},
            {Language::JAPANESE, "ターミナル"},
            {Language::CHINESE, "终端"},
            {Language::VIETNAMESE, "Terminal"},
            {Language::RUSSIAN, "Терминал"},
            {Language::FRENCH, "Terminal"},
            {Language::SPANISH, "Terminal"},
            {Language::INDONESIAN, "Terminal"}
        };

        translations["workspace_switcher"] = {
            {Language::ENGLISH, "Brings up the workspaces switcher"},
            {Language::JAPANESE, "ワークスペーススイッチャーを表示"},
            {Language::CHINESE, "打开工作区切换器"},
            {Language::VIETNAMESE, "Hiển thị bộ chuyển đổi không gian làm việc"},
            {Language::RUSSIAN, "Открыть переключатель рабочих пространств"},
            {Language::FRENCH, "Afficher le sélecteur d'espaces de travail"},
            {Language::SPANISH, "Mostrar selector de espacios de trabajo"},
            {Language::INDONESIAN, "Tampilkan pengalih ruang kerja"}
        };

        translations["change_language"] = {
            {Language::ENGLISH, "Change Language"},
            {Language::JAPANESE, "言語を変更"},
            {Language::CHINESE, "更改语言"},
            {Language::VIETNAMESE, "Thay đổi ngôn ngữ"},
            {Language::RUSSIAN, "Изменить язык"},
            {Language::FRENCH, "Changer de langue"},
            {Language::SPANISH, "Cambiar idioma"},
            {Language::INDONESIAN, "Ubah bahasa"}
        };

        translations["lock_screen"] = {
            {Language::ENGLISH, "Lock your screen Hyprlock"},
            {Language::JAPANESE, "画面をロック Hyprlock"},
            {Language::CHINESE, "锁定屏幕 Hyprlock"},
            {Language::VIETNAMESE, "Khóa màn hình Hyprlock"},
            {Language::RUSSIAN, "Заблокировать экран Hyprlock"},
            {Language::FRENCH, "Verrouiller l'écran Hyprlock"},
            {Language::SPANISH, "Bloquear pantalla Hyprlock"},
            {Language::INDONESIAN, "Kunci layar Hyprlock"}
        };

        translations["powermenu"] = {
            {Language::ENGLISH, "Powermenu"},
            {Language::JAPANESE, "電源メニュー"},
            {Language::CHINESE, "电源菜单"},
            {Language::VIETNAMESE, "Menu nguồn"},
            {Language::RUSSIAN, "Меню питания"},
            {Language::FRENCH, "Menu d'alimentation"},
            {Language::SPANISH, "Menú de energía"},
            {Language::INDONESIAN, "Menu daya"}
        };

        translations["switch_workspaces"] = {
            {Language::ENGLISH, "Switch workspaces"},
            {Language::JAPANESE, "ワークスペースを切り替え"},
            {Language::CHINESE, "切换工作区"},
            {Language::VIETNAMESE, "Chuyển đổi không gian làm việc"},
            {Language::RUSSIAN, "Переключить рабочие пространства"},
            {Language::FRENCH, "Changer d'espaces de travail"},
            {Language::SPANISH, "Cambiar espacios de trabajo"},
            {Language::INDONESIAN, "Beralih ruang kerja"}
        };

        translations["workspaces_viewer"] = {
            {Language::ENGLISH, "Workspaces viewer Hyprspace"},
            {Language::JAPANESE, "ワークスペースビューアー Hyprspace"},
            {Language::CHINESE, "工作区查看器 Hyprspace"},
            {Language::VIETNAMESE, "Trình xem không gian làm việc Hyprspace"},
            {Language::RUSSIAN, "Просмотрщик рабочих пространств Hyprspace"},
            {Language::FRENCH, "Visualiseur d'espaces de travail Hyprspace"},
            {Language::SPANISH, "Visor de espacios de trabajo Hyprspace"},
            {Language::INDONESIAN, "Penampil ruang kerja Hyprspace"}
        };

        translations["notification"] = {
            {Language::ENGLISH, "Opens Swaync Notification"},
            {Language::JAPANESE, "Swaync通知を開く"},
            {Language::CHINESE, "打开Swaync通知"},
            {Language::VIETNAMESE, "Mở thông báo Swaync"},
            {Language::RUSSIAN, "Открыть уведомления Swaync"},
            {Language::FRENCH, "Ouvrir les notifications Swaync"},
            {Language::SPANISH, "Abrir notificaciones Swaync"},
            {Language::INDONESIAN, "Buka notifikasi Swaync"}
        };

        translations["system_info_widget"] = {
            {Language::ENGLISH, "EWW Widget for system info"},
            {Language::JAPANESE, "システム情報のEWWウィジェット"},
            {Language::CHINESE, "系统信息EWW小部件"},
            {Language::VIETNAMESE, "Widget EWW cho thông tin hệ thống"},
            {Language::RUSSIAN, "EWW виджет для системной информации"},
            {Language::FRENCH, "Widget EWW pour les infos système"},
            {Language::SPANISH, "Widget EWW para información del sistema"},
            {Language::INDONESIAN, "Widget EWW untuk info sistem"}
        };

        translations["wallpapers_menu"] = {
            {Language::ENGLISH, "Launches Wallpapers menu"},
            {Language::JAPANESE, "壁紙メニューを起動"},
            {Language::CHINESE, "启动壁纸菜单"},
            {Language::VIETNAMESE, "Khởi động menu hình nền"},
            {Language::RUSSIAN, "Запустить меню обоев"},
            {Language::FRENCH, "Lancer le menu des fonds d'écran"},
            {Language::SPANISH, "Abrir menú de fondos de pantalla"},
            {Language::INDONESIAN, "Luncurkan menu wallpaper"}
        };

        translations["exit_hyprland"] = {
            {Language::ENGLISH, "Exit Hyprland altogether"},
            {Language::JAPANESE, "Hyprlandを完全に終了"},
            {Language::CHINESE, "完全退出Hyprland"},
            {Language::VIETNAMESE, "Thoát hoàn toàn khỏi Hyprland"},
            {Language::RUSSIAN, "Полностью выйти из Hyprland"},
            {Language::FRENCH, "Quitter complètement Hyprland"},
            {Language::SPANISH, "Salir completamente de Hyprland"},
            {Language::INDONESIAN, "Keluar sepenuhnya dari Hyprland"}
        };

        translations["toggle_float"] = {
            {Language::ENGLISH, "Toggle float a window"},
            {Language::JAPANESE, "ウィンドウのフロート切り替え"},
            {Language::CHINESE, "切换窗口浮动"},
            {Language::VIETNAMESE, "Chuyển đổi cửa sổ nổi"},
            {Language::RUSSIAN, "Переключить плавающее окно"},
            {Language::FRENCH, "Basculer une fenêtre flottante"},
            {Language::SPANISH, "Alternar ventana flotante"},
            {Language::INDONESIAN, "Alihkan jendela mengambang"}
        };

        translations["text_editor"] = {
            {Language::ENGLISH, "Launch text editor VSCODE"},
            {Language::JAPANESE, "テキストエディタVSCODEを起動"},
            {Language::CHINESE, "启动文本编辑器VSCODE"},
            {Language::VIETNAMESE, "Khởi động trình soạn thảo văn bản VSCODE"},
            {Language::RUSSIAN, "Запустить текстовый редактор VSCODE"},
            {Language::FRENCH, "Lancer l'éditeur de texte VSCODE"},
            {Language::SPANISH, "Abrir editor de texto VSCODE"},
            {Language::INDONESIAN, "Luncurkan editor teks VSCODE"}
        };

        translations["file_manager"] = {
            {Language::ENGLISH, "Launch File manager Thunar"},
            {Language::JAPANESE, "ファイルマネージャーThunarを起動"},
            {Language::CHINESE, "启动文件管理器Thunar"},
            {Language::VIETNAMESE, "Khởi động trình quản lý tệp Thunar"},
            {Language::RUSSIAN, "Запустить файловый менеджер Thunar"},
            {Language::FRENCH, "Lancer le gestionnaire de fichiers Thunar"},
            {Language::SPANISH, "Abrir gestor de archivos Thunar"},
            {Language::INDONESIAN, "Luncurkan pengelola file Thunar"}
        };

        translations["browser"] = {
            {Language::ENGLISH, "Launch Floorp Browser"},
            {Language::JAPANESE, "FloORPブラウザを起動"},
            {Language::CHINESE, "启动Floorp浏览器"},
            {Language::VIETNAMESE, "Khởi động trình duyệt Floorp"},
            {Language::RUSSIAN, "Запустить браузер Floorp"},
            {Language::FRENCH, "Lancer le navigateur Floorp"},
            {Language::SPANISH, "Abrir navegador Floorp"},
            {Language::INDONESIAN, "Luncurkan browser Floorp"}
        };

        translations["full_screenshot"] = {
            {Language::ENGLISH, "Take a full screenshot"},
            {Language::JAPANESE, "フルスクリーンショットを撮る"},
            {Language::CHINESE, "截取全屏"},
            {Language::VIETNAMESE, "Chụp ảnh toàn màn hình"},
            {Language::RUSSIAN, "Сделать полный снимок экрана"},
            {Language::FRENCH, "Prendre une capture d'écran complète"},
            {Language::SPANISH, "Tomar captura de pantalla completa"},
            {Language::INDONESIAN, "Ambil tangkapan layar penuh"}
        };

        translations["region_screenshot"] = {
            {Language::ENGLISH, "Take a region screenshot"},
            {Language::JAPANESE, "領域スクリーンショットを撮る"},
            {Language::CHINESE, "截取区域"},
            {Language::VIETNAMESE, "Chụp ảnh vùng được chọn"},
            {Language::RUSSIAN, "Сделать снимок области"},
            {Language::FRENCH, "Prendre une capture de région"},
            {Language::SPANISH, "Tomar captura de región"},
            {Language::INDONESIAN, "Ambil tangkapan layar area"}
        };

        translations["mute_volume"] = {
            {Language::ENGLISH, "MUTE Volume"},
            {Language::JAPANESE, "音量ミュート"},
            {Language::CHINESE, "静音"},
            {Language::VIETNAMESE, "TẮT ÂM LƯỢNG"},
            {Language::RUSSIAN, "ВЫКЛЮЧИТЬ ЗВУК"},
            {Language::FRENCH, "COUPER LE VOLUME"},
            {Language::SPANISH, "SILENCIAR VOLUMEN"},
            {Language::INDONESIAN, "MATIKAN SUARA"}
        };

        translations["lower_brightness"] = {
            {Language::ENGLISH, "Lower Brightness"},
            {Language::JAPANESE, "明度を下げる"},
            {Language::CHINESE, "降低亮度"},
            {Language::VIETNAMESE, "Giảm độ sáng"},
            {Language::RUSSIAN, "Уменьшить яркость"},
            {Language::FRENCH, "Diminuer la luminosité"},
            {Language::SPANISH, "Reducir brillo"},
            {Language::INDONESIAN, "Turunkan kecerahan"}
        };

        translations["higher_brightness"] = {
            {Language::ENGLISH, "Higher Brightness"},
            {Language::JAPANESE, "明度を上げる"},
            {Language::CHINESE, "提高亮度"},
            {Language::VIETNAMESE, "Tăng độ sáng"},
            {Language::RUSSIAN, "Увеличить яркость"},
            {Language::FRENCH, "Augmenter la luminosité"},
            {Language::SPANISH, "Aumentar brillo"},
            {Language::INDONESIAN, "Tingkatkan kecerahan"}
        };

        translations["lower_volume"] = {
            {Language::ENGLISH, "Lower Volume"},
            {Language::JAPANESE, "音量を下げる"},
            {Language::CHINESE, "降低音量"},
            {Language::VIETNAMESE, "Giảm âm lượng"},
            {Language::RUSSIAN, "Уменьшить громкость"},
            {Language::FRENCH, "Diminuer le volume"},
            {Language::SPANISH, "Reducir volumen"},
            {Language::INDONESIAN, "Turunkan volume"}
        };

        translations["higher_volume"] = {
            {Language::ENGLISH, "Higher Volume"},
            {Language::JAPANESE, "音量を上げる"},
            {Language::CHINESE, "提高音量"},
            {Language::VIETNAMESE, "Tăng âm lượng"},
            {Language::RUSSIAN, "Увеличить громкость"},
            {Language::FRENCH, "Augmenter le volume"},
            {Language::SPANISH, "Aumentar volumen"},
            {Language::INDONESIAN, "Tingkatkan volume"}
        };

        translations["mute_microphone"] = {
            {Language::ENGLISH, "MUTE Microphone"},
            {Language::JAPANESE, "マイクミュート"},
            {Language::CHINESE, "静音麦克风"},
            {Language::VIETNAMESE, "TẮT MICROPHONE"},
            {Language::RUSSIAN, "ВЫКЛЮЧИТЬ МИКРОФОН"},
            {Language::FRENCH, "COUPER LE MICROPHONE"},
            {Language::SPANISH, "SILENCIAR MICRÓFONO"},
            {Language::INDONESIAN, "MATIKAN MIKROFON"}
        };

        // Tips and footer
        translations["tip_message"] = {
            {Language::ENGLISH, "💡 Keep this open for quick reference!"},
            {Language::JAPANESE, "💡 クイックリファレンス用に開いたままにしてください！"},
            {Language::CHINESE, "💡 保持打开以便快速参考！"},
            {Language::VIETNAMESE, "💡 Giữ mở để tham khảo nhanh!"},
            {Language::RUSSIAN, "💡 Держите открытым для быстрого доступа!"},
            {Language::FRENCH, "💡 Gardez ouvert pour référence rapide !"},
            {Language::SPANISH, "💡 ¡Mantén abierto para referencia rápida!"},
            {Language::INDONESIAN, "💡 Tetap buka untuk referensi cepat!"}
        };

        translations["copyright"] = {
            {Language::ENGLISH, "TM and © 2025 ElysiaOS.\nAll Rights Reserved."},
            {Language::JAPANESE, "TM and © 2025 ElysiaOS.\nAll Rights Reserved."},
            {Language::CHINESE, "TM and © 2025 ElysiaOS.\nAll Rights Reserved."},
            {Language::VIETNAMESE, "TM and © 2025 ElysiaOS.\nAll Rights Reserved."},
            {Language::RUSSIAN, "TM and © 2025 ElysiaOS.\nAll Rights Reserved."},
            {Language::FRENCH, "TM and © 2025 ElysiaOS.\nAll Rights Reserved."},
            {Language::SPANISH, "TM and © 2025 ElysiaOS.\nAll Rights Reserved."},
            {Language::INDONESIAN, "TM and © 2025 ElysiaOS.\nAll Rights Reserved."}
        };

        translations["unknown_cpu"] = {
            {Language::ENGLISH, "Unknown CPU"},
            {Language::JAPANESE, "不明なCPU"},
            {Language::CHINESE, "未知处理器"},
            {Language::VIETNAMESE, "CPU không xác định"},
            {Language::RUSSIAN, "Неизвестный процессор"},
            {Language::FRENCH, "CPU inconnu"},
            {Language::SPANISH, "CPU desconocido"},
            {Language::INDONESIAN, "CPU tidak dikenal"}
        };

        translations["unknown_gpu"] = {
            {Language::ENGLISH, "Unknown GPU"},
            {Language::JAPANESE, "不明なGPU"},
            {Language::CHINESE, "未知显卡"},
            {Language::VIETNAMESE, "GPU không xác định"},
            {Language::RUSSIAN, "Неизвестная видеокарта"},
            {Language::FRENCH, "GPU inconnu"},
            {Language::SPANISH, "GPU desconocido"},
            {Language::INDONESIAN, "GPU tidak dikenal"}
        };

        translations["unknown_linux"] = {
            {Language::ENGLISH, "Unknown Linux"},
            {Language::JAPANESE, "不明なLinux"},
            {Language::CHINESE, "未知Linux"},
            {Language::VIETNAMESE, "Linux không xác định"},
            {Language::RUSSIAN, "Неизвестный Linux"},
            {Language::FRENCH, "Linux inconnu"},
            {Language::SPANISH, "Linux desconocido"},
            {Language::INDONESIAN, "Linux tidak dikenal"}
        };

        translations["unknown_pc"] = {
            {Language::ENGLISH, "Unknown PC"},
            {Language::JAPANESE, "不明なPC"},
            {Language::CHINESE, "未知电脑"},
            {Language::VIETNAMESE, "PC không xác định"},
            {Language::RUSSIAN, "Неизвестный ПК"},
            {Language::FRENCH, "PC inconnu"},
            {Language::SPANISH, "PC desconocido"},
            {Language::INDONESIAN, "PC tidak dikenal"}
        };

        translations["unknown_os"] = {
            {Language::ENGLISH, "Unknown OS"},
            {Language::JAPANESE, "不明なOS"},
            {Language::CHINESE, "未知操作系统"},
            {Language::VIETNAMESE, "Hệ điều hành không xác định"},
            {Language::RUSSIAN, "Неизвестная ОС"},
            {Language::FRENCH, "OS inconnu"},
            {Language::SPANISH, "SO desconocido"},
            {Language::INDONESIAN, "OS tidak dikenal"}
        };
    }

    Language detectLanguage() {
        const char* lang = std::getenv("LANG");
        if (lang == nullptr) {
            return Language::ENGLISH; // Default fallback
        }

        std::string langStr(lang);
        
        // Convert to lowercase for easier comparison
        std::transform(langStr.begin(), langStr.end(), langStr.begin(), ::tolower);

        if (langStr.find("ja") == 0) {
            return Language::JAPANESE;
        } else if (langStr.find("zh") == 0 || langStr.find("cn") != std::string::npos) {
            return Language::CHINESE;
        } else if (langStr.find("vi") == 0) {
            return Language::VIETNAMESE;
        } else if (langStr.find("ru") == 0) {
            return Language::RUSSIAN;
        } else if (langStr.find("fr") == 0) {
            return Language::FRENCH;
        } else if (langStr.find("es") == 0) {
            return Language::SPANISH;
        } else if (langStr.find("id") == 0) {
            return Language::INDONESIAN;
        } else {
            return Language::ENGLISH; // Default fallback
        }
    }

public:
    Translations() {
        initializeTranslations();
        currentLanguage = detectLanguage();
    }

    std::string get(const std::string& key) const {
        auto it = translations.find(key);
        if (it != translations.end()) {
            auto langIt = it->second.find(currentLanguage);
            if (langIt != it->second.end()) {
                return langIt->second;
            }
            // Fallback to English if current language not found
            auto englishIt = it->second.find(Language::ENGLISH);
            if (englishIt != it->second.end()) {
                return englishIt->second;
            }
        }
        // Ultimate fallback - return the key itself
        return key;
    }

    Language getCurrentLanguage() const {
        return currentLanguage;
    }

    void setLanguage(Language lang) {
        currentLanguage = lang;
    }
};

#endif // TRANSLATIONS_H
