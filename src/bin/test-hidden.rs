use eframe::egui;

struct TestApp {
    hidden: bool,
    hidden_time: Option<std::time::Instant>,
}

impl eframe::App for TestApp {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        println!("Update called! hidden: {}", self.hidden);
        
        if !self.hidden {
            if let Some(t) = self.hidden_time {
                if t.elapsed().as_secs() > 2 {
                    println!("Moving viewport offscreen...");
                    ctx.send_viewport_cmd(egui::ViewportCommand::OuterPosition(egui::pos2(-10000.0, -10000.0)));
                    self.hidden = true;

                    let ctx_clone = ctx.clone();
                    std::thread::spawn(move || {
                        std::thread::sleep(std::time::Duration::from_secs(3));
                        println!("Thread: moving viewport back and requesting repaint...");
                        ctx_clone.send_viewport_cmd(egui::ViewportCommand::OuterPosition(egui::pos2(100.0, 100.0)));
                        ctx_clone.request_repaint();
                    });
                }
            } else {
                self.hidden_time = Some(std::time::Instant::now());
            }
        } else {
            // hidden
        }

        egui::CentralPanel::default().show(ctx, |ui| {
            ui.label("Hello from the test app!");
        });
    }
}

fn main() {
    let options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default()
            .with_taskbar(false)
            .with_always_on_top(),
        ..Default::default()
    };
    eframe::run_native("Test", options, Box::new(|_| Ok(Box::new(TestApp { hidden: false, hidden_time: None })))).unwrap();
}
