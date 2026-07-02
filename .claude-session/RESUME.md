# Resuming the Claude build session on another computer

This folder carries the state of the Claude Code session that built the game
(2026-07-01). Two ways to continue, from strongest to simplest:

## Option A — restore the actual session (full chat history)

Claude Code keys sessions to the project's absolute path. Clone the repo to the
SAME path as the original machine (`D:\UEprojects\BeastForgeKingdom`), then:

```powershell
# session store slug is the project path with separators replaced by '-'
$store = "$env:USERPROFILE\.claude\projects\D--UEprojects-BeastForgeKingdom"
New-Item -ItemType Directory -Force $store, "$store\memory" | Out-Null
Copy-Item .claude-session\transcript.jsonl "$store\35461404-6a06-4176-8db1-96ac51b332cf.jsonl"
Copy-Item .claude-session\memory\* "$store\memory\"
```

Launch `claude` inside the repo and use `--resume` (or `/resume`) to pick the
session. Claude will have the entire conversation history.

If you clone to a DIFFERENT path, adjust the slug: replace `\` and `:` in your
absolute project path with `-` (e.g. `C:\dev\BFK` -> `C--dev-BFK`) and copy the
files into that folder instead. The transcript resumes the same either way;
only the folder name must match the path Claude runs in.

## Option B — fresh session with full context (recommended for new work)

Just start `claude` in the repo. `CLAUDE.md` auto-loads and points to
`Docs/SESSION_HANDOFF.md`, which contains everything the original session knew:
architecture, decisions, gotchas, known issues, and the backlog. Copy
`.claude-session/memory/*` into the session store (as above) if you also want
the persistent memory recall.

## Before you build here

- UE 5.8 must be installed/associated (see `CLAUDE.md` -> Setup).
- To push to GitHub, recreate `Content/keys/githubKey.txt` (git-ignored):
  line 1 = a GitHub token for `tcodetitanx`, line 3 = `tcodetitanx`.
  The token itself is intentionally NOT in this repo.
