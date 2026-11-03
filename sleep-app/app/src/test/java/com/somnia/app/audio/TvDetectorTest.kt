package com.somnia.app.audio

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import kotlin.random.Random

class TvDetectorTest {
    private val rnd = Random(4)
    private fun feedSeconds(tv: TvDetector, secs: Int, level: (Int) -> Float, floor: Float = -60f) {
        for (s in 0 until secs) repeat(10) { tv.process(level(s), floor) }
    }

    @Test fun continuousTvIsDetectedThenReleased() {
        val tv = TvDetector()
        // Télé : son presque continu autour de -38 dB (pauses courtes)
        feedSeconds(tv, 150, { if (it % 9 == 0) -58f else -38f + rnd.nextFloat() * 6f })
        assertTrue(tv.isOn)
        assertTrue("niveau ${tv.tvLevelDb}", tv.tvLevelDb in -40f..-30f)
        // Télé éteinte : silence
        feedSeconds(tv, 150, { -61f })
        assertFalse(tv.isOn)
    }

    @Test fun occasionalVoiceIsNotTv() {
        val tv = TvDetector()
        // Quelqu'un parle de temps en temps (20 % du temps)
        feedSeconds(tv, 150, { if (it % 5 == 0) -35f else -60f })
        assertFalse(tv.isOn)
    }

    @Test fun detectorThresholdRisesAboveTv() {
        val d = SoundEventDetector().apply { setSensitivity(4) }
        val before = d.threshold
        d.mediaLevelDb = -35f
        assertEquals(-27f, d.threshold, 0.01f)
        assertTrue(d.threshold > before)
    }

    @Test fun mediaWinsWhenStrongest() {
        val c = EventClassifier(object : SoundClassifier {
            override val name = "t"
            override fun classify(window: FloatArray) = mapOf(SoundType.MEDIA to 0.6f, SoundType.SPEECH to 0.4f)
        })
        assertEquals(SoundType.MEDIA, c.classify(ShortArray(8000)).type)
    }
}
