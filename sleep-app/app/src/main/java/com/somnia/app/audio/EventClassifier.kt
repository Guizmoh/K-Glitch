package com.somnia.app.audio

/** Résultat de la classification d'un événement complet. */
data class Classification(
    val type: SoundType,
    val confidence: Float,
    val scores: SoundScores,
)

/**
 * Découpe un événement en fenêtres de ~1 s (recouvrement 50 %), les fait
 * classer, puis agrège : pour chaque catégorie on garde la moyenne des deux
 * meilleures fenêtres, ce qui évite qu'un mot isolé dans un long ronflement
 * passe inaperçu sans pour autant se fier à une seule fenêtre douteuse.
 *
 * [minConfidence] fixe, par catégorie, le score minimum exigé : c'est le
 * réglage « intensité de captation » des rires, pleurs, etc.
 */
class EventClassifier(
    private val classifier: SoundClassifier,
    var minConfidence: Map<SoundType, Float> = defaultThresholds(),
) {
    fun classify(pcm: ShortArray): Classification {
        val win = SoundClassifier.WINDOW
        val hop = win / 2
        val windows = ArrayList<FloatArray>()
        if (pcm.size <= win) {
            val f = FloatArray(win)
            for (i in pcm.indices) f[i] = pcm[i] / 32768f
            windows += f
        } else {
            var p = 0
            while (p < pcm.size) {
                val f = FloatArray(win)
                val n = minOf(win, pcm.size - p)
                for (i in 0 until n) f[i] = pcm[p + i] / 32768f
                windows += f
                if (p + win >= pcm.size) break
                p += hop
            }
        }
        val perType = HashMap<SoundType, MutableList<Float>>()
        for (w in windows) {
            for ((t, s) in classifier.classify(w)) perType.getOrPut(t) { ArrayList() }.add(s)
        }
        val agg = perType.mapValues { (_, l) ->
            val top = l.sortedDescending().take(2)
            top.average().toFloat()
        }
        return decide(agg)
    }

    fun decide(agg: SoundScores): Classification {
        // Télé / musique : si c'est le score le plus fort, le son est considéré comme du fond
        val media = agg[SoundType.MEDIA] ?: 0f
        val bestOther = agg.filterKeys { it != SoundType.MEDIA }.maxOfOrNull { it.value } ?: 0f
        if (media >= 0.3f && media >= bestOther) return Classification(SoundType.MEDIA, media, agg)
        // Ordre de priorité si deux catégories passent le seuil avec des
        // scores proches : un cri ou des pleurs priment sur la parole.
        val priority = listOf(
            SoundType.NIGHT_TERROR, SoundType.CRYING, SoundType.LAUGHTER,
            SoundType.SPEECH, SoundType.SNORING, SoundType.COUGH, SoundType.BREATHING,
        )
        val passing = priority.filter { (agg[it] ?: 0f) >= (minConfidence[it] ?: 0.3f) }
        if (passing.isEmpty()) {
            val best = agg.maxOfOrNull { it.value } ?: 0f
            return Classification(SoundType.NOISE, 1f - best, agg)
        }
        val top = passing.maxOf { agg[it] ?: 0f }
        val chosen = passing.first { (agg[it] ?: 0f) >= top * 0.75f }
        return Classification(chosen, agg[chosen] ?: 0f, agg)
    }

    companion object {
        fun defaultThresholds(): Map<SoundType, Float> = mapOf(
            SoundType.SPEECH to 0.30f,
            SoundType.NIGHT_TERROR to 0.25f,
            SoundType.SNORING to 0.25f,
            SoundType.LAUGHTER to 0.25f,
            SoundType.CRYING to 0.25f,
            SoundType.COUGH to 0.35f,
            SoundType.BREATHING to 0.40f,
        )

        /** Sensibilité 1..10 -> score minimum (1 => 0,60 ; 10 => 0,08). */
        fun thresholdFromLevel(level: Int): Float = 0.6f - (level.coerceIn(1, 10) - 1) * 0.0578f
    }
}
