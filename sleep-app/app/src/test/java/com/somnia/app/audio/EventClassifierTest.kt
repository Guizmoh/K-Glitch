package com.somnia.app.audio

import org.junit.Assert.assertEquals
import org.junit.Test

class EventClassifierTest {
    private class Fixed(val s: SoundScores) : SoundClassifier {
        override val name = "test"
        override fun classify(window: FloatArray) = s
    }

    @Test fun picksTypeAboveThreshold() {
        val c = EventClassifier(Fixed(mapOf(SoundType.LAUGHTER to 0.5f, SoundType.SPEECH to 0.2f)))
        assertEquals(SoundType.LAUGHTER, c.classify(ShortArray(20_000)).type)
    }

    @Test fun belowThresholdIsNoise() {
        val c = EventClassifier(Fixed(mapOf(SoundType.CRYING to 0.15f)))
        assertEquals(SoundType.NOISE, c.classify(ShortArray(8000)).type)
    }

    @Test fun higherSensitivityCatchesWeakCrying() {
        val c = EventClassifier(
            Fixed(mapOf(SoundType.CRYING to 0.15f)),
            EventClassifier.defaultThresholds() + (SoundType.CRYING to EventClassifier.thresholdFromLevel(10)),
        )
        assertEquals(SoundType.CRYING, c.classify(ShortArray(8000)).type)
    }

    @Test fun nightTerrorHasPriorityOverCloseSpeech() {
        val c = EventClassifier(Fixed(mapOf(SoundType.SPEECH to 0.6f, SoundType.NIGHT_TERROR to 0.5f)))
        assertEquals(SoundType.NIGHT_TERROR, c.classify(ShortArray(8000)).type)
    }

    @Test fun heuristicFallbackRecognisesLowRumbleAsSnoring() {
        val h = HeuristicClassifier()
        val rnd = java.util.Random(5)
        // Bruit filtré passe-bas (~100 Hz) modulé : ressemble à un ronflement.
        var y = 0.0
        val w = FloatArray(SoundClassifier.WINDOW) {
            y = 0.97 * y + 0.03 * rnd.nextGaussian()
            (y * 2).toFloat()
        }
        val s = h.classify(w)
        val best = s.maxBy { it.value }.key
        assertEquals(SoundType.SNORING, best)
    }
}
