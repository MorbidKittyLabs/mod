# Occupied mining-node inspection

Base: `8b7013e4643e85b4fe08e849d5f211d594b88c8e`.

ActionView on another player's occupied mining node selects its current occupant,
opens native rewards, hides rewards on the second press to expose strength, and
selects the original mining POI on the third press. Restoration is confirmed only
when that node's native viewer is visible. No Scan call or section change is used.
Only numeric node/fleet IDs and galaxy/system/instance identity survive a frame.
A selection or section change cancels tracking. Binding has a two-second deadline.
Restoration failure is logged and consumes the key without toggling back to loot.

Native MineClicked/OnEngageButtonClicked/OnScanButtonClicked paths are untouched.
Saved inspection IDs are never passed to an attack or course-setting method.
The third press resolves the current node POI again; the next inspection resolves
its occupant again, so a replacement is never substituted into a saved attack.
The change is limited to native occupied-node inspection.

## Metadata evidence

Recovered October 24 files, inspected directly with Il2CppDumper at metadata
version 31 (CodeRegistration `0x184c7dbc0`, MetadataRegistration `0x1853999e0`).

- global-metadata.dat SHA256: `ab0ccdc3e9e09e59d16a13431e2a0722255a71d5ff373cc09d95a47cd130ca04`
- GameAssembly.dll SHA256: `a5df6314bd78f15892a232a44e77cdcf2e3d20e9a6192f99120c0f3e52632274`
- NavigationInteractionUIContext has public reference field `Poi` of type
  `Digit.Prime.Navigation.POI`; POI has `Int64 get_ID()` and `NodeAddress get_Address()`.
- NavigationManager has instance `Boolean CanSelectPoi(Int64)` at RVA `0x12f77b0`
  and `Boolean SelectPoi(Int64)` at RVA `0x12f78b0`. Disassembly confirms both call
  `POI.Get(Int64)` (`0x13170f0`); SelectPoi forwards the live POI to native selection.
- MiningObjectViewerWidget.ScanButtonClickedEventHandler (`0x10ed060`) jumps to
  SetPoiToSelect (`0x10ed070`), which reads inherited widget context and its Poi
  and copies Poi.ID for native return selection. The feature does not invoke it.
- FleetDeployedData has `Int64 get_ID()` (`0x19ea420`).
- NavigationViewData has `NodeAddress get_ViewingAddress()`; NodeAddress exposes
  Galaxy/System (Int64) and Instance (Int32).

Runtime bindings verify full method/field types and handle managed exceptions.
Offsets/RVAs above document the inspected client and are never used as bindings.
Compatibility with newer game clients still requires the live test below.

`show_info_pending` is already declared at hotkeys.cc:76 in this exact base and
its existing native rewards path remains intact. This feature uses its own
bounded pending selection and does not depend on that variable.

## Validation

Host regression fixture: `tests/occupied_node_cycle.cc`, built with C++23,
-Wall -Wextra -Werror, AddressSanitizer and UndefinedBehaviorSanitizer. Leak
checking is disabled in the container because its ptrace setup prevents LSAN.
Windows full compilation, checked bindings and installer checks are recorded by
the isolated build workflow. A successful compile is not a live game test.

Live acceptance: one account, node -> loot -> strength -> the same node; repeat.
Then empty node; occupant leaves/replaced; unrelated selection; system and server
instance changes; native Scan and Attack remain available, with no stale target.
There must be no Scan screen, blackout or system reentry. Do not update upstream
PR #382 until these live checks succeed.
