<!-- .agents/AGENTS.md (markdown) -->

## Build Verification
Always run `just build` to verify compilation before completing a task and handing it back to the user. Do not simply rely on the user to run `just run`. 

## Version Control
Never proactively commit code or run `git commit`. Always wait for the user to explicitly instruct you to `commit` before staging or committing changes.
