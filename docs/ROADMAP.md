# ParusEngine Roadmap

**Date:** 2026-06-24
**Author context:** Solo developer, a few hrs/week. Project is a **portfolio / career showcase**. Core differentiator is **creator-side generative AI** (describe a game in natural language → AI composes scenes from ParusStore assets). Likes visible/fun results *and* clean, frequently-refactored code.

---

## Where you actually are

You are much further along than "I don't know where to go next" suggests:

- **Rendering is portfolio-grade already.** Vulkan multi-pass pipeline (shadow → depth pre-pass → SSAO → SSAO blur → main), full GGX PBR with 5 texture types, PCF directional shadows, point lights, SSAO, ACES tonemapping, procedural sky captured to a cubemap. ~7,300 lines, well-structured with a clean builder pattern.
- **Architecture is clean.** Service-locator DI, type-safe event system, thread pool, INI config. No circular dependencies.
- **Authoring substrate just landed.** Console command system (trie autocomplete, multi-word commands) + the new binary scene format (`.pworld`/`.pmesh`/`.ptex`) with save/open/import and async load.

**The gap between what you have and your vision is NOT graphics.** You have a beautiful *viewer*. Your vision is a *creation tool for non-technical people, driven by AI*. The missing pieces are, in order: **authoring**, **the AI loop**, **gameplay**, and a **store backend** — not more rendering features.

## The one strategic insight that orders everything

Your **console command system + binary scene format are an almost-perfect substrate for creator-side AI.** An LLM's natural output is structured text / tool calls. If your engine is fully drivable by commands that build and edit scenes, then *"AI builds your game"* reduces to *"the LLM emits engine commands."* And that exact same authorable API is what a **human** non-technical creator needs through a GUI editor.

So one investment — **making the scene fully authorable through a structured entity + command API** — serves both the human editor and the AI. That's why authoring comes first.

---

## Recommended sequence

### Phase 1 — Make the scene authorable (entity model + in-editor editing)
*Foundational **and** visible — and it scratches the clean-code itch.*

Turn the viewer into an editor.

- Introduce a clean **`Entity`** abstraction: named, has a transform, optional components (mesh, light; later collider, script). Refactor the loose `WorldMeshInstance` + light arrays into entities. *(This is the satisfying refactor that also unblocks everything.)*
- Extend serialization to round-trip entities — and **write your first unit tests here**: `save → load → compare`. Your data foundation deserves this one piece of test coverage.
- ImGui **Scene Outliner** panel: list of entities + selection.
- **Properties panel**: edit transform (pos/rot/scale), swap mesh, edit light/sky.
- Runtime **add / duplicate / delete** entity — expose each as a console command too (these commands become the AI's tools in Phase 2).
- *(Stretch)* translate/rotate gizmo in the viewport.

**Visible win:** click an object and move it; build a scene by hand without editing files.

### Phase 2 — The AI creation loop (MVP of the differentiator)
*Your unique selling point and the portfolio centerpiece.*

Type a natural-language request → the AI composes/edits the scene using your command/entity API and a catalog of available assets.

- Define a structured **scene-action schema** (add entity, set transform, set light, set sky…) — essentially a JSON mirror of your console commands.
- Build a thin **HTTP client** (your first networking code) to call the **Claude API** with **tool use**: expose the scene actions as tools, let the model plan and emit tool calls, the engine executes them live.
- Give the model a **manifest of available assets** (mesh names + descriptions) so it composes from real content.
- ImGui **prompt box**: user types, watches the scene assemble.

Start local and simple — API key in config, asset manifest from a local folder, no server, no auth. **The magic is the loop, not the infrastructure.**

**Visible win:** "make a cozy forest clearing with a campfire" → the scene builds itself. This is the demo that makes the portfolio memorable.

### Phase 3 — ParusStore (minimal → real)
*Gives the AI a real library to compose from, and finally realizes the README's "download from the web" vision.*

- **v1 (local catalog):** a JSON catalog over `bin/assets` with metadata (name, tags, description, thumbnail) that the AI queries. No server.
- **v2 (real backend):** a small ParusStore server that serves the catalog + `.pmesh`/`.ptex`/`.pworld` over HTTP so the engine downloads assets/scenes on demand. Add accounts/sharing only if the product path demands it.

### Phase 4 — Gameplay primitives (make it a *game*, not a diorama)
*Your option 4. Big — sequence it after the AI loop is proven, but a minimal slice can come earlier for fun.*

- **Minimal slice first:** a first-person/character controller with gravity + capsule-vs-world collision so you can *walk through* generated scenes. (Great visible win; consider a taste of this earlier.)
- **Behavior/scripting hooks:** build gameplay events on the type-safe `Event.h` you already have. Embed Lua only if hand-rolled C++ hooks prove limiting.
- **Physics:** start with your own AABB/capsule collision rather than Bullet/PhysX — lighter, more fitting for a learning portfolio piece, and enough for walk-around games. Graduate to a real library only if needed.

### Phase 5 — Graphics treats (opportunistic)
*Your option 5. Lowest urgency — this is already your strength — but the best source of quick screenshots.*

Use these as *reward* tasks between heavier phases:
- Finish the **fog** (already stubbed in `main.frag`).
- **Day/night** by animating sky params + sun direction.
- **Clouds**, then further polish.

### Phase 6 — Infrastructure (deliberately deferred)
*Your option 1 — mostly "not yet."*

- **Skip** the CMake rewrite + GitHub Actions CI for now: high effort, ~zero visible payoff, and you are the only developer. Revisit when you get contributors or need Linux/macOS builds.
- **Do** add tests incrementally — specifically the serialization round-trip tests in Phase 1. That is the 20% of testing that gives 80% of the safety for your data foundation.

---

## Verdict on your six options

| Your option | Verdict | When |
|---|---|---|
| 1. CMake + CI/CD + tests | Mostly **defer**. Do serialization round-trip tests only. | Tests in Phase 1; CMake/CI later, contributor-driven |
| 2. Backend server (download games/models) | **Defer**; start as a local catalog. | Phase 3 |
| 3. Refactor all existing code | **Don't** as a milestone — refactor *opportunistically* inside every phase. | Continuous (boy-scout rule) |
| 4. Engine features (physics, API, player, skeleton) | **Yes** — but after the AI loop is proven. Do a minimal walkable slice for fun earlier. | Phase 4 (taste earlier) |
| 5. Graphics (photorealism, day/night, clouds, fog) | **Lowest urgency** (already your strength). Use as reward tasks. | Phase 5 |
| 6. Something else | **This is it:** the authorable editor + AI creation loop. | Phases 1–2 |

## Cross-cutting principle

**Refactor opportunistically, not in a big bang.** You like cleaning code — channel it inside each phase as you touch each area. A dedicated "refactor everything" phase risks destabilizing working code with no visible progress.

## Immediate next step

Start **Phase 1**: the `Entity` model + Scene Outliner + properties panel. It is foundational, visibly testable, refactor-flavored, solo-friendly, and the prerequisite for the AI loop. Brainstorm it into a concrete design + plan when ready.
