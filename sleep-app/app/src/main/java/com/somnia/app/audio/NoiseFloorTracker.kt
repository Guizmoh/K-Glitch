package com.somnia.app.audio

/**
 * Estime en continu le bruit de fond (ventilateur, frigo, rue...) comme le
 * 20e percentile des niveaux des 60 dernières secondes. Un son ponctuel
 * (voix, ronflement) ne déplace donc presque pas le plancher, alors qu'un
 * changement durable d'ambiance est suivi en moins d'une minute.
 */
class NoiseFloorTracker(
    private val windowFrames: Int = 600,
    private val percentile: Float = 0.2f,
    private val updateEvery: Int = 10,
) {
    private val history = FloatArray(windowFrames)
    private var count = 0
    private var index = 0
    private var sinceUpdate = 0

    var floorDb: Float = -60f
        private set

    fun add(db: Float) {
        history[index] = db
        index = (index + 1) % windowFrames
        if (count < windowFrames) count++
        if (++sinceUpdate >= updateEvery || count < updateEvery) {
            sinceUpdate = 0
            val sorted = history.copyOf(count).also { it.sort() }
            floorDb = sorted[((count - 1) * percentile).toInt()]
        }
    }

    /**
     * Pendant un événement on n'alimente l'historique qu'avec le plancher
     * courant afin que de longues paroles ne fassent pas monter le seuil.
     */
    fun addDuringEvent() = add(floorDb)
}
