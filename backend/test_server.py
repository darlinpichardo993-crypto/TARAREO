"""Deterministic tests for TARAREO's composition contract (no API key needed)."""
import json
import io
import os
import asyncio
import unittest
from unittest.mock import AsyncMock, patch

from fastapi import HTTPException
import numpy as np
import soundfile as sf

from server import LyricError, compose, make_prompt, parse_composition, lyric_provider, generate


def song(*words):
    return json.dumps({"lines": [{"words": [
        {"text": word, "syllables": parts} for word, parts in words
    ]}]}, ensure_ascii=False)


class CompositionTests(unittest.TestCase):
    def setUp(self):
        self.events = [{"midi": 60 + i, "start": i * 0.25, "dur": 0.25} for i in range(6)]
        self.settings = dict(genre="Bachata", section="Coro", key="C major",
                             voice="Masculina", vocal_range="Baritono",
                             bpm=132, creativity=65, theme="un amor lejano")

    def test_displayed_words_and_exact_syllables(self):
        text = song(("Quiero", ["Quie", "ro"]), ("volverte", ["vol", "ver", "te"]),
                    ("a", ["a"]))
        lyrics, syllables = parse_composition(text, 6)
        self.assertEqual(lyrics, "Quiero volverte a")
        self.assertEqual(len(syllables), 6)

    def test_rejects_mismatched_syllables(self):
        with self.assertRaisesRegex(LyricError, "se necesitan 6"):
            parse_composition(song(("Te", ["Te"])), 6)

    def test_rejects_syllables_that_do_not_form_a_word(self):
        with self.assertRaisesRegex(LyricError, "no forman"):
            parse_composition(song(("Quiero", ["La", "la"])), 2)

    def test_rejects_repetitive_filler(self):
        with self.assertRaises(LyricError):
            parse_composition(song(("La", ["la"] * 8)), 8)

    def test_retry_on_bad_model_output(self):
        good = song(("Quiero", ["Quie", "ro"]), ("volverte", ["vol", "ver", "te"]),
                    ("a", ["a"]))
        with patch("server.lyric_provider", side_effect=[("{}", "mock"), (good, "mock")]) as model:
            lyrics, syllables, provider = compose(self.events, **self.settings)
        self.assertEqual(model.call_count, 2)
        self.assertEqual(provider, "mock")
        self.assertEqual(len(syllables), 6)
        self.assertIn("Quiero", lyrics)

    def test_rejects_three_invalid_outputs(self):
        with patch("server.lyric_provider", return_value=("{}", "mock")):
            with self.assertRaises(HTTPException) as ctx:
                compose(self.events, **self.settings)
        self.assertEqual(ctx.exception.status_code, 502)

    def test_prompt_uses_user_choices(self):
        prompt = make_prompt(self.events, **self.settings)
        for required in ("Bachata", "Coro", "Baritono", "132", "un amor lejano",
                         "EXACTAMENTE 6"):
            self.assertIn(required, prompt)

    def test_missing_ai_is_reported_not_faked(self):
        with patch.dict(os.environ, {}, clear=True):
            with self.assertRaises(HTTPException) as ctx:
                lyric_provider("Compose", 65)
        self.assertEqual(ctx.exception.status_code, 503)


class EndpointTests(unittest.TestCase):
    def setUp(self):
        out = io.BytesIO()
        samples = np.zeros(22050, dtype=np.float32)
        sf.write(out, samples, 22050, format="WAV")
        self.wav = out.getvalue()

    def call_generation(self, data):
        class InMemoryUpload:
            async def read(self, size):
                return data[:size]
        file = InMemoryUpload()
        return asyncio.run(generate(file=file, genre="Bachata", section="Coro",
                                    voice="Masculina", vocal_range="Baritono",
                                    bpm=132, creativity=65, theme="amor"))

    def test_guide_mode_is_truthfully_labeled(self):
        notes = [{"midi": 60, "start": 0.0, "dur": 1.0}]
        with patch("server.extract_notes", return_value=notes), \
             patch("server.compose", return_value=("Te amo", ["Te"], "mock")), \
             patch("server.render_singing", new_callable=AsyncMock, return_value=None):
            response = self.call_generation(self.wav)
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.headers["x-audio-mode"], "guide")
        self.assertIn("Te%20amo", response.headers["x-lyrics"])
        self.assertTrue(response.body.startswith(b"RIFF"))

    def test_uploaded_audio_max_size(self):
        with self.assertRaises(HTTPException) as ctx:
            self.call_generation(b"0" * (15 * 1024 * 1024 + 1))
        self.assertEqual(ctx.exception.status_code, 413)


if __name__ == "__main__":
    unittest.main()
