"""TARAREO v3: tarareo -> melodía -> letra coherente -> motor de canto.

No se distribuyen pesos de voces. Si falta un motor de canto autorizado, la
salida de audio es el tarareo ORIGINAL, identificado como pista guía.
"""
from __future__ import annotations

import io
import json
import os
import secrets
import unicodedata
import urllib.parse
from typing import Any

import librosa
import numpy as np
import soundfile as sf
from fastapi import FastAPI, File, Form, HTTPException, UploadFile
from fastapi.responses import Response

app = FastAPI(title="TARAREO Professional API", version="0.3.0")
NOTES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
MAJOR = np.array([6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88])
MINOR = np.array([6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17])
MAX_UPLOAD = 15 * 1024 * 1024


class LyricError(Exception):
    """El modelo devolvió una composición inutilizable."""


def letters_only(text: str) -> str:
    normalized = unicodedata.normalize("NFD", text.casefold())
    return "".join(ch for ch in normalized if ch.isalpha() and not unicodedata.combining(ch))


def extract_notes(y: np.ndarray, sr: int) -> list[dict[str, Any]]:
    """Extrae eventos MIDI con su tiempo y duración originales.

    V3 todavía aproxima una sílaba por evento tonal: un melisma puede tener
    varias notas por sílaba. Es una limitación explícita de esta versión.
    """
    hop = 512
    f0, voiced, _ = librosa.pyin(
        y, fmin=librosa.note_to_hz("C2"), fmax=librosa.note_to_hz("C6"),
        sr=sr, frame_length=2048, hop_length=hop,
    )
    valid = voiced & np.isfinite(f0)
    midi = np.where(valid, np.rint(librosa.hz_to_midi(np.nan_to_num(f0, nan=100.0))), -1).astype(int)
    for i in range(1, len(midi) - 1):
        if midi[i - 1] == midi[i + 1] and midi[i] != midi[i - 1]:
            midi[i] = midi[i - 1]
    events = []
    start = 0
    for i in range(1, len(midi) + 1):
        if i == len(midi) or midi[i] != midi[start]:
            dur = (i - start) * hop / sr
            if midi[start] >= 0 and dur >= 0.08:
                events.append({"midi": int(midi[start]),
                               "start": round(start * hop / sr, 3),
                               "dur": round(dur, 3)})
            start = i
    return events


def detect_key(notes: list[dict[str, Any]]) -> str:
    histogram = np.zeros(12)
    for note in notes:
        histogram[note["midi"] % 12] += note["dur"]
    if not histogram.any():
        return "Unknown"
    best_score, best = float("-inf"), "C major"
    for root in range(12):
        for profile, mode in ((MAJOR, "major"), (MINOR, "minor")):
            score = float(np.corrcoef(histogram, np.roll(profile, root))[0, 1])
            if np.isfinite(score) and score > best_score:
                best_score, best = score, f"{NOTES[root]} {mode}"
    return best


def parse_composition(raw: str, count: int) -> tuple[str, list[str]]:
    """Valida estructura, legibilidad y número exacto de sílabas."""
    raw = raw.strip()
    if raw.startswith("```"):
        raw = raw.split("\n", 1)[-1].rsplit("```", 1)[0].strip()
    try:
        data = json.loads(raw)
    except json.JSONDecodeError as exc:
        raise LyricError("La IA no devolvió JSON válido") from exc
    if not isinstance(data, dict) or not isinstance(data.get("lines"), list):
        raise LyricError("Falta el objeto JSON 'lines'")
    lines, syllables = [], []
    for line in data["lines"]:
        if not isinstance(line, dict) or not isinstance(line.get("words"), list):
            raise LyricError("Cada línea necesita 'words'")
        words = []
        for word in line["words"]:
            if not isinstance(word, dict):
                raise LyricError("Palabra sin estructura")
            spelling, parts = word.get("text"), word.get("syllables")
            if not isinstance(spelling, str) or not spelling.strip() or not isinstance(parts, list):
                raise LyricError("Palabra sin texto o sílabas")
            if not parts or any(not isinstance(p, str) or not p.strip() or
                                " " in p.strip() or len(p) > 24 for p in parts):
                raise LyricError("Sílabas vacías o inválidas")
            if letters_only("".join(parts)) != letters_only(spelling):
                raise LyricError(f"Las sílabas no forman la palabra '{spelling}'")
            words.append(spelling.strip())
            syllables.extend(p.strip() for p in parts)
        if words:
            lines.append(" ".join(words))
    if len(syllables) != count:
        raise LyricError(f"La IA devolvió {len(syllables)} sílabas; se necesitan {count}")
    if not lines or len(" ".join(lines)) < max(3, count):
        raise LyricError("La letra es demasiado corta o vacía")
    if count >= 8 and len({s.lower() for s in syllables}) < 3:
        raise LyricError("Demasiadas sílabas repetidas")
    if len(lines) > 1 and len({line.casefold() for line in lines}) == 1:
        raise LyricError("La IA repitió todas las líneas")
    return "\n".join(lines), syllables


def make_prompt(notes: list[dict[str, Any]], *, genre: str, section: str,
                key: str, voice: str, vocal_range: str, bpm: int,
                creativity: int, theme: str, previous_error: str = "") -> str:
    note_data = [{"midi": n["midi"], "seconds": n["dur"]} for n in notes]
    return f"""Eres compositor profesional de canciones ORIGINALES en español.
Género: {genre}. Sección: {section}. Tema: {theme or 'amor, memoria y emoción'}.
Tonalidad estimada: {key}. BPM: {bpm}. Voz: {voice}, registro {vocal_range}.
Creatividad (0-100): {creativity}. Variación: {secrets.token_hex(6)}.
Escribe una letra CON SENTIDO, frases naturales, historia coherente, rima,
acentos cantables y un gancho si es coro. Evita relleno y frases repetidas.
No reproduzcas letras existentes. Divide cada palabra en sílabas del español.
La suma de TODAS las sílabas debe ser EXACTAMENTE {len(notes)}, una por evento
de melodía y en orden, tomando en cuenta duraciones.
Eventos: {json.dumps(note_data, ensure_ascii=False)}
Responde SOLO un JSON válido, sin Markdown, con esta estructura:
{{"lines":[{{"words":[{{"text":"Quiero","syllables":["Quie","ro"]}},
{{"text":"volver","syllables":["vol","ver"]}}]}}]}}
Incluye puntuación en el texto de las palabras, no en las sílabas.
{f'Corrige el error anterior: {previous_error}' if previous_error else ''}
"""


def lyric_provider(prompt: str, creativity: int) -> tuple[str, str]:
    """Nunca sustituye la IA por sílabas fijas sin advertirlo."""
    temperature = max(0.0, min(1.0, creativity / 100))
    if os.getenv("ANTHROPIC_API_KEY"):
        import anthropic
        msg = anthropic.Anthropic().messages.create(
            model=os.getenv("TARAREO_LYRICS_MODEL", "claude-sonnet-4-6"),
            max_tokens=3000, temperature=temperature,
            messages=[{"role": "user", "content": prompt}],
        )
        return "\n".join(block.text for block in msg.content if block.type == "text"), "Claude"
    if os.getenv("TARAREO_OLLAMA_URL") and os.getenv("TARAREO_OLLAMA_MODEL"):
        import httpx
        url = os.environ["TARAREO_OLLAMA_URL"].rstrip("/") + "/api/generate"
        with httpx.Client(timeout=120) as client:
            response = client.post(url, json={
                "model": os.environ["TARAREO_OLLAMA_MODEL"],
                "prompt": prompt, "stream": False, "format": "json",
                "options": {"temperature": temperature},
            })
            response.raise_for_status()
            return response.json()["response"], "Ollama"
    raise HTTPException(
        503, "Configura ANTHROPIC_API_KEY o TARAREO_OLLAMA_URL + TARAREO_OLLAMA_MODEL para generar letras con IA"
    )


def compose(notes: list[dict[str, Any]], **settings: Any) -> tuple[str, list[str], str]:
    error = ""
    for _ in range(3):
        raw, provider = lyric_provider(
            make_prompt(notes, previous_error=error, **settings), settings["creativity"]
        )
        try:
            lyric, syllables = parse_composition(raw, len(notes))
            return lyric, syllables, provider
        except LyricError as exc:
            error = str(exc)
    raise HTTPException(502, f"La IA no encajó las sílabas tras 3 intentos: {error}")


def guide_wav(y: np.ndarray, sr: int) -> bytes:
    """Tarareo original como pista guía; NUNCA se etiqueta como voz IA."""
    audio = librosa.resample(y, orig_sr=sr, target_sr=44100)
    output = io.BytesIO()
    sf.write(output, audio, 44100, format="WAV", subtype="PCM_16")
    return output.getvalue()


async def render_singing(notes: list[dict[str, Any]], syllables: list[str],
                         **settings: Any) -> bytes | None:
    """Adaptador de voz cantada externa con pesos licenciados.

    POST a TARAREO_SINGER_URL/sing con notas MIDI (start/dur), sílabas,
    selección de voz, rango, tonalidad y BPM; respuesta audio/wav.
    """
    singer_url = os.getenv("TARAREO_SINGER_URL", "").strip()
    if not singer_url:
        return None
    import httpx
    try:
        async with httpx.AsyncClient(timeout=180) as client:
            response = await client.post(singer_url.rstrip("/") + "/sing", json={
                "notes": notes, "syllables": syllables, **settings, "sample_rate": 44100,
            })
            response.raise_for_status()
            if "audio/wav" not in response.headers.get("content-type", ""):
                raise ValueError("El motor no devolvió audio/wav")
            sf.info(io.BytesIO(response.content))
            return response.content if len(response.content) >= 128 else None
    except (httpx.HTTPError, ValueError, RuntimeError) as exc:
        print("TARAREO: motor de voz no disponible:", exc)
        return None


@app.get("/health")
def health() -> dict[str, Any]:
    return {
        "version": "0.3.0",
        "lyrics_ai": bool(os.getenv("ANTHROPIC_API_KEY") or
                          (os.getenv("TARAREO_OLLAMA_URL") and os.getenv("TARAREO_OLLAMA_MODEL"))),
        "singing_engine": bool(os.getenv("TARAREO_SINGER_URL")),
        "singing_bundled": False,
    }


@app.post("/generate")
async def generate(
    file: UploadFile = File(...), genre: str = Form("Bachata"),
    section: str = Form("Coro"), voice: str = Form("Masculina"),
    vocal_range: str = Form("Baritono"), bpm: int = Form(132),
    creativity: int = Form(65), theme: str = Form(""),
):
    data = await file.read(MAX_UPLOAD + 1)
    if len(data) > MAX_UPLOAD:
        raise HTTPException(413, "Archivo demasiado grande (máximo 15 MB)")
    if not 60 <= bpm <= 220 or not 0 <= creativity <= 100:
        raise HTTPException(422, "BPM o creatividad fuera de rango")
    try:
        y, sr = librosa.load(io.BytesIO(data), sr=22050, mono=True)
    except Exception as exc:
        raise HTTPException(422, "No se pudo leer el audio") from exc
    if len(y) < sr // 2 or len(y) > 31 * sr:
        raise HTTPException(422, "Graba entre 0.5 y 30 segundos")
    notes = extract_notes(y, sr)
    if not notes:
        raise HTTPException(422, "No se detectaron notas claras. Intenta tararear más cerca")
    if len(notes) > 160:
        raise HTTPException(422, "Demasiados cambios de nota: intenta frases más cortas")
    key = detect_key(notes)
    settings = dict(genre=genre[:40], section=section[:40], voice=voice[:30],
                    vocal_range=vocal_range[:40], bpm=bpm,
                    creativity=creativity, theme=theme[:160], key=key)
    try:
        lyrics, syllables, provider = compose(notes, **settings)
    except HTTPException:
        raise
    except Exception as exc:
        print("TARAREO: fallo del modelo de letras:", exc)
        raise HTTPException(502, "Falló el modelo de letras. Comprueba la configuración") from exc

    rendered = await render_singing(notes, syllables, **settings)
    singing = rendered is not None
    return Response(rendered if singing else guide_wav(y, sr), media_type="audio/wav", headers={
        "X-Lyrics": urllib.parse.quote(lyrics, safe=""),
        "X-Key": urllib.parse.quote(key, safe=""),
        "X-Audio-Mode": "singing" if singing else "guide",
        "X-Lyrics-Model": provider,
        "X-Note-Count": str(len(notes)),
    })
