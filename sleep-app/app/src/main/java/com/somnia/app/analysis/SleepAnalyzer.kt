package com.somnia.app.analysis

import kotlin.math.roundToInt

enum class Stage(val label: String) { DEEP("Profond"), LIGHT("Léger"), AWAKE("Éveillé") }

data class MinuteInput(
    val minute: Int,
    val movement: Float,
    /** Événements sonores dérangeants (paroles, cris, toux, pleurs...) dans la minute. */
    val disturbances: Int,
    val snoring: Boolean,
)

data class SleepAnalysis(
    val stages: List<Stage>,
    val onsetMinute: Int?,
    val wakeMinute: Int?,
    val deepMinutes: Int,
    val lightMinutes: Int,
    val awakeMinutes: Int,
    val awakenings: Int,
    val snoringMinutes: Int,
    val score: Int,
) {
    val asleepMinutes get() = deepMinutes + lightMinutes
}

/**
 * Estimation des phases de sommeil par actigraphie (mouvements du matelas
 * captés par l'accéléromètre), complétée par les sons. Inspirée de
 * l'algorithme de Cole-Kripke : on lisse l'activité sur une fenêtre de
 * 7 minutes ; au-dessus d'un seuil on considère la personne éveillée.
 *
 * C'est une estimation, pas un diagnostic : seule une polysomnographie
 * mesure réellement les stades du sommeil.
 */
object SleepAnalyzer {
    private val WEIGHTS = floatArrayOf(0.04f, 0.06f, 0.1f, 0.2f, 0.4f, 0.1f, 0.04f) // m-4 .. m+2

    fun analyze(input: List<MinuteInput>, goalMinutes: Int = 480): SleepAnalysis {
        val n = input.size
        if (n == 0) return SleepAnalysis(emptyList(), null, null, 0, 0, 0, 0, 0, 0)
        val act = FloatArray(n) { input[it].movement + 2f * input[it].disturbances }
        val sorted = act.sorted()
        val median = sorted[n / 2]
        val p25 = sorted[n / 4]
        val awakeThr = maxOf(median * 3f, 1.5f)

        val smooth = FloatArray(n) { m ->
            var s = 0f
            for (k in WEIGHTS.indices) {
                val idx = m - 4 + k
                if (idx in 0 until n) s += WEIGHTS[k] * act[idx]
            }
            s
        }
        val stages = Array(n) { if (smooth[it] > awakeThr) Stage.AWAKE else Stage.LIGHT }

        // Sommeil profond : au moins 10 minutes calmes d'affilée, sans bruit.
        var runStart = -1
        for (m in 0..n) {
            val calm = m < n && stages[m] != Stage.AWAKE && act[m] <= maxOf(p25, 0.3f) && input[m].disturbances == 0
            if (calm && runStart < 0) runStart = m
            if (!calm && runStart >= 0) {
                if (m - runStart >= 10) for (k in runStart until m) stages[k] = Stage.DEEP
                runStart = -1
            }
        }

        // Endormissement : début de la première période de 10 min sans éveil.
        var onset: Int? = null
        run {
            var c = 0
            for (m in 0 until n) {
                c = if (stages[m] != Stage.AWAKE) c + 1 else 0
                if (c >= 10) { onset = m - 9; return@run }
            }
        }
        val start = onset ?: return SleepAnalysis(stages.toList(), null, null, 0, 0, n, 0, 0, 0)
        var wake = n
        while (wake > start && stages[wake - 1] == Stage.AWAKE) wake--
        for (m in 0 until start) stages[m] = Stage.AWAKE
        for (m in wake until n) stages[m] = Stage.AWAKE

        var deep = 0; var light = 0; var awake = 0; var awakenings = 0; var snoring = 0
        var awakeRun = 0
        for (m in start until wake) {
            when (stages[m]) {
                Stage.DEEP -> deep++
                Stage.LIGHT -> light++
                Stage.AWAKE -> awake++
            }
            if (stages[m] == Stage.AWAKE) awakeRun++ else { if (awakeRun >= 2) awakenings++; awakeRun = 0 }
            if (input[m].snoring) snoring++
        }
        val disturbances = input.subList(start, wake).sumOf { it.disturbances }

        val asleep = deep + light
        val inBed = n
        val durationPts = 40f * (asleep.toFloat() / goalMinutes).coerceAtMost(1f)
        val efficiency = asleep.toFloat() / inBed
        val efficiencyPts = 25f * ((efficiency - 0.70f) / 0.25f).coerceIn(0f, 1f)
        val deepPts = if (asleep == 0) 0f else 15f * ((deep.toFloat() / asleep) / 0.2f).coerceAtMost(1f)
        val calmPts = (20f - awakenings * 3f - snoring / 10f - disturbances / 2f).coerceIn(0f, 20f)
        val score = (durationPts + efficiencyPts + deepPts + calmPts).roundToInt().coerceIn(0, 100)

        return SleepAnalysis(
            stages = stages.toList(), onsetMinute = start, wakeMinute = wake,
            deepMinutes = deep, lightMinutes = light, awakeMinutes = awake,
            awakenings = awakenings, snoringMinutes = snoring, score = score,
        )
    }
}
