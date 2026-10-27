# HexMG-Core optimized branch

User authorization, 2026-10-27:

- `main` (and any `master` branch) is owned exclusively by the user.
  Never commit, push, merge, reset or rewrite those branches for the user.
- Develop and commit on `codex/optimized-solver`; pushing this branch to
  `https://github.com/LaszloPohl/HexMG-Core.git` is expressly authorized.
  This supersedes the earlier blanket prohibition on solver pushes only
  for the optimized branch. Never push optimized work to the baseline.
- Keep baseline CLI/HMG/output compatibility. Optional optimization hints
  may be ignored by a baseline reader after its agreed skip support is added.
  Physics, excitation, boundary changes and required output declarations
  must remain completely specified in ordinary baseline-compatible HMG.
- Do not silently switch the GUI to this variant. Keep an untouched official
  checkout/binary for comparison, and record the baseline commit for tests.
- Bring new official changes into this branch deliberately, with regression
  checks. Do not modify the baseline as part of that synchronization.
- The optional HMG extension syntax is currently a proposal, not approved.
  The user requested a baseline skip-code excerpt only AFTER approving the
  format. Do not implement the parser extension before that approval.
- This worktree is source code even though its current path is under the
  GUI's build directory. Never delete it as regenerable build output.

See `doc/OPTIMIZED_VARIANT_HU.md` for the branch's initial state.
