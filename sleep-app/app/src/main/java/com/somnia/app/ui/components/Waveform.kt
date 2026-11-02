package com.somnia.app.ui.components

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.gestures.detectHorizontalDragGestures
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.produceState
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.unit.dp
import com.somnia.app.audio.WavFile
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.io.File
import kotlin.math.abs

/** Pics d'un fichier WAV, réduits à [bins] barres (0..1). */
fun wavePeaks(path: String, bins: Int = 90): FloatArray {
    val pcm = runCatching { WavFile.read(File(path)) }.getOrNull() ?: return FloatArray(0)
    if (pcm.isEmpty()) return FloatArray(0)
    val per = maxOf(1, pcm.size / bins)
    val out = FloatArray(minOf(bins, pcm.size)) { b ->
        var m = 0
        val end = minOf(pcm.size, (b + 1) * per)
        for (i in b * per until end) m = maxOf(m, abs(pcm[i].toInt()))
        m.toFloat()
    }
    val max = out.maxOrNull()?.takeIf { it > 0 } ?: 1f
    // racine : les passages faibles restent visibles à côté d'un gros pic
    return FloatArray(out.size) { kotlin.math.sqrt(out[it] / max) }
}

/**
 * Forme d'onde d'un enregistrement avec la position de lecture.
 * Toucher ou glisser dessus appelle [onSeek] avec une fraction 0..1.
 */
@Composable
fun Waveform(path: String, progress: Float, color: Color, onSeek: (Float) -> Unit, modifier: Modifier = Modifier) {
    val peaks by produceState(FloatArray(0), path) { value = withContext(Dispatchers.IO) { wavePeaks(path) } }
    Canvas(
        modifier.fillMaxWidth().height(64.dp)
            .pointerInput(path) { detectTapGestures { onSeek((it.x / size.width).coerceIn(0f, 1f)) } }
            .pointerInput(path) { detectHorizontalDragGestures { ch, _ -> onSeek((ch.position.x / size.width).coerceIn(0f, 1f)) } },
    ) {
        val n = peaks.size
        if (n == 0) return@Canvas
        val slot = size.width / n
        val bw = (slot * 0.62f).coerceAtLeast(1.5f)
        val mid = size.height / 2
        for (i in 0 until n) {
            val h = (peaks[i] * size.height * 0.92f).coerceAtLeast(2.dp.toPx())
            val played = (i + 0.5f) / n <= progress
            drawRoundRect(
                if (played) color else color.copy(alpha = 0.32f),
                Offset(i * slot + (slot - bw) / 2, mid - h / 2), Size(bw, h), CornerRadius(bw / 2),
            )
        }
        if (progress > 0f) {
            val x = size.width * progress
            drawLine(Color.White, Offset(x, 0f), Offset(x, size.height), 2.dp.toPx())
        }
    }
}
