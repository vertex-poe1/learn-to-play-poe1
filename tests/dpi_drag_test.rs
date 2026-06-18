use std::ffi::CString;
use std::process::Command;
use std::time::Duration;

#[cfg(windows)]
#[test]
fn test_cross_monitor_dpi_scaling_stability() {
    use windows::Win32::Foundation::{BOOL, LPARAM, RECT};
    use windows::Win32::Graphics::Gdi::{EnumDisplayMonitors, HDC, HMONITOR};
    use windows::Win32::UI::WindowsAndMessaging::{FindWindowA, GetWindowRect, SetCursorPos};
    use windows::Win32::UI::Input::KeyboardAndMouse::{
        mouse_event, MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP,
    };

    // Callback to gather all monitors
    unsafe extern "system" fn monitor_enum_proc(
        _hmonitor: HMONITOR,
        _hdc: HDC,
        lprcmonitor: *mut RECT,
        dwdata: LPARAM,
    ) -> BOOL {
        unsafe {
            let monitors = &mut *(dwdata.0 as *mut Vec<RECT>);
            monitors.push(*lprcmonitor);
        }
        true.into()
    }

    // 1. Gather all monitors
    let mut monitors: Vec<RECT> = Vec::new();
    unsafe {
        let _ = EnumDisplayMonitors(
            HDC::default(),
            None,
            Some(monitor_enum_proc),
            LPARAM(&mut monitors as *mut _ as isize),
        );
    }

    if monitors.len() < 2 {
        println!("Test requires at least 2 monitors. Skipping test.");
        return;
    }

    println!("Detected {} monitors.", monitors.len());

    // 2. Spawn the app in a background process
    let exe = env!("CARGO_BIN_EXE_learn-to-play-poe1");
    let mut child = Command::new(exe)
        .spawn()
        .expect("Failed to start the application");

    // Give it time to start and render the window
    std::thread::sleep(Duration::from_secs(3));

    let title = CString::new("Learn to Play: Path of Exile 1").unwrap();
    let hwnd = unsafe { FindWindowA(None, windows::core::PCSTR(title.as_ptr() as *const u8)) };

    if hwnd.0 == 0 {
        let _ = child.kill();
        panic!("Window not found! Make sure the title matches exactly.");
    }

    // 3. Get initial size and position
    let mut current_rect = RECT::default();
    let _ = unsafe { GetWindowRect(hwnd, &mut current_rect) };

    let win_center_x = current_rect.left + (current_rect.right - current_rect.left) / 2;
    let win_center_y = current_rect.top + (current_rect.bottom - current_rect.top) / 2;

    // Find which monitor the window is on
    let mut current_monitor_idx = 0;
    for (i, m) in monitors.iter().enumerate() {
        if win_center_x >= m.left
            && win_center_x <= m.right
            && win_center_y >= m.top
            && win_center_y <= m.bottom
        {
            current_monitor_idx = i;
            break;
        }
    }

    println!("Window spawned on monitor {}.", current_monitor_idx);

    // Pick another monitor
    let target_monitor_idx = if current_monitor_idx == 0 { 1 } else { 0 };
    let target_monitor = monitors[target_monitor_idx];

    let window_width = current_rect.right - current_rect.left;
    let window_height = current_rect.bottom - current_rect.top;

    let target_center_x = target_monitor.left + (target_monitor.right - target_monitor.left) / 2;
    let target_center_y = target_monitor.top + (target_monitor.bottom - target_monitor.top) / 2;

    // Plot course to exactly place all 4 edges of the window securely inside the target monitor
    let target_x = target_center_x - window_width / 2;
    let target_y = target_center_y - window_height / 2;

    println!(
        "Plotting course to monitor {} top-left: ({}, {})",
        target_monitor_idx, target_x, target_y
    );

    let start_x = current_rect.left;
    let start_y = current_rect.top;

    let steps = 150;
    let dx = (target_x - start_x) as f32 / steps as f32;
    let dy = (target_y - start_y) as f32 / steps as f32;

    let mut last_area = 0;
    let mut resize_ticks = 0;

    // RAII guard: ensures MOUSEEVENTF_LEFTUP is sent even if the test panics,
    // preventing the left button from staying stuck system-wide after the test.
    struct MouseReleaseGuard;
    impl Drop for MouseReleaseGuard {
        fn drop(&mut self) {
            unsafe {
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            }
        }
    }

    // Grab the window's title bar (offset +50, +10 from top left)
    unsafe {
        let _ = SetCursorPos(start_x + 50, start_y + 10);
        mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
    }
    let _mouse_guard = MouseReleaseGuard;
    std::thread::sleep(Duration::from_millis(50));

    // 4. Simulate a physical smooth drag across to the target monitor
    for step in 0..=steps {
        let current_x = (start_x as f32 + dx * step as f32) as i32;
        let current_y = (start_y as f32 + dy * step as f32) as i32;

        // We must use raw mouse inputs to trigger the OS modal drag loop (WM_ENTERSIZEMOVE)
        unsafe {
            // Move cursor to the new coordinate
            let _ = SetCursorPos(current_x, current_y);
        }

        // If we are halfway, pause for 1 second to check for ping-pong
        if step == steps / 2 {
            println!("Pausing mouse mid-drag on the boundary to check for ping-pong...");
            for _ in 0..20 {
                std::thread::sleep(Duration::from_millis(50));
                let _ = unsafe { GetWindowRect(hwnd, &mut current_rect) };
                let width = current_rect.right - current_rect.left;
                let height = current_rect.bottom - current_rect.top;
                let current_area = width * height;

                if current_area != last_area && last_area != 0 {
                    resize_ticks += 1;
                    println!("Window resized at boundary! New area: {}", current_area);
                }
                last_area = current_area;
            }
        } else {
            std::thread::sleep(Duration::from_millis(15));
        }

        // Measure the size during the drag
        let _ = unsafe { GetWindowRect(hwnd, &mut current_rect) };
        let width = current_rect.right - current_rect.left;
        let height = current_rect.bottom - current_rect.top;
        let current_area = width * height;

        if current_area != last_area && last_area != 0 {
            resize_ticks += 1;
            println!("Window resized! New area: {}", current_area);
        }
        last_area = current_area;
    }

    // Clean up before asserting (mouse button released by _mouse_guard drop)
    let _ = child.kill();

    assert!(
        resize_ticks <= 1,
        "Window resized multiple times during drag! Resized {} times while moving.",
        resize_ticks
    );
}
