mod config;
mod tracker;

use egui_overlay::{EguiOverlay, start};
use tracker::{create_tracker, WindowTracker};
use config::{load_config, AppConfig};

struct OverlayApp {
    tracker: Box<dyn WindowTracker>,
    config: AppConfig,
}

impl OverlayApp {
    fn new(config: AppConfig) -> Self {
        Self {
            tracker: create_tracker(),
            config,
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
        let mut found_game = false;
        if let Some(rect) = self.tracker.get_window_rect("Path of Exile") {
            found_game = true;
            glfw_backend.window.set_pos(rect.x as i32, rect.y as i32);
            glfw_backend.window.set_size(rect.width as i32, rect.height as i32);
        } else {
            glfw_backend.window.set_pos(0, 0);
            glfw_backend.window.set_size(1920, 1080);
        }

        // Enable passthrough on the underlying GLFW window natively
        glfw_backend.window.set_mouse_passthrough(true);
        // We can request a repaint if needed, but egui_overlay does this continuously if transparent
        egui_context.request_repaint();

        egui::CentralPanel::default()
            .frame(egui::Frame::none().fill(egui::Color32::TRANSPARENT))
            .show(egui_context, |ui| {
                let screen_rect = egui_context.screen_rect();
                let text = if found_game { "TEST" } else { "WAITING FOR POE..." };
                ui.painter().text(
                    egui::pos2(screen_rect.max.x - 40.0, screen_rect.min.y + 40.0),
                    egui::Align2::RIGHT_TOP,
                    text,
                    egui::FontId::proportional(40.0),
                    egui::Color32::RED,
                );
            });
    }
}

fn main() {
    let config = load_config();
    let app = OverlayApp::new(config);
    start(app);
}
