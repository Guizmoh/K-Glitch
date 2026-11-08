package com.somnia.app.analysis

import com.somnia.app.audio.AlarmRinger
import com.somnia.app.audio.Ambience
import com.somnia.app.audio.AmbienceGenerator
import com.somnia.app.data.MinuteStat
import com.somnia.app.data.SleepSession
import com.somnia.app.data.SoundEventEntity
import com.somnia.app.ui.screens.CalibrationResult
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertTrue
import org.junit.Test

class NewFeaturesTest {
    private val t0 = 1_761_950_000_000L
    private fun snore(min: Int, secs: Int = 30, db: Float = -40f) =
        SoundEventEntity(0, 1, t0 + min * 60_000L, secs * 1000L, "SNORING", 0.7f, db, -60f, "", null, null)

    @Test fun snoreCurveBucketsAndWorstWindow() {
        val ev = (120 until 240 step 5).map { snore(it) } + listOf(snore(20))
        val curve = Snoring.curve(ev, t0, 480)
        assertEquals(48, curve.size)
        assertTrue(curve[12].seconds > 0)
        assertEquals(20f, curve[12].intensityDb, 0.01f)
        assertEquals(120, Snoring.worstWindow(curve))
    }

    @Test fun snoringIsLinkedToPosition() {
        // 0-239 : position A (inclinaison 0°), 240-479 : position B (8°) ; on ne ronfle qu'en B
        val minutes = (0 until 480).map { MinuteStat(1, it, 0.2f, -55f, -50f, 0, tilt = if (it < 240) 0f else 8f, pitch = 2f, hasPosition = true) }
        val ev = (250 until 470 step 3).map { snore(it) }
        val pos = Snoring.positions(minutes, ev, t0)
        assertEquals(2, pos.size)
        val b = pos.maxBy { it.snoreMinutes }
        assertEquals(240, b.minutes)
        assertTrue(pos.minBy { it.snoreMinutes }.snoreMinutes == 0)
    }

    @Test fun noPositionWithoutAccelerometer() {
        val minutes = (0 until 300).map { MinuteStat(1, it, 0f, -55f, -50f, 0) }
        assertTrue(Snoring.positions(minutes, emptyList(), t0).isEmpty())
    }

    @Test fun idealBedtimeInCycles() {
        val s = Bedtime.suggestions(7 * 60) // réveil 7:00
        assertEquals(6 to (21 * 60 + 45), s[0]) // 9 h + 15 min
        assertEquals(5 to (23 * 60 + 15), s[1])
        assertEquals(6 to (22 * 60 + 45), Bedtime.suggestions(8 * 60)[0])
    }

    @Test fun weeklyReportSummary() {
        val day = 86_400_000L
        val now = t0 + 8 * day
        val nights = (1..7).map { i ->
            SleepSession(id = i.toLong(), startTime = now - i * day + (i % 2) * 20 * 60_000L, endTime = now - i * day + 8 * 3_600_000L,
                deepMinutes = 90, lightMinutes = 330 + i * 5, awakeMinutes = 20, score = 60 + i * 3, tags = if (i % 2 == 0) "Sport" else "Caféine")
        }
        val w = WeeklyReport.compute(nights, now)
        assertNotNull(w); w!!
        assertEquals(7, w.nights)
        assertTrue(w.bedtimeSpreadMin <= 30)
        assertEquals("très réguliers", w.regularity)
        assertEquals(7L, w.best!!.id)
        assertEquals(1L, w.worst!!.id)
        assertTrue(w.summary().contains("7 nuit(s)"))
    }

    @Test fun calibrationPicksSensitivity() {
        val quiet = List(600) { -66f + (it % 3) }
        assertEquals(5, CalibrationResult.from(quiet).sensitivity)
        val noisy = List(600) { -40f + (it % 4) }
        assertEquals(2, CalibrationResult.from(noisy).sensitivity)
        // télé : fort presque tout le temps, volume régulier
        val tv = List(600) { if (it % 10 == 0) -62f else -42f + (it % 5) }
        assertTrue(CalibrationResult.from(tv).suggestTv)
        assertTrue(!CalibrationResult.from(quiet).suggestTv)
    }

    @Test fun ambiencesStayInRangeAndAreNotSilent() {
        for (a in Ambience.entries) {
            val g = AmbienceGenerator(a)
            var energy = 0.0
            repeat(22_050) { val v = g.next(); assertTrue(v in -1f..1f); energy += v * v }
            assertTrue("$a trop faible", energy / 22_050 > 1e-4)
        }
    }

    @Test fun alarmRampsUpOverOneMinute() {
        assertEquals(0.05f, AlarmRinger.ramp(0), 0.001f)
        assertTrue(AlarmRinger.ramp(30_000) in 0.45f..0.6f)
        assertEquals(1f, AlarmRinger.ramp(90_000), 0.001f)
    }
}
