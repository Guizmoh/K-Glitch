package com.somnia.app.audio

import kotlin.math.PI
import kotlin.math.abs
import kotlin.math.cos
import kotlin.math.max
import kotlin.math.min
import kotlin.math.sin

/** Conversion de fréquence d'échantillonnage et petites retouches du son enregistré. */
object Resample {
    private val taps = HashMap<Int, FloatArray>()

    /** Filtre passe-bas (sinc fenêtré) pour diviser la fréquence par [factor] sans repliement. */
    private fun lowpass(factor: Int): FloatArray = taps.getOrPut(factor) {
        val n = 16 * factor + 1
        val fc = 0.45f / factor
        val h = FloatArray(n) { i ->
            val m = i - (n - 1) / 2
            val sinc = if (m == 0) 2 * fc else (sin(2 * PI * fc * m) / (PI * m)).toFloat()
            val w = (0.42 - 0.5 * cos(2 * PI * i / (n - 1)) + 0.08 * cos(4 * PI * i / (n - 1))).toFloat()
            sinc * w
        }
        val sum = h.sum(); FloatArray(n) { h[it] / sum }
    }

    /** Divise la fréquence par un entier (48 kHz -> 16 kHz : facteur 3). */
    fun down(pcm: ShortArray, factor: Int): ShortArray {
        if (factor <= 1) return pcm
        val h = lowpass(factor); val half = h.size / 2
        return ShortArray(pcm.size / factor) { o ->
            val c = o * factor
            var acc = 0f
            for (k in h.indices) {
                val i = c + k - half
                if (i in pcm.indices) acc += pcm[i] * h[k]
            }
            acc.toInt().coerceIn(-32768, 32767).toShort()
        }
    }

    /** Ramène un son à 16 kHz (pour la transcription et l'empreinte vocale). */
    fun to16k(pcm: ShortArray, rate: Int): ShortArray = when {
        rate == AudioMath.SAMPLE_RATE -> pcm
        rate % AudioMath.SAMPLE_RATE == 0 -> down(pcm, rate / AudioMath.SAMPLE_RATE)
        else -> { // cas rare : interpolation linéaire
            val ratio = rate.toDouble() / AudioMath.SAMPLE_RATE
            ShortArray((pcm.size / ratio).toInt()) { pcm[min(pcm.size - 1, (it * ratio).toInt())] }
        }
    }

    /**
     * Retouche douce de l'enregistrement original : retire le continu et le grondement
     * (< ~70 Hz), remonte le volume (jusqu'à x4) sans saturer, et adoucit le début
     * et la fin pour éviter les « clics ».
     */
    fun polish(pcm: ShortArray, rate: Int): ShortArray = polishWith(pcm, rate).first

    /** Comme [polish] ; [fixedGain] réutilise le gain d'un morceau précédent. Renvoie aussi le gain. */
    fun polishWith(
        pcm: ShortArray, rate: Int, fixedGain: Float? = null, fadeIn: Boolean = true, fadeOut: Boolean = true,
    ): Pair<ShortArray, Float> {
        if (pcm.isEmpty()) return pcm to 1f
        val a = (1.0 / (1.0 + 2 * PI * 70.0 / rate)).toFloat()
        val out = FloatArray(pcm.size)
        var prevX = pcm[0].toFloat(); var prevY = 0f
        for (i in pcm.indices) {
            val x = pcm[i].toFloat()
            val y = a * (prevY + x - prevX)
            out[i] = y; prevX = x; prevY = y
        }
        var peak = 1f
        for (v in out) peak = max(peak, abs(v))
        val gain = fixedGain ?: min(4f, 0.89f * 32767f / peak)
        val fade = min(pcm.size / 4, rate / 100) // 10 ms
        return ShortArray(pcm.size) { i ->
            val f = when {
                fadeIn && i < fade -> i / fade.toFloat()
                fadeOut && i >= pcm.size - fade -> (pcm.size - 1 - i) / fade.toFloat()
                else -> 1f
            }
            (out[i] * gain * f).toInt().coerceIn(-32768, 32767).toShort()
        } to gain
    }
}
