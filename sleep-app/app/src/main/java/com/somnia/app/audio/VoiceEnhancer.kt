package com.somnia.app.audio

import kotlin.math.PI
import kotlin.math.abs
import kotlin.math.cos
import kotlin.math.max
import kotlin.math.min
import kotlin.math.sqrt

/**
 * Isole la voix du bruit ambiant par soustraction spectrale :
 *  1. on mesure le spectre du bruit de fond (juste avant l'événement, ou à
 *     défaut par « statistiques minimales » sur l'enregistrement lui-même) ;
 *  2. chaque trame est atténuée là où elle ne dépasse pas ce bruit ;
 *  3. on garde surtout la bande de la voix (90 Hz – 5 kHz) ;
 *  4. on normalise le volume pour rendre audibles les chuchotements.
 */
object VoiceEnhancer {
    private const val N = 512
    private const val HOP = N / 2
    private val fft = Fft(N)
    // Fenêtre racine de Hann (analyse + synthèse => reconstruction parfaite à 50 %).
    private val window = DoubleArray(N) { sqrt(0.5 - 0.5 * cos(2 * PI * it / N)) }

    fun enhance(
        pcm: ShortArray,
        noiseSample: ShortArray? = null,
        sampleRate: Int = AudioMath.SAMPLE_RATE,
        overSubtraction: Double = 2.0,
        floorGain: Double = 0.06,
        voiceBand: Boolean = true,
    ): ShortArray {
        if (pcm.size < N) return pcm.copyOf()
        val bins = N / 2 + 1
        val noise = noiseSample?.takeIf { it.size >= N * 2 }?.let { averagePower(it) }
            ?: minimumStatistics(pcm)

        val out = DoubleArray(pcm.size + N)
        val re = DoubleArray(N)
        val im = DoubleArray(N)
        val prevGain = DoubleArray(bins) { 1.0 }
        val binHz = sampleRate.toDouble() / N

        var pos = 0
        while (pos + N <= pcm.size) {
            for (i in 0 until N) { re[i] = pcm[pos + i] / 32768.0 * window[i]; im[i] = 0.0 }
            fft.transform(re, im)
            for (k in 0 until bins) {
                val p = re[k] * re[k] + im[k] * im[k]
                var g = if (p <= 1e-12) floorGain else sqrt(max(0.0, 1.0 - overSubtraction * noise[k] / p))
                g = max(floorGain, 0.7 * g + 0.3 * prevGain[k]) // lissage : moins de « bruit musical »
                prevGain[k] = g
                if (voiceBand) {
                    val hz = k * binHz
                    if (hz < 90) g *= 0.1 else if (hz > 5000) g *= 0.3
                }
                re[k] *= g; im[k] *= g
                if (k in 1 until N / 2) { re[N - k] = re[k]; im[N - k] = -im[k] }
            }
            fft.transform(re, im, inverse = true)
            for (i in 0 until N) out[pos + i] += re[i] * window[i]
            pos += HOP
        }

        var peak = 1e-9
        for (i in pcm.indices) peak = max(peak, abs(out[i]))
        val gain = min(0.9 / peak, 30.0)
        return ShortArray(pcm.size) { (out[it] * gain * 32767).toInt().coerceIn(-32768, 32767).toShort() }
    }

    private fun framePowers(pcm: ShortArray): List<DoubleArray> {
        val re = DoubleArray(N); val im = DoubleArray(N)
        val frames = ArrayList<DoubleArray>()
        var pos = 0
        while (pos + N <= pcm.size) {
            for (i in 0 until N) { re[i] = pcm[pos + i] / 32768.0 * window[i]; im[i] = 0.0 }
            fft.transform(re, im)
            frames += DoubleArray(N / 2 + 1) { re[it] * re[it] + im[it] * im[it] }
            pos += HOP
        }
        return frames
    }

    private fun averagePower(pcm: ShortArray): DoubleArray {
        val frames = framePowers(pcm)
        return DoubleArray(N / 2 + 1) { k -> frames.sumOf { it[k] } / frames.size }
    }

    /** Bruit estimé = 15e percentile de la puissance de chaque fréquence. */
    private fun minimumStatistics(pcm: ShortArray): DoubleArray {
        val frames = framePowers(pcm)
        return DoubleArray(N / 2 + 1) { k ->
            val v = DoubleArray(frames.size) { frames[it][k] }.also { it.sort() }
            v[(v.size * 0.15).toInt().coerceAtMost(v.size - 1)] * 1.5
        }
    }
}
