use learn_to_play_poe1::tracker::{create_tracker, WindowTracker};
use learn_to_play_poe1::config::AppConfig;
use egui_overlay::{EguiOverlay, start};
use std::sync::{Arc, Mutex};
use std::io::{self, BufRead};

struct OverlayApp {
    tracker: Box<dyn WindowTracker>,
    config: Arc<Mutex<AppConfig>>,
    title_set: bool,
}

impl OverlayApp {
    fn new(config: Arc<Mutex<AppConfig>>) -> Self {
        Self {
            tracker: create_tracker(),
            config,
            title_set: false,
        }
    }
}

impl EguiOverlay for OverlayApp {
    fn gui_run(
        &mut self,
        egui_context: &egui::Context,
        _default_gfx_backend: &mut egui_overlay::egui_render_three_d::ThreeDBackend,
        glfw_backend: &mut egui_overlay::egui_window_glfw_passthrough::GlfwBackend,
    ) {
        if !self.title_set {
            glfw_backend.window.set_title("l2p-poe1 overlay");
            self.title_set = true;
        }

        // Overlay is ALWAYS 100% passthrough
        glfw_backend.window.set_mouse_passthrough(true);

        let config = self.config.lock().unwrap().clone();

        let executable_name = if cfg!(windows) {
            &config.windows_executable_name
        } else {
            &config.linux_executable_name
        };

        let mut found_game = false;
        if let Some(rect) = self.tracker.get_window_rect(executable_name) {
            found_game = true;
            glfw_backend.window.set_pos(rect.x as i32, rect.y as i32);
            glfw_backend.window.set_size(rect.width as i32, rect.height as i32);
        } else {
            glfw_backend.window.set_pos(0, 0);
            glfw_backend.window.set_size(1920, 1080);
        }

        // We request a repaint to ensure continuous updates if needed
        egui_context.request_repaint();

        // In-game Overlay
        if config.use_game_overlay && found_game {
            egui::CentralPanel::default()
                .frame(egui::Frame::none().fill(egui::Color32::TRANSPARENT))
                .show(egui_context, |ui| {
                    ui.painter().text(
                        egui::pos2(10.0, 10.0),
                        egui::Align2::LEFT_TOP,
                        "TEST",
                        egui::FontId::proportional(40.0),
                        egui::Color32::RED,
                    );
                });
        }
    }
}

fn main() {
    let config = Arc::new(Mutex::new(AppConfig::default()));
    
    // Read stdin in a background thread to update config via IPC
    let config_clone = config.clone();
    std::thread::spawn(move || {
        let stdin = io::stdin();
        for line in stdin.lock().lines() {
            if let Ok(line) = line {
                if let Ok(new_config) = serde_json::from_str::<AppConfig>(&line) {
                    *config_clone.lock().unwrap() = new_config;
                }
            } else {
                break; // stdin closed, main process died
            }
        }
        std::process::exit(0);
    });

    let app = OverlayApp::new(config);
    start(app);
}
