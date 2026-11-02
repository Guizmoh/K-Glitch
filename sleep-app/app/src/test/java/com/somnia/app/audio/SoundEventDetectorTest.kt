package com.somnia.app.audio

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test
import kotlin.math.PI
import kotlin.math.sin
import kotlin.random.Random

class SoundEventDetectorTest {
    private val rnd = Random(1)

    private fun noise(amp: Double) = ShortArray(AudioMath.FRAME) { (rnd.nextDouble(-1.0, 1.0) * amp * 32767).toInt().toShort() }
    private fun tone(amp: Double, hz: Double, noiseAmp: Double) = ShortArray(AudioMath.FRAME) {
        ((sin(2 * PI * hz * it / AudioMath.SAMPLE_RATE) * amp + rnd.nextDouble(-1.0, 1.0) * noiseAmp) * 32767).toInt().toShort()
    }

    private fun run(d: SoundEventDetector, frames: List<ShortArray>): List<CapturedEvent> =
        frames.mapNotNull { d.process(it) } + listOfNotNull(d.flush())

    @Test fun steadyBackgroundProducesNoEvent() {
        val d = SoundEventDetector().apply { setSensitivity(6) }
        val events = run(d, List(600) { noise(0.01) })
        assertTrue(events.isEmpty())
    }

    @Test fun voiceAboveBackgroundIsCapturedWithPreRoll() {
        val d = SoundEventDetector().apply { setSensitivity(6) }
        val frames = List(300) { noise(0.01) } + List(20) { tone(0.2, 200.0, 0.01) } + List(50) { noise(0.01) }
        val events = run(d, frames)
        assertEquals(1, events.size)
        val e = events[0]
        // 2 s de son + 1,5 s de pré-enregistrement + 0,3 s de queue (marge pour l'attaque)
        assertTrue("durée ${e.durationMs}", e.durationMs in 3200..4200)
        assertTrue(e.peakDb > e.noiseFloorDb + 20)
        assertTrue(e.noiseSample.isNotEmpty())
        assertTrue("début ${e.startOffsetMs}", e.startOffsetMs in 28_000L..29_000L)
    }

    @Test fun clickIsIgnored() {
        val d = SoundEventDetector().apply { setSensitivity(6) }
        val frames = List(300) { noise(0.01) } + List(2) { tone(0.3, 1000.0, 0.0) } + List(50) { noise(0.01) }
        assertTrue(run(d, frames).isEmpty())
    }

    @Test fun lowSensitivityIgnoresQuietSound() {
        val quiet = List(300) { noise(0.01) } + List(20) { tone(0.03, 200.0, 0.01) } + List(50) { noise(0.01) }
        val low = SoundEventDetector().apply { setSensitivity(1) }
        val high = SoundEventDetector().apply { setSensitivity(10) }
        assertTrue(run(low, quiet).isEmpty())
        assertEquals(1, run(high, quiet).size)
    }

    @Test fun backgroundChangeIsTracked() {
        val t = NoiseFloorTracker()
        repeat(600) { t.add(-60f) }
        assertEquals(-60f, t.floorDb, 0.1f)
        repeat(600) { t.add(-40f) }
        assertEquals(-40f, t.floorDb, 0.1f)
        assertNotNull(t)
    }

    @Test fun longEventIsSplit() {
        val d = SoundEventDetector().apply { setSensitivity(6) }
        val frames = List(300) { noise(0.01) } + List(900) { tone(0.2, 200.0, 0.01) } + List(50) { noise(0.01) }
        val events = run(d, frames)
        assertTrue(events.size >= 2)
        assertNull(d.flush())
    }
}
