#[derive(Debug, Clone, Copy, PartialEq)]
pub struct WindowRect {
    pub x: f32,
    pub y: f32,
    pub width: f32,
    pub height: f32,
}

pub trait WindowTracker {
    fn get_window_rect(&mut self, title: &str) -> Option<WindowRect>;
    fn get_window_process_path(&mut self, title: &str) -> Option<String>;
}

pub fn create_tracker() -> Box<dyn WindowTracker> {
    #[cfg(target_os = "windows")]
    {
        Box::new(windows_tracker::WindowsTracker::new())
    }
    #[cfg(target_os = "linux")]
    {
        Box::new(linux_tracker::LinuxTracker::new())
    }
    #[cfg(not(any(target_os = "windows", target_os = "linux")))]
    {
        Box::new(dummy_tracker::DummyTracker::new())
    }
}

#[cfg(target_os = "windows")]
mod windows_tracker {
    use super::{WindowRect, WindowTracker};
    use std::ffi::CString;
    use windows::Win32::Foundation::{RECT, MAX_PATH};
    use windows::Win32::UI::WindowsAndMessaging::{FindWindowA, GetWindowRect, GetWindowThreadProcessId};
    use windows::Win32::System::Threading::{OpenProcess, PROCESS_QUERY_LIMITED_INFORMATION, QueryFullProcessImageNameW};
    use windows::core::PCSTR;
    use std::os::windows::ffi::OsStringExt;

    pub struct WindowsTracker {}

    impl WindowsTracker {
        pub fn new() -> Self {
            Self {}
        }
    }

    impl WindowTracker for WindowsTracker {
        fn get_window_rect(&mut self, title: &str) -> Option<WindowRect> {
            let c_title = CString::new(title).ok()?;
            let hwnd = unsafe { FindWindowA(None, PCSTR(c_title.as_ptr() as *const u8)) };

            let mut rect = RECT::default();
            if unsafe { GetWindowRect(hwnd, &mut rect) }.is_ok() {
                let width = (rect.right - rect.left) as f32;
                let height = (rect.bottom - rect.top) as f32;

                if width > 0.0 && height > 0.0 {
                    return Some(WindowRect {
                        x: rect.left as f32,
                        y: rect.top as f32,
                        width,
                        height,
                    });
                }
            }
            None
        }

        fn get_window_process_path(&mut self, title: &str) -> Option<String> {
            let c_title = CString::new(title).ok()?;
            let hwnd = unsafe { FindWindowA(None, PCSTR(c_title.as_ptr() as *const u8)) };
            if hwnd.0 == 0 {
                return None;
            }

            let mut process_id = 0;
            unsafe { GetWindowThreadProcessId(hwnd, Some(&mut process_id)) };
            if process_id == 0 {
                return None;
            }

            let handle = unsafe { OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, process_id) }.ok()?;
            
            let mut buffer = [0u16; MAX_PATH as usize];
            let mut size = buffer.len() as u32;
            
            let success = unsafe { QueryFullProcessImageNameW(handle, windows::Win32::System::Threading::PROCESS_NAME_FORMAT(0), windows::core::PWSTR(buffer.as_mut_ptr()), &mut size) };
            if success.is_ok() {
                let os_string = std::ffi::OsString::from_wide(&buffer[..size as usize]);
                return os_string.into_string().ok();
            }

            None
        }
    }
}

#[cfg(target_os = "linux")]
mod linux_tracker {
    use super::{WindowRect, WindowTracker};
    use x11rb::connection::Connection;
    use x11rb::protocol::xproto::{AtomEnum, ConnectionExt, Window};
    use x11rb::rust_connection::RustConnection;

    pub struct LinuxTracker {
        conn: Option<RustConnection>,
    }

    impl LinuxTracker {
        pub fn new() -> Self {
            let conn = match x11rb::connect(None) {
                Ok((conn, _)) => Some(conn),
                Err(_) => None,
            };
            Self { conn }
        }

        fn get_window_name(&self, conn: &RustConnection, window: Window) -> Option<String> {
            let cookie = conn
                .get_property(
                    false,
                    window,
                    AtomEnum::WM_NAME.into(),
                    AtomEnum::STRING.into(),
                    0,
                    1024,
                )
                .ok()?;
            let reply = cookie.reply().ok()?;
            String::from_utf8(reply.value).ok()
        }

        fn find_window_by_title(
            &self,
            conn: &RustConnection,
            root: Window,
            title: &str,
        ) -> Option<Window> {
            let tree = conn.query_tree(root).ok()?.reply().ok()?;

            for child in tree.children {
                if let Some(name) = self.get_window_name(conn, child) {
                    if name.contains(title) {
                        return Some(child);
                    }
                }

                if let Some(found) = self.find_window_by_title(conn, child, title) {
                    return Some(found);
                }
            }
            None
        }
    }

    impl WindowTracker for LinuxTracker {
        fn get_window_rect(&mut self, title: &str) -> Option<WindowRect> {
            let conn = self.conn.as_ref()?;
            let setup = conn.setup();
            let root = setup.roots[0].root;

            if let Some(window) = self.find_window_by_title(conn, root, title) {
                if let Ok(geom_cookie) = conn.get_geometry(window) {
                    if let Ok(geom) = geom_cookie.reply() {
                        if let Ok(trans_cookie) = conn.translate_coordinates(window, root, 0, 0) {
                            if let Ok(trans) = trans_cookie.reply() {
                                return Some(WindowRect {
                                    x: trans.dst_x as f32,
                                    y: trans.dst_y as f32,
                                    width: geom.width as f32,
                                    height: geom.height as f32,
                                });
                            }
                        }
                    }
                }
            }
            None
        }

        fn get_window_process_path(&mut self, _title: &str) -> Option<String> {
            None
        }
    }
}

#[cfg(not(any(target_os = "windows", target_os = "linux")))]
mod dummy_tracker {
    use super::{WindowRect, WindowTracker};

    pub struct DummyTracker {}
    impl DummyTracker {
        pub fn new() -> Self {
            Self {}
        }
    }
    impl WindowTracker for DummyTracker {
        fn get_window_rect(&mut self, _title: &str) -> Option<WindowRect> {
            None
        }
        fn get_window_process_path(&mut self, _title: &str) -> Option<String> {
            None
        }
    }
}
