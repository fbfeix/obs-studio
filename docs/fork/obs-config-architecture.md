# OBS: Profile, Szenensammlungen, Szenen, Quellen

Kurzüberblick, wie OBS seine Konfiguration aufteilt, und wo der native
Vertical-Canvas dieses Forks hineinpasst.

## Grundprinzip

OBS trennt zwei unabhängige Achsen, die frei kombinierbar sind:

| Achse | Frage | Enthält |
|---|---|---|
| **Profil** | *Wie* wird ausgegeben? | Auflösung, FPS, Encoder, Stream-Ziel + Key, Aufnahmepfad/-format, Audio-Samplerate |
| **Szenensammlung** | *Was* wird gezeigt? | Szenen, Quellen, Filter, Übergänge, Canvases, Hotkeys der Quellen |

Beispiel: Profil „ReelScreencast“ (1080x1920, 60 fps) lässt sich mit jeder
Szenensammlung verwenden, und umgekehrt.

## Ablage auf der Platte

```
~/.config/obs-studio/                 (Flatpak: ~/.var/app/com.obsproject.Studio/config/obs-studio)
├── user.ini                          aktives Profil + aktive Sammlung, Sprache, UI-Einstellungen
├── global.ini                        app-weite Einstellungen (ältere Versionen: alles hier)
├── basic/
│   ├── profiles/<ProfilDir>/
│   │   ├── basic.ini                 Video, Output, Audio, Aufnahme-Pfad
│   │   ├── service.json              Stream-Dienst, Server, Stream-Key (geheim!)
│   │   ├── streamEncoder.json        Encoder-Einstellungen Stream
│   │   └── recordEncoder.json        Encoder-Einstellungen Aufnahme (nur im Advanced-Modus)
│   └── scenes/<Sammlung>.json        eine Datei pro Szenensammlung
└── plugin_config/<plugin>/           Plugin-eigene Einstellungen (z. B. obs-websocket)
```

Im Fork liegt dasselbe unter `build_docker/config/.config/obs-studio/`.

## Inhalt einer Szenensammlung (`basic/scenes/*.json`)

```
{
  "name", "version",
  "sources":   [ ... ],          ALLE Quellen, auch Szenen und Gruppen
  "groups":    [ ... ],
  "scene_order": [ {name} ... ], Reihenfolge in der Szenenliste
  "current_scene",               Vorschau-Szene (Studio-Modus)
  "current_program_scene",       Live-Szene
  "transitions", "current_transition", "transition_duration", "quick_transitions",
  "canvases":  [ ... ],          zusätzliche Canvases (z. B. vertikal)
  "vertical":  { ... },          nur Fork: Vertical-Konfiguration (s. u.)
  "DesktopAudioDevice1", ...     globale Audioquellen (Kanäle 1–6)
  "modules":   { ... }           Plugin-Daten pro Sammlung
}
```

## Objektmodell (libobs)

```
Canvas (Video-Mix: Auflösung, FPS)
 └── Kanal 0 ──► Szene (die gerade ausgegebene)
                  └── Szenen-Item ──► Quelle (+ Filter)
                       (Position, Skalierung, Crop, Sichtbarkeit, Bounds)
```

- **Quelle** (`obs_source_t`): Kamera, Bildschirm, Bild, Mikro, … Hat Name,
  UUID, Einstellungen, Filter. Existiert **einmal** pro Sammlung.
- **Szene** ist selbst eine Quelle (Typ `scene`). Sie enthält Szenen-Items.
- **Szenen-Item** ist die *Platzierung* einer Quelle in einer Szene. Dieselbe
  Quelle kann in mehreren Szenen stecken, jede mit eigener Position/Größe.
- **„Bestehende hinzufügen“** erzeugt ein neues Item auf dieselbe Quelle:
  Einstellungen (z. B. DroidCam-Gerät) sind geteilt, Position/Größe nicht.
- **„Neu erstellen“** erzeugt eine zweite, unabhängige Quelle.
- **Gruppe**: Szene innerhalb einer Szene, wird zusammen verschoben.
- **Filter** hängen an der Quelle, wirken also in jeder Szene, die sie nutzt.

## Canvases

- **Haupt-Canvas**: Auflösung aus dem Profil (Basis/Ausgabe). Seine Szenen sind
  die normale Szenenliste. Stream, Aufnahme und virtuelle Kamera lesen ihn.
- **Zusätzliche Canvases** (seit OBS 31): eigener Video-Mix mit eigener
  Auflösung und eigenen Szenen. Szenen verweisen per `canvas_uuid` auf ihren
  Canvas. Gespeichert wird nur Name/UUID/Flags, *nicht* die Auflösung; die setzt
  das Frontend beim Laden.
- Quellen sind canvas-übergreifend: eine Kamera kann gleichzeitig in einer
  Hauptszene und einer vertikalen Szene stecken.

## Ausgaben

```
Canvas-Video ──► Video-Encoder ─┐
Audio-Mix    ──► Audio-Encoder ─┴─► Output (Stream: RTMP/SRT/WHIP · Aufnahme: Datei)
```

Encoder und Outputs werden aus dem **Profil** gebaut, beim Start von Stream
oder Aufnahme.

## Fork-Erweiterung: nativer Vertical-Canvas

| Teil | Wo gespeichert | Inhalt |
|---|---|---|
| Vertical-Canvas + Szenen | Szenensammlung, `canvases` + `sources` | wie oben, Name „Vertical“ (übernimmt „Aitum Vertical“) |
| Vertical-Konfiguration | Szenensammlung, Schlüssel `vertical` | `width`, `height`, `record`, `stream`, `canvas_uuid`, `current_scene_uuid` |
| Szenenpaare | Szenensammlung, `vertical.pairs` | `[{main: <UUID>, vertical: <UUID>}]` |
| Vertical-Aufnahme | zur Laufzeit | klont Encoder/Output der Hauptaufnahme, Datei `<name>-vertical.<ext>` |
| Vertical-Stream | Profil, `vertical_service.json` | `enabled`, `server`, `key`, `video_bitrate`; startet nach dem Hauptstream, eigene Encoder |

Code: `frontend/utility/vertical/` (Logik), `frontend/docks/VerticalCanvasDock.*` (UI).

## Faustregeln

- Layout ändern = Szenensammlung. Qualität/Ziel ändern = Profil.
- Stream-Keys gehören ins Profil (`service.json`), nie in eine Szenensammlung.
- Pro Szene andere Kamera-Position: dieselbe Quelle per „Bestehende
  hinzufügen“ einfügen und Item pro Szene einzeln skalieren. Andere
  Geräte-Einstellungen: eigene Quelle anlegen.
