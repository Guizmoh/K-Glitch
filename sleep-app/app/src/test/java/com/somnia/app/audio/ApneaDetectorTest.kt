package com.somnia.app.audio

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test

class ApneaDetectorTest {
    private val floor = -60f

    /** Fait avancer de [secs] secondes : souffle de [burst] s toutes les [period] s, au niveau [level]. */
    private fun breathe(d: ApneaDetector, secs: Int, period: Double = 4.0, burst: Double = 0.8, level: Float = -45f, tv: Boolean = false): List<ApneaDetector.Episode> {
        val out = ArrayList<ApneaDetector.Episode>()
        for (i in 0 until secs * 10) {
            val inBurst = (i / 10.0) % period < burst
            d.process(if (inBurst) level else floor - 1, floor, tv)?.let { out += it }
        }
        return out
    }

    private fun silence(d: ApneaDetector, secs: Int) = (0 until secs * 10).mapNotNull { d.process(floor - 1, floor, false) }

    private fun gasp(d: ApneaDetector, level: Float = -35f, secs: Double = 1.5): List<ApneaDetector.Episode> {
        val out = ArrayList<ApneaDetector.Episode>()
        repeat((secs * 10).toInt()) { d.process(level, floor, false)?.let { out += it } }
        repeat(5) { d.process(floor - 1, floor, false)?.let { out += it } }
        return out
    }

    @Test fun regularSnoringThenSilenceThenGaspIsAPause() {
        val d = ApneaDetector()
        assertTrue(breathe(d, 40).isEmpty())
        assertEquals("Suivi actif", d.status)
        assertTrue(silence(d, 15).isEmpty())
        val ep = gasp(d)
        assertEquals(1, ep.size)
        assertTrue("silence ${ep[0].gapMs}", ep[0].gapMs in 14_000..19_000)
    }

    @Test fun normalBreathingGivesNoPause() {
        val d = ApneaDetector()
        assertTrue(breathe(d, 600).isEmpty())
    }

    @Test fun shortSilenceIsNotAPause() {
        val d = ApneaDetector()
        breathe(d, 40); silence(d, 6)
        assertTrue(gasp(d).isEmpty())
    }

    @Test fun quietRecoveryIsNotAPause() {
        val d = ApneaDetector()
        breathe(d, 40); silence(d, 15)
        // reprise aussi discrète et courte qu'un souffle normal : pas assez sûr
        assertTrue(gasp(d, level = -45f, secs = 0.8).isEmpty())
    }

    @Test fun noPauseWithoutRegularBreathingFirst() {
        val d = ApneaDetector()
        silence(d, 30)
        assertTrue(gasp(d).isEmpty())
    }

    @Test fun tvPausesTheAnalysisAndItRestartsAfter() {
        val d = ApneaDetector()
        assertTrue(breathe(d, 120, tv = true).isEmpty())
        assertTrue(d.status.startsWith("En pause"))
        // télé éteinte : il faut d'abord retrouver une respiration régulière
        breathe(d, 40)
        assertEquals("Suivi actif", d.status)
        silence(d, 20)
        assertEquals(1, gasp(d).size)
    }
}
