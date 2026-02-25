# OBSTACLE

**Séquenceur minimal techno — plugin AU pour macOS**

Un drum machine et séquenceur mélodique 6 pistes / 16 pas, construit avec JUCE 8. Inspiré de Trentemøller, Nils Frahm et du workflow Elektron.

---

## 📦 Téléchargement (version précompilée)

👉 https://github.com/laurentcbn/obstacle/releases/tag/v1.2.0

1. Télécharger le fichier **`OBSTACLE-macOS-v1.2.0.zip`**
2. Extraire l'archive
3. Installer le Standalone ou le plugin AU (voir section *Installation* ci-dessous)

---

## Fonctionnalités

- **6 pistes** — Kick, Snare, Hihat, Bass, Lead, Pad
- **Séquenceur 16 pas** avec sélection de note par pas (gamme de La mineur naturel)
- **8 patterns indépendants (A–H)** — composez plusieurs patterns distincts
- **Song Mode** — chaîne de 16 slots avec répétitions par slot (×1 à ×8)
- **Bouton NEXT** — force le passage au pattern suivant à la prochaine boucle
- **Swing** pour le groove
- **Chaîne FX** — Reverb, Delay (mix + feedback), filtre LP, Drive/Saturation
- **Par piste** — volume, mute, et contrôle decay/filtre/attaque
- **Transposition** — ±12 demi-tons
- **Randomize** — génère un nouveau pattern dans le style courant
- **UI web embarquée** directement dans la fenêtre du plugin (pas de navigateur externe)
- Formats : **AU** (GarageBand, Logic Pro) + **Standalone**

---

## Screenshots

> *(add screenshot here)*

---

## Prérequis

- macOS 13+
- Xcode 15+ (pour compiler depuis les sources)
- CMake 3.22+

---

## Compiler depuis les sources

```bash
git clone https://github.com/laurentcbn/obstacle.git
cd obstacle

# Configurer
cmake -B build -G Xcode

# Compiler le Standalone
DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer \
  xcodebuild -project build/OBSTACLE.xcodeproj \
             -scheme OBSTACLE_Standalone \
             -configuration Release

# Compiler le plugin AU
DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer \
  xcodebuild -project build/OBSTACLE.xcodeproj \
             -scheme OBSTACLE_AU \
             -configuration Release
```

---

## Installation

### Application Standalone
```bash
rm -rf /Applications/OBSTACLE.app
cp -r build/OBSTACLE_artefacts/Release/Standalone/OBSTACLE.app /Applications/
```

### Plugin AU (GarageBand / Logic Pro)
```bash
cp -r build/OBSTACLE_artefacts/Release/AU/OBSTACLE.component \
      ~/Library/Audio/Plug-Ins/Components/
```

Puis rescanner dans GarageBand : redémarrer l'app, ou lancer `auval -a` dans le Terminal.

---

## Utilisation dans GarageBand

1. Ouvrir GarageBand et créer une piste **Audio**
2. Ouvrir **Smart Controls** → **Plug-ins**
3. Cliquer sur un slot → **Audio Units** → **Fred** → **OBSTACLE**
4. Appuyer sur **PLAY** dans l'interface du plugin

---

## Utilisation dans Logic Pro

1. Créer une piste **Software Instrument**
2. Ouvrir le slot instrument → **AU Instruments** → **Fred** → **OBSTACLE**
3. Appuyer sur **PLAY** dans l'interface du plugin

---

## Architecture

```
Source/
├── PluginProcessor.cpp   # Moteur séquenceur, synthèse audio, paramètres
├── PluginProcessor.h     # Déclarations des paramètres, types de voix, structures Pattern/SongSlot
├── PluginEditor.cpp      # Hôte WebBrowserComponent + HTML/CSS/JS
├── PluginEditor.h        # Déclaration de la classe éditeur
└── SynthEngine.h         # Voix Kick, Snare, Hihat, Bass, Lead, Pad + chaîne FX
```

L'UI est une page HTML/CSS/JS complète servie depuis la mémoire C++ via le resource provider `WebBrowserComponent` de JUCE 8. La communication JS ↔ C++ utilise le bridge de fonctions natives JUCE (`window.__JUCE__.backend`).

---

## Contrôles

| Contrôle | Description |
|---|---|
| **PLAY / STOP** | Démarrer ou arrêter le séquenceur |
| **▶▶ NEXT** | Forcer le passage au pattern suivant (Song Mode) |
| **REGEN** | Randomiser le pattern en cours d'édition |
| **A–H** | Sélectionner le pattern à éditer (cyan = édition, contour rouge = lecture) |
| **KEY** | Transposer toutes les pistes mélodiques (±12 demi-tons) |
| **BPM** | Tempo (60–200 BPM) |
| **Grille de pas** | Clic gauche pour activer/désactiver un pas. Clic droit sur Bass/Lead/Pad pour choisir la note (A–G) |
| **Song Chain** | Clic = cycle de pattern, clic droit = répétitions (×1–×8), ⟳/■ = boucle ou stop |
| **Mute** | Silence une piste sans effacer son pattern |
| **Vol** | Volume par piste |
| **Dec / Filt / Atk** | Decay (percussions), ouverture filtre (basse), attaque (lead/pad) |
| **REV** | Mix de reverb |
| **DLY / FEED** | Mix et feedback du delay |
| **CUT** | Cutoff du filtre passe-bas global |
| **DRIVE** | Saturation douce |
| **Master VOL** | Volume de sortie |

---

## Licence

MIT — faites-en ce que vous voulez.

---

*Construit par CBN*
