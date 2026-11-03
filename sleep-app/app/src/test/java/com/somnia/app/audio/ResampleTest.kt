package com.somnia.app.audio

import org.junit.Assert.assertTrue
import org.junit.Test
import kotlin.math.PI
import kotlin.math.sin
import kotlin.math.sqrt
import kotlin.random.Random

class ResampleTest {
    private fun rms(a: ShortArray) = sqrt(a.sumOf { (it / 32768.0) * (it / 32768.0) } / a.size)
    private fun tone(hz: Double, rate: Int, secs: Double = 1.0) =
        ShortArray((rate * secs).toInt()) { (sin(2 * PI * hz * it / rate) * 12000).toInt().toShort() }

    @Test fun keepsVoiceAndRemovesHighFrequencies() {
        val voice = Resample.down(tone(800.0, 48_000), 3)
        val hiss = Resample.down(tone(14_000.0, 48_000), 3)
        assertTrue("voix ${rms(voice)}", rms(voice) > 0.2)
        assertTrue("aigus ${rms(hiss)}", rms(hiss) < 0.02) // pas de repliement
    }

    @Test fun polishRaisesQuietSoundWithoutClipping() {
        val quiet = ShortArray(48_000) { (sin(2 * PI * 300 * it / 48_000.0) * 2000).toInt().toShort() }
        val out = Resample.polish(quiet, 48_000)
        assertTrue(rms(out) > rms(quiet) * 3)
        assertTrue(out.maxOf { it.toInt() } < 32767)
    }

    @Test fun clearVoiceWorksAt48k() {
        val sr = 48_000; val rnd = Random(2)
        val n = sr * 3
        val mixed = ShortArray(n) { i ->
            val v = if (i in sr until 2 * sr) sin(2 * PI * 180 * i / sr) * 0.1 else 0.0
            ((v + rnd.nextDouble(-1.0, 1.0) * 0.05) * 32767).toInt().toShort()
        }
        val out = VoiceEnhancer.enhance(mixed, null, sr)
        fun part(a: ShortArray, r: IntRange) = sqrt(r.sumOf { (a[it] / 32768.0) * (a[it] / 32768.0) } / r.count())
        val before = part(mixed, sr until 2 * sr) / part(mixed, 0 until sr)
        val after = part(out, sr until 2 * sr) / part(out, 0 until sr)
        assertTrue("SNR avant $before après $after", after > before * 2)
    }
}
