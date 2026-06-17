mod config;
mod tracker;

use eframe::egui;
use tracker::{create_tracker, WindowTracker};
use config::{load_config, AppConfig};

fn main() -> Result<(), eframe::Error> {
    let config = load_config();
    
    let options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default()
            .with_decorations(false)
            .with_transparent(true)
            .with_always_on_top()
            .with_mouse_passthrough(true),
        ..Default::default()
    };

    eframe::run_native(
        "POE Overlay",
        options,
        Box::new(move |_cc| Box::new(OverlayApp::new(config))),
    )
}

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

impl eframe::App for OverlayApp {
    fn clear_color(&self, _visuals: &egui::Visuals) -> [f32; 4] {
        [0.0, 0.0, 0.0, 0.0]
    }

    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        // We currently track by window title, but the config contains the executable name.
        // We can keep tracking by "Path of Exile" for now, or use the configured executable name
        // if the tracker is updated to support it in the future.
        if let Some(rect) = self.tracker.get_window_rect("Path of Exile") {
            ctx.send_viewport_cmd(egui::ViewportCommand::OuterPosition(egui::pos2(
                rect.x, rect.y,
            )));
            ctx.send_viewport_cmd(egui::ViewportCommand::InnerSize(egui::vec2(
                rect.width, rect.height,
            )));
        }

        egui::CentralPanel::default()
            .frame(egui::Frame::none().fill(egui::Color32::TRANSPARENT))
            .show(ctx, |ui| {
                ui.painter().text(
                    egui::pos2(20.0, 20.0),
                    egui::Align2::LEFT_TOP,
                    "POE Overlay Active",
                    egui::FontId::proportional(24.0),
                    egui::Color32::RED,
                );
            });

        ctx.request_repaint();
    }
}
