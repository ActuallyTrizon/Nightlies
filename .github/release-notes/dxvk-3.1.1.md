DXVK **3.1.1** — every flavour, built from the upstream stable tag (`b1a1c99a`), not from master.

Upstream released 3.1.1 on 15 September 2026. Master has already moved past it, so these are rebuilt from the tag rather than taken from a nightly.

## Which file do I want?

| File | Use it for |
|---|---|
| `dxvk-3.1.1.wcp` | Plain upstream DXVK. Start here. |
| `dxvk-gplasync-3.1.1-1.wcp` | Async shader compilation (Ph42oN's GPLAsync). Fewer shader stutters, at the cost of occasional missing effects for a frame or two. |
| `dxvk-binsem-gplasync-3.1.1-1.wcp` | GPLAsync plus binary semaphores — the fix for the timeline-semaphore slowdown on Turnip/Adreno. |
| `dxvk-arm64ec-3.1.1.wcp` | The same as the plain build, compiled for ARM64EC containers. |
| `dxvk-gplasync-arm64ec-3.1.1-1.wcp` | GPLAsync, ARM64EC. |
| `dxvk-binsem-gplasync-arm64ec-3.1.1-1.wcp` | GPLAsync + binary semaphores, ARM64EC. |

Each one is also attached to its own flavour release, so existing links keep working.

## What's in upstream 3.1.1

- `DXGI_SCALING_NONE` and `SetBackgroundColor` implemented for swap chains — reported to fix some Adobe software.
- Fixed a shader compiler crash with certain ENB shaders.
- Fixed deadlocks in some Direct3D 9 games, including **SpellForce 2: Anniversary Edition** and **The Sims: Medieval**.
- **Call of Duty: Ghosts** — fixed exploding vertices on player models.
- **Corpse Party** — fixed a regression that darkened part of the screen.
- **Painkiller: Black Edition** — fixed the game window not being sized properly.
- **Rayman 3** — fixed a regression causing flickering graphics.
- **Skyrim SE** — worked around poor CPU performance when unlocking the frame rate on some setups.

**Upstream's own note:** some anti-virus software has again started flagging the 32-bit builds. These are false positives on upstream's binaries, not something added here.

## Notes on these builds

- The plain builds are upstream's own release binaries, repackaged — the x64 DLLs are byte-for-byte identical to `dxvk-3.1.1.tar.gz`.
- ARM64EC builds are compiled here from the same tag; upstream ships no ARM64EC binaries.
- GPLAsync has no per-release patch for the 3.1 line, so Ph42oN's current patch is applied to the stable tag.
- Every `.wcp` carries a `profile.json` whose version name matches its file name.

Upstream: Philip Rebohle (doitsujin/dxvk) · GPLAsync patch: Ph42oN
