"""Backend de TARAREO: tarareo (wav) -> notas -> letra -> voz cantada (wav).

Ejecutar:  uvicorn server:app --host 127.0.0.1 --port 8000
Opcional:  set ANTHROPIC_API_KEY=...   (letras reales con IA)
"""
import io, json, os, urllib.parse
import numpy as np, librosa, soundfile as sf
from fastapi import FastAPI, File, Form, UploadFile, HTTPException
from fastapi.responses import Response

app = FastAPI(title="TARAREO backend")

NOTE = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
MAJ = np.array([6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88])
MIN = np.array([6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17])


def extract_notes(y, sr):
    hop = 512
    f0, voiced, _ = librosa.pyin(y, fmin=librosa.note_to_hz("C2"), fmax=librosa.note_to_hz("C6"),
                                 sr=sr, frame_length=2048, hop_length=hop)
    ok = voiced & ~np.isnan(f0)
    midi = np.where(ok, np.round(librosa.hz_to_midi(np.nan_to_num(f0, nan=100.0))), -1).astype(int)
    notes, start = [], 0
    for i in range(1, len(midi) + 1):
        if i == len(midi) or midi[i] != midi[start]:
            dur = (i - start) * hop / sr
            if midi[start] >= 0 and dur >= 0.08:
                notes.append({"midi": int(midi[start]), "start": start * hop / sr, "dur": dur})
            start = i
    return notes


def detect_key(notes):
    hist = np.zeros(12)
    for n in notes:
        hist[n["midi"] % 12] += n["dur"]
    best, name = -2, "C major"
    for t in range(12):
        for prof, mode in ((MAJ, "major"), (MIN, "minor")):
            c = np.corrcoef(hist, np.roll(prof, t))[0, 1]
            if c > best:
                best, name = c, f"{NOTE[t]} {mode}"
    return name


def make_lyrics(n, genre, key):
    """Devuelve una lista de n silabas. Con ANTHROPIC_API_KEY usa IA; si no, relleno de prueba."""
    if os.getenv("ANTHROPIC_API_KEY"):
        try:
            import anthropic
            msg = anthropic.Anthropic().messages.create(
                model=os.getenv("TARAREO_MODEL", "claude-sonnet-4-6"), max_tokens=1500,
                messages=[{"role": "user", "content":
                    f"Eres letrista de {genre} en español, tonalidad {key}. Escribe una letra cantable "
                    f"con EXACTAMENTE {n} silabas, una por nota, con sentido y buena rima. "
                    f"Responde SOLO con un arreglo JSON de {n} strings (cada string es una silaba)."}])
            data = json.loads(msg.content[0].text)
            if isinstance(data, list) and len(data) == n:
                return [str(s) for s in data]
        except Exception as e:
            print("Fallo la IA de letras:", e)
    base = ["a", "mor", "te", "quie", "ro", "ba", "jar", "la", "luz"]
    return [base[i % len(base)] for i in range(n)]


def sing(notes, sr=44100):
    """PLACEHOLDER: sintetiza un tono tipo voz con la melodia detectada.
    Aqui se conecta el motor real de voz cantada (DiffSinger / RVC entrenado con tu voz),
    pasandole notes + silabas para que cante la letra."""
    total = max(n["start"] + n["dur"] for n in notes) + 0.3
    out = np.zeros(int(total * sr))
    for n in notes:
        f = 440.0 * 2 ** ((n["midi"] - 69) / 12)
        t = np.arange(int(n["dur"] * sr)) / sr
        w = sum(np.sin(2 * np.pi * f * k * t) / k for k in range(1, 7))
        env = np.minimum(1, t / 0.02) * np.minimum(1, (n["dur"] - t) / 0.05)
        i = int(n["start"] * sr)
        out[i:i + len(t)] += 0.2 * w * env
    return out / max(1e-6, np.abs(out).max()) * 0.8


@app.post("/generate")
async def generate(file: UploadFile = File(...), genre: str = Form("Reggaeton")):
    y, sr = librosa.load(io.BytesIO(await file.read()), sr=22050, mono=True)
    notes = extract_notes(y, sr)
    if not notes:
        raise HTTPException(422, "No detecte notas")
    key = detect_key(notes)
    syl = make_lyrics(len(notes), genre, key)
    audio = sing(notes)
    buf = io.BytesIO()
    sf.write(buf, audio, 44100, format="WAV")
    return Response(buf.getvalue(), media_type="audio/wav", headers={
        "X-Lyrics": urllib.parse.quote(" ".join(syl)), "X-Key": urllib.parse.quote(key)})
