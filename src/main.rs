#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use learn_to_play_poe1::config::{load_config, save_config, AppConfig};
use learn_to_play_poe1::tracker::{create_tracker, WindowTracker};
use tray_icon::{Icon, TrayIconBuilder, TrayIcon};
use tray_icon::menu::{Menu, MenuItem};
use eframe::egui;
use std::process::Child;
use std::io::Write;
use std::time::Instant;
use std::sync::{Arc, Mutex};

struct AppState {
    show_main_window: bool,
    show_settings: bool,
    exit_requested: bool,
}

struct MainApp {
    config: AppConfig,
    _tray_icon: Option<TrayIcon>,
    overlay_process: Option<Child>,
    show_main_window: bool,
    show_settings: bool,
    tracker: Box<dyn WindowTracker>,
    last_tracker_check: Instant,
    overlay_exe: std::path::PathBuf,
    was_visible: Option<bool>,
    state: Arc<Mutex<AppState>>,
    log_messages: String,
}

impl MainApp {
    fn new(cc: &eframe::CreationContext<'_>, config: AppConfig) -> Self {
        let tray_menu = Menu::new();
        let open_i = MenuItem::new("Open", true, None);
        let open_id = open_i.id().clone();
        let settings_i = MenuItem::new("Settings", true, None);
        let settings_id = settings_i.id().clone();
        let quit_i = MenuItem::new("Exit", true, None);
        let quit_id = quit_i.id().clone();
        
        tray_menu.append(&open_i).unwrap();
        tray_menu.append(&settings_i).unwrap();
        tray_menu.append(&quit_i).unwrap();

        let icon_bytes = include_bytes!("../assets/logo/vertex-icon.png");
        let image = image::load_from_memory(icon_bytes).unwrap().into_rgba8();
        let (width, height) = image.dimensions();
        let icon_rgba = image.into_raw();
        let icon = Icon::from_rgba(icon_rgba, width, height).unwrap();

        let tray_icon = TrayIconBuilder::new()
            .with_menu(Box::new(tray_menu))
            .with_menu_on_left_click(false)
            .with_tooltip("Learn to Play: Path of Exile 1")
            .with_icon(icon)
            .build()
            .unwrap();

        let mut overlay_exe = std::env::current_exe().unwrap_or_else(|_| std::path::PathBuf::from("."));
        overlay_exe.pop(); 
        
        #[cfg(windows)]
        overlay_exe.push("l2p-poe1-overlay.exe");
        #[cfg(not(windows))]
        overlay_exe.push("l2p-poe1-overlay");

        let start_minimized = config.start_minimized;
        let state = Arc::new(Mutex::new(AppState {
            show_main_window: !start_minimized,
            show_settings: false,
            exit_requested: false,
        }));

        let state_clone = state.clone();
        let ctx_clone = cc.egui_ctx.clone();

        std::thread::spawn(move || {
            let menu_channel = tray_icon::menu::MenuEvent::receiver();
            let tray_channel = tray_icon::TrayIconEvent::receiver();
            println!("Settings ID: {:?}, Quit ID: {:?}", settings_id, quit_id);
            loop {
                let mut changed = false;
                while let Ok(event) = menu_channel.try_recv() {
                    println!("Menu event: {:?}", event.id);
                    let mut st = state_clone.lock().unwrap();
                    if event.id == quit_id {
                        println!("Matched Quit ID!");
                        st.exit_requested = true;
                        changed = true;
                    } else if event.id == open_id {
                        println!("Matched Open ID!");
                        st.show_main_window = true;
                        changed = true;
                    } else if event.id == settings_id {
                        println!("Matched Settings ID!");
                        st.show_main_window = true;
                        st.show_settings = true;
                        changed = true;
                    }
                }
                while let Ok(event) = tray_channel.try_recv() {
                    if let tray_icon::TrayIconEvent::Click { button: tray_icon::MouseButton::Left, .. } = event {
                        let mut st = state_clone.lock().unwrap();
                        st.show_main_window = true;
                        changed = true;
                    }
                }
                if changed {
                    let st = state_clone.lock().unwrap();
                    let _is_visible = st.show_main_window || st.show_settings;
                    
                    if st.exit_requested {
                        std::process::exit(0);
                    }
                    ctx_clone.request_repaint();
                }
                std::thread::sleep(std::time::Duration::from_millis(50));
            }
        });

        Self {
            config,
            _tray_icon: Some(tray_icon),
            overlay_process: None,
            show_main_window: !start_minimized,
            show_settings: false,
            tracker: create_tracker(),
            last_tracker_check: Instant::now() - std::time::Duration::from_secs(10), // force immediate check
            overlay_exe,
            was_visible: None,
            state,
            log_messages: format!("[{}] Application started.\n", chrono::Local::now().format("%H:%M")),
        }
    }

    pub fn log_message(&mut self, msg: &str) {
        let time_str = chrono::Local::now().format("%H:%M").to_string();
        self.log_messages.push_str(&format!("[{}] {}\n", time_str, msg));
    }

    fn sync_config(&mut self) {
        save_config(&self.config);
        let msg = serde_json::to_string(&self.config).unwrap();
        if let Some(child) = self.overlay_process.as_mut() {
            if let Some(stdin) = child.stdin.as_mut() {
                let _ = writeln!(stdin, "{}", msg);
            }
        }
    }
}

impl eframe::App for MainApp {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        let st = self.state.lock().unwrap();
        if st.exit_requested {
            self._tray_icon.take(); // force drop tray icon
            std::process::exit(0);
        }
        self.show_main_window = st.show_main_window;
        self.show_settings = st.show_settings;
        drop(st);

        if ctx.input(|i| i.viewport().close_requested()) {
            if self.config.minimize_to_tray {
                ctx.send_viewport_cmd(egui::ViewportCommand::CancelClose);
                self.show_main_window = false;
                self.show_settings = false;
                let mut st = self.state.lock().unwrap();
                st.show_main_window = false;
                st.show_settings = false;
            } else {
                let mut st = self.state.lock().unwrap();
                st.exit_requested = true;
            }
        }

        let is_visible = self.show_main_window || self.show_settings;

        if self.was_visible != Some(is_visible) {
            ctx.send_viewport_cmd(egui::ViewportCommand::Visible(is_visible));
            if is_visible {
                ctx.send_viewport_cmd(egui::ViewportCommand::Focus);
            }
            self.was_visible = Some(is_visible);
        }

        // ALWAYS draw CentralPanel so the window is never black if it's visible
        egui::CentralPanel::default().show(ctx, |ui| {
            egui::menu::bar(ui, |ui| {
                ui.menu_button("File", |ui| {
                    if ui.button("Settings").clicked() {
                        self.show_settings = true;
                        ui.close_menu();
                    }
                });
            });
            egui::ScrollArea::vertical()
                .auto_shrink([false; 2])
                .stick_to_bottom(true)
                .show(ui, |ui| {
                    ui.add_sized(
                        ui.available_size(),
                        egui::TextEdit::multiline(&mut self.log_messages)
                            .interactive(false)
                            .frame(false)
                            .font(egui::TextStyle::Monospace),
                    );
                });
        });

        if self.show_settings {
            let mut close_settings = false;
            let mut config_changed = false;
            
            egui::Window::new("Settings")
                .open(&mut self.show_settings)
                .show(ctx, |ui| {
                    if ui.checkbox(&mut self.config.use_game_overlay, "Use game overlay").changed() {
                        config_changed = true;
                    }
                    if ui.checkbox(&mut self.config.auto_detect_install_dir, "Auto detect game installDir from launch").changed() {
                        config_changed = true;
                    }
                    ui.horizontal(|ui| {
                        ui.label("Install Directory:");
                        ui.add_enabled_ui(!self.config.auto_detect_install_dir, |ui| {
                            if ui.text_edit_singleline(&mut self.config.install_dir).changed() {
                                config_changed = true;
                            }
                        });
                    });
                    ui.horizontal(|ui| {
                        ui.label("Windows Executable:");
                        if ui.text_edit_singleline(&mut self.config.windows_executable_name).changed() {
                            config_changed = true;
                        }
                    });
                    ui.horizontal(|ui| {
                        ui.label("Linux Executable:");
                        if ui.text_edit_singleline(&mut self.config.linux_executable_name).changed() {
                            config_changed = true;
                        }
                    });
                    ui.separator();
                    ui.add_enabled_ui(false, |ui| {
                        if ui.checkbox(&mut self.config.auto_start_on_boot, "Automatically open when computer starts up (Coming soon)").changed() {
                            config_changed = true;
                        }
                    });
                    if ui.checkbox(&mut self.config.start_minimized, "Start minimized").changed() {
                        config_changed = true;
                    }
                    if ui.checkbox(&mut self.config.minimize_to_tray, "Minimize to system tray on close").changed() {
                        config_changed = true;
                    }
                });
                
            if !self.show_settings {
                close_settings = true;
            }
            if config_changed || close_settings {
                self.sync_config();
                // Force an immediate tracker check if settings changed (e.g. toggled off)
                self.last_tracker_check = Instant::now() - std::time::Duration::from_secs(10);
            }
        }

        // Write state back
        let mut st = self.state.lock().unwrap();
        st.show_main_window = self.show_main_window;
        st.show_settings = self.show_settings;
        drop(st);

        // Check game window every 1 second
        if self.last_tracker_check.elapsed() > std::time::Duration::from_secs(1) {
            self.last_tracker_check = Instant::now();
            let executable_name = if cfg!(windows) {
                &self.config.windows_executable_name
            } else {
                &self.config.linux_executable_name
            };

            let game_found = self.tracker.get_window_rect(executable_name).is_some();

            if game_found && self.config.auto_detect_install_dir {
                if let Some(path) = self.tracker.get_window_process_path(executable_name) {
                    if let Some(dir) = std::path::Path::new(&path).parent() {
                        let dir_str = dir.to_string_lossy().to_string();
                        if self.config.install_dir != dir_str {
                            self.config.install_dir = dir_str.clone();
                            self.log_message(&format!("Auto-detected game install directory: {}", dir_str));
                            self.sync_config();
                        }
                    }
                }
            }

            // Overlay launching temporarily disabled while focusing on the regular GUI
            /*
            if _game_found && self.config.use_game_overlay {
                if self.overlay_process.is_none() {
                    let mut child = Command::new(&self.overlay_exe)
                        .stdin(Stdio::piped())
                        .spawn()
                        .unwrap_or_else(|e| panic!("Failed to start overlay process at {:?}: {}", self.overlay_exe, e));
                    
                    let msg = serde_json::to_string(&self.config).unwrap();
                    if let Some(stdin) = child.stdin.as_mut() {
                        let _ = writeln!(stdin, "{}", msg);
                    }
                    self.overlay_process = Some(child);
                }
            } else {
                if let Some(mut child) = self.overlay_process.take() {
                    let _ = child.kill();
                    let _ = child.wait();
                }
            }
            */
        }

        // We do NOT need to ctx.request_repaint() constantly anymore!
        // The background thread will wake us up when a tray event happens.
        // This makes the app consume 0% CPU when minimized!
    }
}

impl Drop for MainApp {
    fn drop(&mut self) {
        if let Some(mut child) = self.overlay_process.take() {
            let _ = child.kill();
        }
    }
}

fn main() -> Result<(), eframe::Error> {
    // Must be set before winit's EventLoop::new() claims it — Windows only honors
    // the first call per process. SYSTEM_AWARE prevents WM_DPICHANGED ping-pong
    // when the window is dragged across monitors with different DPI scales.
    #[cfg(windows)]
    unsafe {
        use windows::Win32::UI::HiDpi::{SetProcessDpiAwarenessContext, DPI_AWARENESS_CONTEXT_SYSTEM_AWARE};
        let _ = SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
    }

    let config = load_config();
    
    let icon_bytes = include_bytes!("../assets/logo/vertex-icon.png");
    let image = image::load_from_memory(icon_bytes).unwrap().into_rgba8();
    let (width, height) = image.dimensions();
    let icon_data = egui::IconData {
        rgba: image.into_raw(),
        width,
        height,
    };

    let options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default()
            // .with_taskbar(false) // Temporarily disabled: WS_EX_TOOLWINDOW causes infinite resize loop on cross-monitor drag in winit
            .with_title("Learn to Play: Path of Exile 1")
            .with_icon(Arc::new(icon_data))
            .with_visible(!config.start_minimized)
            .with_inner_size([600.0, 400.0]),
        run_and_return: false,
        ..Default::default()
    };

    eframe::run_native(
        "Learn to Play: Path of Exile 1",
        options,
        Box::new(|cc| Ok(Box::new(MainApp::new(cc, config)))),
    )
}
