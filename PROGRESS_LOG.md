# Nightlies — Progress Log

**Repo:** https://github.com/The412Banner/Nightlies
**Local path:** `/data/data/com.termux/files/home/Nightlies`
**Rules:** No pull requests ever. Log every change. Push commits as needed.

---

## Session — 2026-06-03

### [fix] — Rebase + vendor gplasync master patch (DXVK cc418519 broke it); fix build-dxvk checkout (2026-06-03)
**Commits:** `84d41ff` (patch rebase + vendor), `b38eb9b` (build-dxvk checkout)

#### Problem
All 4 DXVK gplasync jobs went red starting 2026-06-01. Root cause: DXVK upstream
commit `cc418519` ("[dxvk] Fix thread synchronization on pipeline compiles") rewrote
`DxvkGraphicsPipeline::getOptimizedPipeline` in `src/dxvk/dxvk_graphics.cpp`:
- `m_fastPipelines.insert({ key, handle })` → `m_fastPipelines.emplace(std::piecewise_construct, …)`
- single compile path → `if (entry.second) { …compile…; return handle; } else { …spin-wait… }`

Ph42oN's `dxvk-gplasync-master.patch` (upstream main, last touched 2025-12-29 — he had
NOT fixed it) anchors `m_async = false;` on the old `insert` block → `patch does not apply`
at `dxvk_graphics.cpp:1397`. Jobs clone DXVK `HEAD` (floating), so it broke live.

#### Fix
1. **Rebased the patch** — only one hunk changed: moved `m_async = false;` into the new
   `if (entry.second)` compile branch, right before its `return handle;`. Preserves the
   original semantics (reset on the actual-compile path only; cache-hit early-return does
   not reset). All other hunks + the binsem patch (touches cmdlist/queue, unaffected) apply
   clean. Verified `git apply --check` exit 0 on pristine DXVK HEAD `840d147`; binsem applies
   on top.
2. **Vendored** the rebased patch at `patches/dxvk-gplasync-master.patch` and switched all 4
   DXVK jobs from `curl …Ph42oN…|git apply` to `git apply "$GITHUB_WORKSPACE/patches/dxvk-gplasync-master.patch"`.
   Trade-off: no longer auto-tracks Ph42oN — **revert to curl once he rebases upstream main.**
3. `build-dxvk` (plain GPLAsync std job) had NO `actions/checkout` (it only ever curl'd the
   patch) → first re-run failed with `can't open patch … No such file or directory`. Added a
   `Checkout (for patch file)` step (the other 3 DXVK jobs already had one).

#### Verification
- Run `26873026129` — all 4 DXVK jobs green (compiled + packaged): Build DXVK (GPLAsync),
  Build DXVK BinSem (GPLAsync), Build DXVK (ARM64EC), Build DXVK BinSem (ARM64EC).
- Build-proven only; runtime async-correctness under the new sync model NOT device-tested.
- Filed-issue-on-Ph42oN: drafted, not posted (no GitLab creds in env).

#### Files touched
- `patches/dxvk-gplasync-master.patch` (vendored, rebased)
- `.github/workflows/new-All-in-one-nightly+zips-latest-stable.yml` (4 jobs → local apply; build-dxvk checkout)

---

## Session — 2026-05-13

### [feat] — Surface Banners-Turnip releases in nightlies_components.json (2026-05-13)

#### What changed
- `nightlies-components-json.yml` already fetched all Banners-Turnip releases on every run, but the loop body appended only to `turnip_driver_entries[]` (→ `banners-turnip_drivers.json` + `drivers.json`) and explicitly skipped `entries[]` (→ `nightlies_components.json`), per a "catalog stays focused on emulator/translation layers" policy.
- Flipped the policy: turnip entries now `entries.append(entry)` alongside the existing `turnip_driver_entries.append(entry)`. `banners-turnip_drivers.json` and `drivers.json` still written exactly as before (mirror workflows in kimchi/stevenmxz/mtr/white/all-in-one are unaffected).
- Result: on next run of the watcher, `nightlies_components.json` gains a `type: "GpuDriver"` row per turnip `.zip` asset across all Banners-Turnip tags. Subsequent runs idempotently re-scan all tags, so new releases land automatically with no state file.

#### Files touched
- `.github/workflows/nightlies-components-json.yml` (lines 193-195: replaced 3-line "NOT added" comment + lone append with two appends)

#### Trigger
- Watcher runs on `workflow_run` (completed) + cron + `workflow_dispatch`. Will pick up automatically on the next trigger, or can be kicked manually with `gh workflow run nightlies-components-json.yml`.

---

## Session — 2026-05-08

### [feat] — Add `.wcp.xz` Wine + Proton entries to components catalog (2026-05-08)
**Commits:** `ff7c02c` (initial add) → re-applied as `d004d9d` after watcher wipe

#### What changed
- Manually added `wine-11.3-arm64ec` and `proton-10-arm64ec` to `nightlies_components.json` (158 → 160). Both back `.wcp.xz` assets in the `Proton/wine` release that the watcher's `.wcp` / `.zip` filter currently skips.
- First add (`ff7c02c`) was wiped by the watcher (`86fec95`, regenerated back to 158). Recovered via rebase + re-insert in `d004d9d`.

#### Files touched
- `nightlies_components.json`

#### Known follow-up
- Watcher (`nightlies-components-json.yml`) only ingests `.wcp` / `.zip`; any `.wcp.xz` adds remain a manual-maintenance burden until the workflow is taught to include them.

---

### [docs] — Add Component Catalog (JSON Index) section to README (2026-05-08)
**Commit:** `c42b2d1`

#### What changed
- New top-level section between the intro and "Latest Nightly Releases" pointing to `The412Banner/winlator-contents` and the raw `contents.json` URL, so Winlator-family clients have a discoverable entry point for the catalog.

#### Files touched
- `README.md`

---

### [chore] — Strip `nightly-latest` entries from components catalog (2026-05-08)
**Commit:** `b2d392e`

#### What changed
- Removed all 22 `nightly-latest` rolling-tag entries (160 → 138): 8 Box64 · 4 DXVK · 4 VKD3D · 4 WOWBox64 · 2 FEXCore.
- Reason: `nightly-latest` entries duplicated archived versions and carried type/filename mismatches (WOWBox64 builds filed under Box64 and vice versa).
- Mirrored the same delete to `winlator-contents/contents.json` (`5b0bfc9` on that repo).

#### Files touched
- `nightlies_components.json`

#### Known follow-up
- Workflow `nightlies-components-json.yml` lines 98–115 unconditionally re-fetch the `nightly-latest` tag, so this delete will be reverted on the next workflow run unless those lines are removed.

---

## Session — 2026-03-23

### [fix] — Add actions: write permission to Create Nightly Release job (2026-03-23)
**Commit:** `278522f`

#### What changed
- `new-All-in-one-nightly+zips-latest-stable.yml`: added `actions: write` to the `Create Nightly Release` job's `permissions` block
- Root cause: `gh workflow run nightlies-components-json.yml` requires `actions: write` on `GITHUB_TOKEN`; job only had `contents: write`, causing HTTP 403 on every run

#### Files touched
- `.github/workflows/new-All-in-one-nightly+zips-latest-stable.yml`

---

## Session — 2026-03-06

### [feat] — Dynamic release notes + upstream change tracking (2026-03-06)
**Commit:** `c783331`

#### What changed

**Upstream-watcher.yml:**
- Added `NAMES` array mapping each repo URL to a friendly display name
- Before overwriting `upstream_hashes.txt`, captures old hash per repo to detect what changed
- For each changed repo: fetches commit message + date from GitHub API
- Builds `CHANGED_JSON` array (repo, name, old hash, new hash, full hash, message, date)
- Writes `upstream_changes.json` with `triggered_at` timestamp + `changed` array
- Commits both `upstream_hashes.txt` and `upstream_changes.json` when changes detected
- Triggers `new-All-in-one-nightly+zips-latest-stable.yml` (unchanged trigger mechanism)

**new-All-in-one-nightly+zips-latest-stable.yml (create-release job):**
- Added `actions/checkout@v4` step so the job can read `upstream_changes.json`
- Added `Build Release Body` step:
  - Queries GitHub API live for all 6 upstream repos (latest commit hash, message, date)
  - Marks any repo updated in last 24h with 🆕 badge in status table
  - Reads `upstream_changes.json` — if written within the last 3 hours, generates "🔄 What Triggered This Build" table with old→new commit links
  - Writes full release body to `release_body.md`
- Switched from `body:` (static inline) to `body_path: release_body.md` in softprops action
- Added disclaimer to top of every nightly release body:
  `⚠️ DISCLAIMER: NIGHTLY BUILDS ARE NOT ALWAYS STABLE OR RECOMMENDED! USE AT YOUR OWN RISK! STABLE RELEASES ARE ALWAYS BEST TO USE!`

#### Files touched
- `.github/workflows/Upstream-watcher.yml`
- `.github/workflows/new-All-in-one-nightly+zips-latest-stable.yml`

---

### [chore] — Disclaimer added to all existing nightly releases (2026-03-06)
**Method:** `gh release edit` loop via CLI (no commit)

#### What changed
- Prepended disclaimer to all 9 existing nightly release descriptions:
  - nightly-20260306-143533
  - nightly-20260306-135338
  - nightly-20260306-124048
  - nightly-20260306-112704
  - nightly-20260306-103204
  - nightly-20260306-090646
  - nightly-20260306-073819
  - nightly-20260305-223528
  - nightly-20260305-192208
- Non-nightly releases (Steam-clients, Bionic-Ludashi-proton, Box64, FexCore, etc.) left untouched

---

### [docs] — Progress log created (2026-03-06)
**Commit:** (this file)
#### What changed
- Created PROGRESS_LOG.md to track all changes to this repo going forward

---

## Session — 2026-03-11

### [feat] — Proton Bleeding-Edge ARM64EC standalone workflow (2026-03-11)
**Commits:** `491632a` (initial), `9943fa9`→`e9c28db` (release notes fix)

#### What changed

**New file: `.github/workflows/proton-bleeding-edge-nightly.yml`**
- Standalone workflow to build ValveSoftware/wine `bleeding-edge` branch with GameNative Android + ARM64EC patches
- Schedules every 6 hours + `workflow_dispatch` (inputs: `wine_ref`, `gamenative_ref`, `target_app_id`, `force_build`)
- **Job 0 — sync-scripts:** Fetches all build scripts/patches from Pepelespooder/proton-arm64-nightlies via GitHub API, compares byte-for-byte, commits any changes to `proton-scripts/`. Gracefully skips if upstream unavailable. Local copies act as permanent fallback.
- **Job 2 — build:** Clones ValveSoftware/wine + GameNative/proton-wine, applies patches from `proton-scripts/`, downloads LLVM MinGW 20250920 (bylaws) + NDK r27d + termuxfs aarch64, compiles full ARM64EC Wine, packages `.wcp` (zstd-tar) and `.wcp.xz` (XZ-tar + prefixPack) with SHA256
- **Job 3 — release:** Queries GitHub API live for wine bleeding-edge commit info, builds styled release notes matching all-in-one format, always publishes a release (no skip gate), updates and commits `proton-latest.json`
- Release tag format: `proton-bleeding-edge-{date}-{hash}-run{N}`
- Release is always pre-release

**New file: `proton-scripts/`** (39 files)
- Full backup of all build dependencies from Pepelespooder's repo
- `scripts/` — 26 Python/shell scripts (filter_patches.py, patch_build_script.py, fix_*.py, generate_profile.py, create-proton-wcp.sh, verify_required_markers.py, etc.)
- `ge-second-pass/test-bylaws/` — 2 BYLAWS patch overrides
- `ge-second-pass/focus/`, `keyboard/`, `mouse/`, `performance/` — additional patches
- `patches/` — dlls_winex11_drv_window_c.patch

**New file: `proton-latest.json`**
- Tracks last built Wine hash, version name, WCP/WCP.XZ filenames + SHA256 checksums, release tag
- Read by release job to determine old hash for "What Triggered" section

**Modified: `.github/workflows/Upstream-watcher.yml`**
- Added ValveSoftware/wine `bleeding-edge` branch tracking (separate from the main 5-repo loop)
- Stored under key `https://github.com/ValveSoftware/wine@bleeding-edge: <hash>` in `upstream_hashes.txt`
- New output: `wine_changed` (true when wine bleeding-edge HEAD changes)
- New output: `anything_changed` (true when any hash changed — used for commit step)
- New trigger step: `gh workflow run "proton-bleeding-edge-nightly.yml"` fires when `wine_changed == true`
- Existing `changed` output still triggers all-in-one nightly as before

#### Release note format (matching all-in-one style)
- Disclaimer at top
- `### 🚀 Proton Bleeding-Edge Build: {tag}`
- `### 🔄 What Triggered This Build` — old→new wine hash table when hash changed; "Scheduled/manual" otherwise
- `### 📊 Upstream Status` — wine bleeding-edge commit with 🆕 badge if updated in last 24h
- `### 📦 Built Components` — Proton ARM64EC row with commit link
- `### 📦 Files Included` — WCP + WCP.XZ with sha256 note

#### Bug fixed
- **Heredoc PY terminator not found (run6):** Triple-quoted f-string content had zero YAML indentation. YAML calculated min-indent=0 so stripped nothing — the `PY` end-marker kept 10 leading spaces in the shell script and bash never matched it. Fixed by replacing `f"""..."""` with `list` + `"\n".join()` so all lines stay indented inside the Python block.

#### Files touched
- `.github/workflows/proton-bleeding-edge-nightly.yml` (new)
- `.github/workflows/Upstream-watcher.yml`
- `proton-scripts/` (new directory, 39 files)
- `proton-latest.json` (new)

---

## Session — 2026-03-11 (continued)

### [fix] — Rebase conflict fix + release description improvements (2026-03-11)
**Commit:** `dfacb3f`

#### What changed
- `git pull --rebase -X ours` in "Commit proton-latest.json" step — concurrent runs no longer fail with a merge conflict; current run's version of `proton-latest.json` always wins
- Added `⬇️ Download` section to release description with a per-file table (file name, description, link) and a "Which file do I need?" callout explaining `.wcp` vs `.wcp.xz`

#### Files touched
- `.github/workflows/proton-bleeding-edge-nightly.yml`

---

### [fix] — sync-scripts rebase conflict fix (2026-03-11)
**Commit:** `76645fe`

#### What changed
- `git pull --rebase -X ours` also applied to the sync-scripts job commit step (same race condition as release job — if two concurrent runs both detect upstream script changes, the second would conflict)

#### Files touched
- `.github/workflows/proton-bleeding-edge-nightly.yml`

---

### [feat] — Auto-update README with latest releases after every build (2026-03-11)
**Commits:** `35fb31a` (proton README), `6142553` (all-in-one README + combined section)

#### What changed

**README.md:**
- Replaced single `## 🍷 Latest Proton Bleeding-Edge Release` section with a combined `## 🌙 Latest Nightly Releases` section containing two sub-sections:
  - `### 📦 All-in-One Emulation Nightly` — updated by all-in-one workflow; markers: `<!-- NIGHTLY-LATEST-START/END -->`
  - `### 🍷 Proton Bleeding-Edge ARM64EC` — updated by proton workflow; markers: `<!-- PROTON-LATEST-START/END -->`

**proton-bleeding-edge-nightly.yml (Create GitHub Release step):**
- Python block also rewrites `README.md` between `PROTON-LATEST-START/END` markers after writing release notes
- Table shows: release link, wine commit link + message, date, asset download link
- Commit step stages `README.md` alongside `proton-latest.json`

**new-All-in-one-nightly+zips-latest-stable.yml (create-release job):**
- New `Update README with latest nightly` step after `Create GitHub Release`
- Python block rewrites `README.md` between `NIGHTLY-LATEST-START/END` markers
- Table shows: release link, FEX commit+version, VKD3D std+ARM64EC commits, DXVK std+ARM64EC commits, Box64 repo links, asset download link
- Commits and pushes `README.md` with `git pull --rebase -X ours`

#### Files touched
- `README.md`
- `.github/workflows/proton-bleeding-edge-nightly.yml`
- `.github/workflows/new-All-in-one-nightly+zips-latest-stable.yml`

---

## Session — 2026-03-16

### [feat] — Kimchi Driver Mirror workflow (2026-03-16)
**Commit:** `0d9bd05`

#### What changed

**New file: `.github/workflows/kimchi-driver-mirror.yml`**
- Mirrors all releases from K11MCH1/AdrenoToolsDrivers (154 releases, 200 assets, ~938 MB total)
- Runs daily at 06:00 UTC + `workflow_dispatch` (optional `force_full_sync` boolean input)
- **Storage:** actual `.zip` files uploaded as assets on a persistent `kimchi-drivers` release (pre-release, never deleted); filenames prefixed with sanitized tag name to avoid collisions (e.g. `v26.0.0-rc08_Turnip_v26.0.0_R8.zip`)
- **Index:** `kimchi/drivers.json` committed to repo — contains `updated_at`, `source`, `mirror_release`, `total_releases`, `total_assets`, and per-release asset list with `name`, `mirror_name`, `size`, `original_url`, `mirror_url`, `published_at`
- **Incremental:** skips assets already present in the mirror release (by name) or already in `drivers.json` with a `mirror_url`; `force_full_sync` re-downloads everything
- `timeout-minutes: 360` — initial full sync can take up to 6h
- `git pull --rebase -X ours` on drivers.json commit step

#### Files touched
- `.github/workflows/kimchi-driver-mirror.yml` (new)
- `kimchi/drivers.json` (created on first run)

---

## Session — 2026-03-18

## Session — 2026-03-21

### [feat] — MTR manual_entries.json + v3.0.0 drivers (2026-03-21)
**Commit:** TBD

#### What changed
- Created `mtr/manual_entries.json` — static list of manually added drivers that survive workflow reruns; any entry whose filename isn't already in the source repo sync is appended to `mtr/drivers.json` after each run
- Added new "Merge manual entries" step to `mtr-driver-mirror.yml` — runs between "Download and mirror" and "Write root mtr_drivers.json"; reads `mtr/manual_entries.json` and appends missing entries by name
- Updated commit step to also stage `mtr/manual_entries.json`
- Added `Turnip_MTR_v3.0.0-b_Axxx.zip` and `Turnip_MTR_v3.0.0-p_Axxx.zip` (manually uploaded to mtr-drivers release, not yet in maxjivi05's repo) to all three JSON files: `mtr/drivers.json`, `mtr_drivers.json`, `drivers.json`

#### Files touched
- `mtr/manual_entries.json` (new)
- `mtr/drivers.json` (total_assets 33→35, two v3.0.0 entries added)
- `mtr_drivers.json` (two v3.0.0 entries added)
- `drivers.json` (two v3.0.0 entries inserted after v2.0.0-p)
- `.github/workflows/mtr-driver-mirror.yml` (new "Merge manual entries" step + manual_entries.json staged in commit)

---

### [feat] — Add MTR v3.2.0-b and v3.2.0-p to JSON tracking (2026-03-25)
**Commit:** `9d2da14`

#### What changed
- Added `Turnip_MTR_v3.2.0-b_Axxx` and `Turnip_MTR_v3.2.0-p_Axxx` entries to `mtr/drivers.json` (total_assets 35→37)
- Added matching `GpuDriver` entries to `mtr_drivers.json`
- Both entries point to mirror URLs under the `mtr-drivers` release tag

#### Files touched
- `mtr/drivers.json`
- `mtr_drivers.json`

---

### [fix] — Replace DXVK-NVAPI with Turnip in upstream status table (2026-03-18)
**Commit:** `e238ebb`

#### What changed
- Removed `jp7677/dxvk-nvapi` from `REPOS` array in `create-release` job status table
- Replaced with `The412Banner/Banners-Turnip`
- `NAMES` array updated: `"DXVK-NVAPI"` → `"Turnip (Banners)"`
- Upstream status table now shows Turnip commit status instead of the removed NVAPI project

#### Files touched
- `.github/workflows/new-All-in-one-nightly+zips-latest-stable.yml`
