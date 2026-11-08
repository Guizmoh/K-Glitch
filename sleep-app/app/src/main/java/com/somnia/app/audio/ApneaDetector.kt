package com.somnia.app.audio

/**
 * Repère les pauses respiratoires suspectes au son :
 *   respiration / ronflement réguliers -> silence de 10 à 90 s -> reprise bruyante.
 *
 * Pour limiter les fausses alertes :
 *  - les « souffles » sont repérés en continu, à +5 dB au-dessus du fond (indépendant
 *    de la sensibilité du micro) ;
 *  - une pause ne compte que si au moins 4 souffles réguliers (2 à 10 s d'écart) la précèdent ;
 *  - le silence doit vraiment rester au niveau du fond ;
 *  - la reprise doit être forte (pic nettement au-dessus des souffles habituels) ou longue ;
 *  - tant qu'une télé / radio est repérée, l'analyse est en pause et repart de zéro ensuite.
 *
 * Ce n'est pas un diagnostic : seule une polygraphie / polysomnographie le permet.
 */
class ApneaDetector(
    private val burstAboveFloorDb: Float = 5f,
    private val minGapMs: Long = 10_000,
    private val maxGapMs: Long = 90_000,
) {
    /** Une pause suspecte : début et fin du silence, force de la reprise. */
    data class Episode(val silenceStartMs: Long, val silenceEndMs: Long, val recoveryPeakDb: Float, val recoveryMs: Long) {
        val gapMs get() = silenceEndMs - silenceStartMs
    }

    private class Burst(val start: Long, var end: Long, var peak: Float)

    private val frameMs = 100L
    private var t = 0L
    private var loudRun = 0
    private var quietRun = 0
    private var current: Burst? = null
    private val bursts = ArrayDeque<Burst>()

    /** « Suivi actif », « En pause (télé) »… pour l'écran de nuit. */
    var status = "En attente d'une respiration régulière"
        private set

    /** Une trame de 100 ms. Renvoie un épisode quand la reprise se termine. */
    fun process(db: Float, floorDb: Float, mediaOn: Boolean): Episode? {
        t += frameMs
        if (mediaOn) {
            bursts.clear(); current = null; loudRun = 0; quietRun = 0
            status = "En pause : télé ou radio allumée"
            return null
        }
        val loud = db > floorDb + burstAboveFloorDb
        var out: Episode? = null
        val c = current
        if (c == null) {
            loudRun = if (loud) loudRun + 1 else 0
            if (loudRun >= 2) current = Burst(t - 2 * frameMs, t, db)
        } else {
            if (loud) { quietRun = 0; c.end = t; if (db > c.peak) c.peak = db }
            else if (++quietRun >= 3) {
                out = closeBurst(c)
                current = null; quietRun = 0; loudRun = 0
            }
        }
        // ménage : on garde ~60 s d'historique
        while (bursts.isNotEmpty() && t - bursts.first().end > 60_000) bursts.removeFirst()
        if (current == null && regular() && bursts.isNotEmpty() && t - bursts.last().end >= minGapMs) {
            status = "Silence en cours (${(t - bursts.last().end) / 1000} s)"
        } else if (regular()) status = "Suivi actif"
        else if (status.startsWith("En pause")) status = "En attente d'une respiration régulière"
        return out
    }

    private fun closeBurst(b: Burst): Episode? {
        val prev = bursts.lastOrNull()
        var ep: Episode? = null
        if (prev != null && regular()) {
            val gap = b.start - prev.end
            if (gap in minGapMs..maxGapMs) {
                val usual = bursts.map { it.peak }.sorted().let { it[it.size / 2] }
                val strong = b.peak >= usual + 3f || (b.end - b.start) >= 1_200
                if (strong) ep = Episode(prev.end, b.start, b.peak, b.end - b.start)
            }
        }
        if (prev != null && b.start - prev.end > maxGapMs) bursts.clear() // longue pause : réveil, on repart de zéro
        bursts.addLast(b)
        return ep
    }

    /** Respiration régulière : ≥ 4 souffles récents espacés de 2 à 10 s, rythme assez stable. */
    private fun regular(): Boolean {
        if (bursts.size < 4) return false
        val recent = bursts.toList().takeLast(6)
        val gaps = recent.zipWithNext { a, b -> (b.start - a.start).toFloat() }
        if (gaps.any { it < 2_000 || it > 10_000 }) return false
        val mean = gaps.average().toFloat()
        val sd = kotlin.math.sqrt(gaps.map { (it - mean) * (it - mean) }.average()).toFloat()
        return sd / mean < 0.45f
    }
}
