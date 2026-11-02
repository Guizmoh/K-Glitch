package com.somnia.app.audio

/** Un passage sonore qui se détache du bruit de fond. */
class CapturedEvent(
    /** Position du début (pré-enregistrement inclus) en ms depuis le début du suivi. */
    val startOffsetMs: Long,
    val pcm: ShortArray,
    val peakDb: Float,
    val noiseFloorDb: Float,
    /** Échantillon de bruit ambiant juste avant l'événement (pour isoler la voix). */
    val noiseSample: ShortArray,
) {
    val durationMs: Long get() = pcm.size * 1000L / AudioMath.SAMPLE_RATE
}

/**
 * Machine à états qui repère les sons émergeant du bruit de fond.
 *
 * - Le plancher de bruit est suivi en continu ([NoiseFloorTracker]).
 * - Un événement démarre si le niveau dépasse plancher + [marginDb] pendant
 *   au moins [onsetFrames] trames de 100 ms.
 * - On conserve [preRollMs] avant le déclenchement pour ne pas couper le
 *   premier mot, et on attend [hangoverMs] de calme avant de clore.
 * - Plus la sensibilité est haute, plus [marginDb] est petit.
 */
class SoundEventDetector(
    var marginDb: Float = 10f,
    var minLevelDb: Float = -72f,
    private val preRollMs: Int = 1500,
    private val hangoverMs: Int = 2000,
    private val maxEventMs: Int = 60_000,
    private val minEventMs: Int = 400,
    private val onsetFrames: Int = 2,
    val noiseTracker: NoiseFloorTracker = NoiseFloorTracker(),
) {
    private val frame = AudioMath.FRAME
    private val frameMs = 1000 * frame / AudioMath.SAMPLE_RATE
    private val preRollFrames = preRollMs / frameMs
    private val ring = ArrayDeque<ShortArray>()
    private val quietRing = ArrayDeque<ShortArray>()

    private var active = false
    private var current = ArrayList<ShortArray>()
    private var eventStartFrame = 0L
    private var quietFrames = 0
    private var loudFrames = 0
    /** Nombre de trames réellement fortes dans l'événement en cours. */
    private var eventLoudFrames = 0
    private var peakDb = -96f
    private var floorAtStart = -60f
    private var noiseAtStart = ShortArray(0)
    private var frameIndex = 0L

    var lastDb = -96f
        private set
    val noiseFloorDb get() = noiseTracker.floorDb
    val threshold get() = maxOf(noiseTracker.floorDb + marginDb, minLevelDb)
    val isRecording get() = active

    fun setSensitivity(level: Int) {
        // 1 = très peu sensible (+21 dB au-dessus du fond) ... 10 = très sensible (+4 dB)
        marginDb = 23f - 1.9f * level.coerceIn(1, 10)
    }

    /** Ajoute une trame de 100 ms ; renvoie un événement quand il se termine. */
    fun process(frameData: ShortArray): CapturedEvent? {
        require(frameData.size == frame)
        val db = AudioMath.rmsDb(frameData)
        lastDb = db
        val loud = db > threshold
        var finished: CapturedEvent? = null

        if (!active) {
            noiseTracker.add(db)
            ring.addLast(frameData)
            while (ring.size > preRollFrames + onsetFrames) ring.removeFirst()
            if (!loud) {
                quietRing.addLast(frameData)
                while (quietRing.size > 10) quietRing.removeFirst()
            }
            loudFrames = if (loud) loudFrames + 1 else 0
            if (loudFrames >= onsetFrames) {
                active = true
                current = ArrayList(ring)
                eventStartFrame = frameIndex - ring.size + 1
                floorAtStart = noiseTracker.floorDb
                noiseAtStart = concat(quietRing)
                peakDb = db
                quietFrames = 0
                eventLoudFrames = loudFrames
                ring.clear()
            }
        } else {
            noiseTracker.addDuringEvent()
            current.add(frameData)
            if (db > peakDb) peakDb = db
            quietFrames = if (loud) 0 else quietFrames + 1
            if (loud) eventLoudFrames++
            val tooLong = current.size * frameMs >= maxEventMs
            if (quietFrames * frameMs >= hangoverMs || tooLong) {
                finished = finish()
            }
        }
        frameIndex++
        return finished
    }

    /** Termine l'événement en cours (fin de nuit). */
    fun flush(): CapturedEvent? = if (active) finish() else null

    private fun finish(): CapturedEvent? {
        active = false
        loudFrames = 0
        // On retire la fin silencieuse, sauf 300 ms de queue.
        val trim = (quietFrames - 3).coerceAtLeast(0)
        val kept = current.subList(0, (current.size - trim).coerceAtLeast(0))
        val loudPartMs = eventLoudFrames * frameMs
        val ev = if (loudPartMs < minEventMs) null else CapturedEvent(
            startOffsetMs = eventStartFrame * frameMs,
            pcm = concat(kept),
            peakDb = peakDb,
            noiseFloorDb = floorAtStart,
            noiseSample = noiseAtStart,
        )
        current = ArrayList()
        quietFrames = 0
        return ev
    }

    private fun concat(parts: Collection<ShortArray>): ShortArray {
        val out = ShortArray(parts.sumOf { it.size })
        var p = 0
        for (a in parts) { a.copyInto(out, p); p += a.size }
        return out
    }
}
