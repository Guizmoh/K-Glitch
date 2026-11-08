package com.somnia.app.analysis

import com.somnia.app.data.SleepSession
import java.util.Calendar
import kotlin.math.roundToInt
import kotlin.math.sqrt

/** Bilan des 7 derniers jours (comparé à la semaine d'avant). */
data class WeeklyReport(
    val nights: Int,
    val avgSleepMin: Int,
    val prevAvgSleepMin: Int?,
    val avgScore: Int?,
    /** Écart type de l'heure de coucher, en minutes. */
    val bedtimeSpreadMin: Int,
    val avgBedtimeMin: Int,
    val best: SleepSession?,
    val worst: SleepSession?,
    val helped: List<Insights.TagEffect>,
    val hurt: List<Insights.TagEffect>,
) {
    val regularity get() = when {
        bedtimeSpreadMin <= 30 -> "très réguliers"
        bedtimeSpreadMin <= 60 -> "assez réguliers"
        else -> "irréguliers"
    }

    fun summary(): String = buildString {
        append("$nights nuit(s) · sommeil moyen ${com.somnia.app.ui.Fmt.duration(avgSleepMin)}")
        prevAvgSleepMin?.let { val d = avgSleepMin - it; if (kotlin.math.abs(d) >= 10) append(" (${if (d > 0) "+" else "−"}${com.somnia.app.ui.Fmt.duration(kotlin.math.abs(d))})") }
        avgScore?.let { append(" · score $it") }
        append(" · horaires $regularity")
    }

    companion object {
        private const val DAY = 86_400_000L

        private fun bedMinutes(t: Long): Int {
            val c = Calendar.getInstance().apply { timeInMillis = t }
            return (c.get(Calendar.HOUR_OF_DAY) * 60 + c.get(Calendar.MINUTE) + 720) % 1440 // midi = 0 : la nuit ne « coupe » pas minuit
        }

        fun compute(all: List<SleepSession>, now: Long): WeeklyReport? {
            val done = all.filter { it.endTime != null }
            val week = done.filter { it.startTime >= now - 7 * DAY }
            if (week.isEmpty()) return null
            val prev = done.filter { it.startTime in (now - 14 * DAY) until (now - 7 * DAY) }
            fun asleep(s: SleepSession) = s.deepMinutes + s.lightMinutes
            val beds = week.map { bedMinutes(it.startTime) }
            val mean = beds.average()
            val sd = sqrt(beds.map { (it - mean) * (it - mean) }.average()).roundToInt()
            val scored = week.filter { it.score != null }
            val effects = Insights.tagEffects(done.filter { it.startTime >= now - 28 * DAY })
            return WeeklyReport(
                nights = week.size,
                avgSleepMin = week.map(::asleep).average().roundToInt(),
                prevAvgSleepMin = prev.takeIf { it.isNotEmpty() }?.map(::asleep)?.average()?.roundToInt(),
                avgScore = scored.takeIf { it.isNotEmpty() }?.map { it.score!! }?.average()?.roundToInt(),
                bedtimeSpreadMin = sd,
                avgBedtimeMin = ((mean.roundToInt()) + 720) % 1440,
                best = scored.maxByOrNull { it.score!! },
                worst = scored.filter { it != scored.maxByOrNull { s -> s.score!! } }.minByOrNull { it.score!! },
                helped = effects.filter { it.delta >= 5 }.take(2),
                hurt = effects.filter { it.delta <= -5 }.take(2),
            )
        }
    }
}

/** Heure de coucher idéale : se réveiller en fin de cycle (~90 min), après ~15 min d'endormissement. */
object Bedtime {
    const val CYCLE_MIN = 90
    const val FALL_ASLEEP_MIN = 15

    /** Heures de coucher (minutes depuis minuit) pour 6 puis 5 cycles avant [wakeMin]. */
    fun suggestions(wakeMin: Int, cycles: List<Int> = listOf(6, 5)): List<Pair<Int, Int>> =
        cycles.map { c -> c to ((wakeMin - c * CYCLE_MIN - FALL_ASLEEP_MIN) % 1440 + 1440) % 1440 }
}
