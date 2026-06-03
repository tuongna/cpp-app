# CLAUDE.md

Guidance for Claude Code when working in this repository. See `ARCHITECTURE.md`
for the full system design and `README.md` for build/run instructions.

## Pull request workflow (standing instruction)

Whenever you create a pull request in this repository, **immediately subscribe
to its activity** with `subscribe_pr_activity` and follow through until told to
stop. This applies in every session — do it automatically without being asked.

- **CI:** if a check fails, read the logs, diagnose the root cause, fix it, and
  push to the PR branch. Re-kick until green. If a failure is genuinely out of
  scope or you're stuck after a few attempts, report the diagnosis instead of
  going silent.
- **Review comments / discussion:** evaluate each one. Apply confident,
  low-risk, in-scope fixes and resolve the thread; ask first (via
  `AskUserQuestion`) if a comment is ambiguous or architecturally significant;
  skip silently if no action is needed.

Stop watching immediately when the user asks (`unsubscribe_pr_activity`), and
don't push further changes to that PR afterwards.

**Auto-unwatch on close:** when a watched PR is merged or closed, call
`unsubscribe_pr_activity` for it — there is no more CI or review activity worth
following once the branch is merged.
