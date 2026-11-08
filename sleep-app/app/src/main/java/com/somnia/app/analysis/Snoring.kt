package com.somnia.app.analysis

import com.somnia.app.audio.SoundType
import com.somnia.app.data.MinuteStat
import com.somnia.app.data.SoundEventEntity
import kotlin.math.abs

/** Ronflement de la nuit : courbe par tranches, moment le plus fort, lien avec la position. */
object Snoring {
    /** Une tranche de [minutes] : secondes de ronflement et intensité max (dB au-dessus du fond). */
    data class Bucket(val startMinute: Int, val seconds: Float, val intensityDb: Float)

    fun curve(events: List<SoundEventEntity>, sessionStart: Long, totalMinutes: Int, minutes: Int = 10): List<Bucket> {
        val n = (totalMinutes + minutes - 1) / minutes
        val secs = FloatArray(n); val peak = FloatArray(n)
        for (e in events.filter { it.type == SoundType.SNORING.name }) {
            val m = ((e.timestamp - sessionStart) / 60_000L).toInt()
            val b = (m / minutes).coerceIn(0, n - 1)
            secs[b] += e.durationMs / 1000f
            peak[b] = maxOf(peak[b], e.peakDb - e.noiseFloorDb)
        }
        return (0 until n).map { Bucket(it * minutes, secs[it], peak[it]) }
    }

    /** Fenêtre de 2 h où l'on ronfle le plus (minute de début), ou null. */
    fun worstWindow(curve: List<Bucket>, minutesPerBucket: Int = 10): Int? {
        val w = 120 / minutesPerBucket
        if (curve.isEmpty() || curve.all { it.seconds == 0f }) return null
        return (0..maxOf(0, curve.size - w)).maxByOrNull { i -> curve.subList(i, minOf(curve.size, i + w)).sumOf { it.seconds.toDouble() } }
            ?.let { curve[it].startMinute }
    }

    /** Position de sommeil (d'après l'inclinaison du téléphone posé sur le matelas). */
    data class Position(val index: Int, val minutes: Int, val snoreMinutes: Int) {
        val label get() = "Position ${index + 1}"
    }

    /**
     * Regroupe les minutes par inclinaison voisine (à 4° près). Quand on se retourne,
     * le matelas penche un peu différemment : chaque groupe ≈ une position.
     */
    fun positions(minutes: List<MinuteStat>, events: List<SoundEventEntity>, sessionStart: Long): List<Position> {
        val withPos = minutes.filter { it.hasPosition }
        if (withPos.size < 30) return emptyList()
        val centers = ArrayList<Pair<Float, Float>>()
        val counts = ArrayList<Int>()
        val assign = HashMap<Int, Int>()
        for (m in withPos) {
            val i = centers.indexOfFirst { abs(it.first - m.tilt) < 4f && abs(it.second - m.pitch) < 4f }
            if (i >= 0) { counts[i]++; assign[m.minute] = i }
            else { centers += m.tilt to m.pitch; counts += 1; assign[m.minute] = centers.size - 1 }
        }
        val snoreMin = events.filter { it.type == SoundType.SNORING.name }
            .flatMap { e -> val a = ((e.timestamp - sessionStart) / 60_000L).toInt(); (a..a + (e.durationMs / 60_000L).toInt()).toList() }.toSet()
        // positions occupées au moins 15 min, de la plus longue à la plus courte
        val order = counts.indices.filter { counts[it] >= 15 }.sortedByDescending { counts[it] }.take(4)
        return order.mapIndexed { rank, c ->
            val mins = assign.filterValues { it == c }.keys
            Position(rank, mins.size, mins.count { it in snoreMin })
        }
    }
}
