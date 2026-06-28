## Build Verification
Always run `just build` to verify compilation before completing a task and handing it back to the user. Do not simply rely on the user to run `just run`. 

If the build fails with an error indicating that `l2p-poe1.exe` is being used by another process, it is likely because the user currently has `just run` active. In this case, politely ask the user to stop their running instance so the build can complete successfully.
