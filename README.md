# ⚡ CYBER_CORE // 2D Real-Time C++ RPG Engine

![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?style=flat-square&logo=cplusplus)
![Framework](https://img.shields.io/badge/Framework-Raylib_v5.5-FF6F00?style=flat-square)
![IDE](https://img.shields.io/badge/IDE-Visual_Studio_2022-5C2D91?style=flat-square&logo=visualstudio)
![License](https://img.shields.io/badge/License-MIT-blue.style=flat-square)

**CYBER_CORE** is a high-performance, top-down 2D action RPG engine built in **C++17** using **Raylib 5.5**. Originally conceived as a CLI-based RPG engine, the project evolved into a real-time, 60 FPS graphical game featuring custom vector rendering, procedural particle effects, top-down camera tracking, a dynamic shop/upgrade economy, and tactical ability systems.

---

## 🛠 Tech Stack & Architecture

* **Language:** C++17
* **Graphics Framework:** [Raylib v5.5](https://www.raylib.com/) (Managed via NuGet)
* **Build System:** MSVC (Visual Studio 2022 Solution)
* **Design Patterns:** Component-driven entity design, state machine user interfaces, procedural particle management.

---

## 🚀 Key Features

* **Real-Time 2D Engine:** Continuous 60 FPS rendering pipeline, dynamic infinite background grid, anti-aliased vector graphics, and smooth camera target dampening.
* **Tactical Ability System:**
  * **[1] Cyber Dash:** Instant directional teleportation utility.
  * **[2] Radial EMP:** High-damage area-of-effect shockwave with custom particle bursts.
  * **[3] Sentry Turret Drop:** Automated perimeter defense drone with area-denial pulse fields.
* **Extensive Entity Roster:**
  * **7 Regular Enemy Types:** Cyber Rat, Drone Scout, Syndicate Thug, Enforcer Bot, Corrupted Cyborg, Viper Assassin, and Heavy Automaton.
  * **2 World Bosses:** *JUGGERNAUT PRIME* and *THE ARCHITECT* with enhanced health pools and high-damage output.
* **Safe Zone Hub & Arsenal Merchant:**
  * Interactive **Nexus Safe Zone** where combat and enemy targeting are suspended.
  * Toggleable shop interface (`E` key) offering **17 Weapons** across 4 rarity tiers (Common, Rare, Legendary, Exotic).
  * Permanent player stat upgrades (Max Health scaling) purchasable using accrued XP.

---

## 🎮 Controls

| Key | Action |
| :--- | :--- |
| **W / A / S / D** | Player Movement |
| **SPACE** | Primary Weapon Attack |
| **1** | Ability 1: Cyber Dash |
| **2** | Ability 2: Radial EMP |
| **3** | Ability 3: Sentry Turret Drop |
| **E** | Toggle Arsenal Merchant (Inside Safe Zone) |
| **H** | Upgrade Max Health (In Shop Menu) |
| **LEFT / RIGHT** | Cycle Available Weapons (In Shop Menu) |
| **B** | Purchase / Equip Selected Weapon |

---

## 📦 Building and Running

### Prerequisites
* Windows 10/11
* Visual Studio 2022 (with the **Desktop development with C++** workload installed)

### Setup Instructions
1. **Clone the Repository:**
   ```bash
   git clone [https://github.com/your-username/Text-Based-RPG-Engine.git](https://github.com/your-username/Text-Based-RPG-Engine.git)
   cd Text-Based-RPG-Engine
