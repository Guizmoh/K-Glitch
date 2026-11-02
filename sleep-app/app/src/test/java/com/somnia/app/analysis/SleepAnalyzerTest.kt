package com.somnia.app.analysis

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertTrue
import org.junit.Test

class SleepAnalyzerTest {
    @Test fun typicalNight() {
        val input = (0 until 480).map { m ->
            val mv = when {
                m < 20 -> 8f              // agité au coucher
                m in 200..205 -> 10f      // réveil nocturne
                m > 465 -> 9f             // lever
                m % 90 in 60..75 -> 0.8f  // sommeil léger
                else -> 0.1f              // sommeil profond / calme
            }
            MinuteInput(m, mv, 0, m in 100..130)
        }
        val a = SleepAnalyzer.analyze(input, 480)
        assertNotNull(a.onsetMinute)
        assertTrue("endormissement ${a.onsetMinute}", a.onsetMinute!! in 15..30)
        assertTrue("réveil ${a.wakeMinute}", a.wakeMinute!! in 455..470)
        assertTrue(a.awakenings >= 1)
        assertTrue(a.deepMinutes > 100)
        assertEquals(31, a.snoringMinutes)
        assertTrue("score ${a.score}", a.score in 50..95)
    }

    @Test fun emptyNight() {
        assertEquals(0, SleepAnalyzer.analyze(emptyList()).score)
    }
}
