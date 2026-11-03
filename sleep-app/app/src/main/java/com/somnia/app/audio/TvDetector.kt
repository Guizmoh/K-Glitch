package com.somnia.app.audio

/**
 * Repère une télé / radio allumée : un son présent presque en continu
 * pendant plusieurs minutes, à un volume régulier (les émissions sont
 * « compressées »), alors qu'une voix humaine dans la chambre arrive par
 * à-coups.
 *
 * Tant que la télé est repérée, [tvLevelDb] donne son niveau typique : le
 * détecteur place alors son seuil au-dessus (voir SoundEventDetector).
 */
class TvDetector(
    private val windowSec: Int = 150,
    private val onFraction: Float = 0.65f,
    private val offFraction: Float = 0.25f,
    private val activeAboveFloorDb: Float = 6f,
    private val maxSpreadDb: Float = 14f,
) {
    private val levels = ArrayDeque<Float>()
    private val actives = ArrayDeque<Boolean>()
    private var secSum = 0.0
    private var secCount = 0

    var isOn = false
        private set
    var tvLevelDb = -96f
        private set

    /** Une trame de 100 ms ; [floorDb] = bruit de fond « calme » actuel. */
    fun process(db: Float, floorDb: Float) {
        secSum += db; secCount++
        if (secCount < 10) return
        val sec = (secSum / secCount).toFloat()
        secSum = 0.0; secCount = 0
        levels.addLast(sec); actives.addLast(sec > floorDb + activeAboveFloorDb)
        while (levels.size > windowSec) { levels.removeFirst(); actives.removeFirst() }
        if (levels.size < 60) return

        val fraction = actives.count { it }.toFloat() / actives.size
        val active = levels.filterIndexed { i, _ -> actives.elementAt(i) }.sorted()
        if (active.isEmpty()) { isOn = false; return }
        val p20 = active[(active.size * 0.2f).toInt()]
        val p80 = active[(active.size * 0.8f).toInt().coerceAtMost(active.size - 1)]
        val steady = p80 - p20 < maxSpreadDb
        if (!isOn && fraction >= onFraction && steady) isOn = true
        else if (isOn && fraction <= offFraction) isOn = false
        tvLevelDb = if (isOn) active[(active.size * 0.7f).toInt().coerceAtMost(active.size - 1)] else -96f
    }
}
