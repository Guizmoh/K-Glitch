package com.somnia.app.audio

import org.junit.Assert.assertTrue
import org.junit.Test
import kotlin.math.PI
import kotlin.math.sin
import kotlin.math.sqrt
import kotlin.random.Random

class VoiceEnhancerTest {
    @Test fun improvesVoiceToNoiseRatio() {
        val sr = AudioMath.SAMPLE_RATE
        val rnd = Random(3)
        val n = sr * 3
        // « Voix » : 1 s de son harmonique au milieu de 3 s de souffle.
        val voice = DoubleArray(n) { i ->
            if (i in sr until 2 * sr) (sin(2 * PI * 180 * i / sr) + 0.5 * sin(2 * PI * 360 * i / sr)) * 0.1 else 0.0
        }
        val noise = DoubleArray(n) { rnd.nextDouble(-1.0, 1.0) * 0.05 }
        val mixed = ShortArray(n) { ((voice[it] + noise[it]) * 32767).toInt().toShort() }
        val noiseOnly = ShortArray(sr) { (rnd.nextDouble(-1.0, 1.0) * 0.05 * 32767).toInt().toShort() }

        val out = VoiceEnhancer.enhance(mixed, noiseOnly)
        fun rms(a: ShortArray, r: IntRange) = sqrt(r.sumOf { (a[it] / 32768.0) * (a[it] / 32768.0) } / r.count())
        val before = rms(mixed, sr until 2 * sr) / rms(mixed, 0 until sr)
        val after = rms(out, sr until 2 * sr) / rms(out, 0 until sr)
        assertTrue("SNR avant $before après $after", after > before * 3)
    }
}
