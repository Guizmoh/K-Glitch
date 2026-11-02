package com.somnia.app.audio

import kotlin.math.log10
import kotlin.math.max
import kotlin.math.sqrt

object AudioMath {
    const val SAMPLE_RATE = 16_000
    /** Trame d'analyse : 100 ms. */
    const val FRAME = SAMPLE_RATE / 10

    /** Niveau RMS en dBFS (0 = pleine échelle, ~-90 = silence numérique). */
    fun rmsDb(buf: ShortArray, offset: Int = 0, length: Int = buf.size): Float {
        if (length <= 0) return -96f
        var sum = 0.0
        for (i in offset until offset + length) {
            val s = buf[i] / 32768.0
            sum += s * s
        }
        val rms = sqrt(sum / length)
        return (20 * log10(max(rms, 1.6e-5))).toFloat()
    }

    fun toFloat(buf: ShortArray, offset: Int = 0, length: Int = buf.size): FloatArray =
        FloatArray(length) { buf[offset + it] / 32768f }
}
