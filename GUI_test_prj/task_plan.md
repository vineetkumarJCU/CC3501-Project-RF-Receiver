# LVGL Windows SDL Test Project Plan

## Goal
Create a self-contained Windows 11 x64 CMake project in this folder using LVGL v9.5.0, SDL, the full LVGL library, and `lv_demo_widgets`; then configure, build, and smoke-test it locally.

## Phases
- [complete] 1. Inspect workspace and available Windows build tools.
- [complete] 2. Acquire pinned LVGL v9.5.0 and SDL dependencies.
- [complete] 3. Add application source, LVGL configuration, and CMake build files.
- [complete] 4. Configure and compile an x64 build.
- [complete] 5. Smoke-test the generated executable and document usage.

## Constraints and decisions
- Pin LVGL exactly to v9.5.0.
- Keep all required library sources under this project so it can be rebuilt without fetching them again.
- Use SDL for the Windows display/input backend.
- Launch `lv_demo_widgets`.

## Errors Encountered
| Error | Attempt | Resolution |
|---|---:|---|
| Combined parallel inspection returned exit code 1 without useful output | 1 | Split inspection into individual commands so an expected missing-tool/search result cannot hide other output. |
| `rg` received Unix-style wildcard paths that PowerShell did not expand | 1 | Search the parent directories with `-g` glob filters instead. |
| Windows `CopyFromScreen` captured the accelerated SDL window as all black | 1 | Add an environment-controlled SDL renderer readback in the desktop-only entry point for accurate visual QA. |
| A multi-file patch contained a malformed hunk boundary | 1 | Split and reissued the patch with valid per-file hunks; no source change was lost. |
| The first automated tune-result capture landed on the overlay transition boundary | 1 | Make the desktop-only screenshot delay configurable and repeat after the interaction has settled. |
| Windows `PostMessage` input did not reliably map to SDL widget coordinates | 1 | Do not treat OS message injection as an LVGL interaction oracle; expose the result modal through the same public API used by the keypad callback and verify it directly. |
| A static API scan used unsupported default-regex lookahead and returned an empty result | 1 | Re-run the focused scan with ripgrep PCRE2 enabled and anchored definition patterns. |
| A planning-file patch expected the waveform spectrum decision on its own line | 1 | Re-read the plan tail and apply smaller hunks matching the combined decision line. |
| The planning completion script was blocked by the machine's PowerShell execution policy | 1 | Re-run that trusted local script with a process-scoped `ExecutionPolicy Bypass`. |
| The current completion script counts only `### Phase` headings, while this legacy plan uses a checklist under `## Phases` | 1 | Preserve the established plan format and verify its 26 complete / 0 pending / 0 in-progress markers directly. |

## Final verification
- CMake configure: passed.
- Release build: passed (941 initial steps).
- Incremental rebuild: passed (`ninja: no work to do`).
- Executable architecture: `pei-x86-64` / `i386:x86-64`.
- Runtime: widgets window launched, responsive, and left open for the user.
- Runtime imports: Windows system DLLs only; SDL2 and MinGW runtimes are statically linked.

## Follow-up: Full configuration file
- [complete] 6. Replace the minimal configuration with the complete official LVGL v9.5.0 template while preserving project settings.
- [complete] 7. Rebuild and verify the full configuration.

## Follow-up: Radio GUI framework
- [complete] 8. Read the GUI specification and inspect the existing 320x240 SDL project.
- [complete] 9. Define the portable GUI architecture, public data/update APIs, callbacks, and visual system.
- [complete] 10. Implement the three horizontally swiped screens and frequency-entry/status overlays.
- [complete] 11. Add a desktop mock backend and replace the widgets demo entry point.
- [complete] 12. Build, run, visually inspect, and refine the 320x240 interface.
- [complete] 13. Document integration and verification results.

### GUI design decisions
- Use a dark radio-instrument aesthetic with cyan, blue, amber, and semantic status accents.
- Present information in hierarchy-driven cards, hero values, status pills, and compact tables rather than a uniform component stack.
- Use a horizontal LVGL tile view ordered Hardware / Radio / GPS, initially focused on Radio.
- Keep GUI code platform-independent: hardware work is exposed through callbacks; hardware/driver state enters through setter APIs.
- Keep callbacks short and synchronous at the GUI boundary so the same GUI can remain single-threaded when moved to the existing Core 1 LVGL task.

### Radio GUI final verification
- Full and incremental CMake Release builds passed; final incremental result is `ninja: no work to do`.
- Hardware, Radio, GPS, numeric keypad, and tune-result modal were captured directly from the SDL renderer and visually inspected at 320x240.
- Battery float formatting and intrusive LVGL sysmon overlays found during visual QA were corrected.
- All 19 public `radio_gui.h` functions have source definitions.
- Final executable is PE x86-64 and imports only Windows system DLLs.
- `rp2040_radio_gui_demo.exe` was launched successfully and left responsive for user inspection.

## Follow-up: Touch keypad sizing
- [complete] 14. Reallocate the 320x240 frequency overlay to prioritize touch targets over decoration.
- [complete] 15. Rebuild and inspect the enlarged keypad through direct SDL renderer capture.

### Touch keypad verification
- Numeric keypad height increased from 100 px to 145 px.
- Approximate numeric button size increased from about 90x21 px to about 98x34 px.
- Modal margins, heading, text input, internal gaps, and decorative pill were reduced or removed.
- kHz, MHz, and OK remain full-width touch targets with no clipping at 320x240.

## Follow-up: Tune-result modal capacity
- [complete] 16. Expand the tune-result modal to show the complete multiline SI4732 status response.
- [complete] 17. Rebuild and verify the long result text through direct SDL renderer capture.

### Tune-result modal verification
- Result card increased from roughly 300x132 px to 312x232 px.
- All current SI4732 result lines render simultaneously without clipping or wrapping loss.
- The OK button remains a full-width 32 px touch target.

## Follow-up: Complete GUI state getters
- [complete] 18. Add read-only getters for every user-controlled and software-set GUI state.
- [complete] 19. Build and run a getter-state verification against the populated SDL demo.
- [complete] 20. Document getter usage and ownership/threading rules.

### Getter verification
- Added 16 getters; the public API now contains 35 functions in total.
- Static header/source parity scan reports zero missing definitions.
- Runtime state dump matched all mock values: Radio page, FM, MHz, 101.70 MHz, volume 32, BW_AUTO, 3.97 V, amplifier enabled/Class-D, backlight 38, GPS enabled/locked, SD OK/logging off, RSSI 48 dBuV, and latitude 27.4698 S.

## Follow-up: High-contrast telemetry tables
- [complete] 21. Enlarge and strengthen the RX/GPS telemetry table typography and spacing.
- [complete] 22. Rebuild and visually verify both scrolled tables at 320x240.

## Follow-up: RX signal waveform page
- [complete] 23. Design the four-page navigation and compact dual-chart layout.
- [complete] 24. Implement the waveform page and public 256-point time/spectrum setters.
- [complete] 25. Populate representative desktop waveforms and document the API.
- [complete] 26. Build, capture, and visually verify the complete waveform page at 320x240.

### Waveform page decisions
- Place the new page between Radio and GPS: Hardware / Radio / RX signal waveform / GPS.
- Keep both plots visible together in one 320x240 viewport.
- Use LVGL chart series for the 256 samples and custom high-contrast axis/grid presentation suitable for the embedded target.
- Time-domain input is signed 8-bit fixed-point data; spectrum input uses 256 logarithmic-amplitude samples with an explicit dB range.

### Waveform verification
- Four-page navigation order and all `01 / 04` through `04 / 04` counters are consistent.
- Public header/source parity reports zero missing definitions; no obsolete three-page references remain in source or README.
- Incremental CMake build and hidden SDL runtime capture passed.
- Both 256-point plots, dynamic axes, and dashed grids fit simultaneously at 320x240 without scrolling or clipping.
