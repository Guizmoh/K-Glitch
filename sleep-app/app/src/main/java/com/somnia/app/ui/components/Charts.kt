package com.somnia.app.ui.components

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.unit.dp
import com.somnia.app.audio.SoundType
import com.somnia.app.data.MinuteStat
import com.somnia.app.data.SoundEventEntity
import com.somnia.app.ui.theme.AwakeColor
import com.somnia.app.ui.theme.DeepColor
import com.somnia.app.ui.theme.LightColor
import com.somnia.app.ui.theme.Moon
import com.somnia.app.ui.theme.color
import com.somnia.app.ui.theme.scoreColor

/** Hypnogramme : éveillé en haut, profond en bas, événements sonores en dessous. */
@Composable
fun Hypnogram(minutes: List<MinuteStat>, events: List<SoundEventEntity>, sessionStart: Long) {
    if (minutes.isEmpty()) { Text("Pas encore de données."); return }
    val total = minutes.size.coerceAtLeast(1)
    Canvas(Modifier.fillMaxWidth().height(150.dp)) {
        val w = size.width / total
        val rowH = 30.dp.toPx()
        for (m in minutes) {
            val (row, c) = when (m.stage) { 2 -> 0 to AwakeColor; 0 -> 2 to DeepColor; else -> 1 to LightColor }
            drawRect(c, Offset(m.minute * w, row * rowH), Size(w + 0.5f, rowH))
        }
        val y0 = rowH * 3 + 8.dp.toPx()
        val evH = size.height - y0
        for (e in events) {
            val x = ((e.timestamp - sessionStart) / 60_000f) * w
            val ew = (e.durationMs / 60_000f * w).coerceAtLeast(2.dp.toPx())
            drawRect(SoundType.fromName(e.type).color(), Offset(x, y0), Size(ew, evH))
        }
    }
    Row(Modifier.padding(top = 6.dp), verticalAlignment = Alignment.CenterVertically) {
        Legend(AwakeColor, "Éveil"); Legend(LightColor, "Léger"); Legend(DeepColor, "Profond")
    }
}

@Composable
fun Legend(c: Color, label: String) {
    Canvas(Modifier.width(10.dp).height(10.dp)) { drawCircle(c) }
    Text(" $label   ", style = MaterialTheme.typography.labelSmall)
}

/** Courbe du niveau sonore moyen par minute. */
@Composable
fun NoiseCurve(minutes: List<MinuteStat>) {
    if (minutes.size < 2) return
    val min = minutes.minOf { it.avgDb }.coerceAtLeast(-90f)
    val max = minutes.maxOf { it.maxDb }.coerceAtMost(0f).coerceAtLeast(min + 10f)
    Canvas(Modifier.fillMaxWidth().height(80.dp)) {
        val w = size.width / (minutes.size - 1)
        fun y(db: Float) = size.height * (1f - ((db - min) / (max - min)).coerceIn(0f, 1f))
        for (i in 1 until minutes.size) {
            drawLine(Moon.copy(alpha = 0.35f), Offset((i - 1) * w, y(minutes[i - 1].maxDb)), Offset(i * w, y(minutes[i].maxDb)))
            drawLine(Moon, Offset((i - 1) * w, y(minutes[i - 1].avgDb)), Offset(i * w, y(minutes[i].avgDb)), strokeWidth = 2.dp.toPx())
        }
    }
}

/** Histogramme simple (une barre par nuit). */
@Composable
fun BarChart(values: List<Float>, labels: List<String>, maxValue: Float? = null, color: Color = Moon, goal: Float? = null) {
    if (values.isEmpty()) { Text("Pas encore assez de nuits."); return }
    val top = (maxValue ?: values.max()).coerceAtLeast(1f)
    Canvas(Modifier.fillMaxWidth().height(140.dp)) {
        val slot = size.width / values.size
        val bw = slot * 0.6f
        values.forEachIndexed { i, v ->
            val h = size.height * (v / top).coerceIn(0f, 1f)
            drawRoundRect(color, Offset(i * slot + (slot - bw) / 2, size.height - h), Size(bw, h), CornerRadius(4.dp.toPx()))
        }
        goal?.let {
            val y = size.height * (1 - it / top)
            drawLine(com.somnia.app.ui.theme.Themes.cur.text.copy(alpha = 0.4f), Offset(0f, y), Offset(size.width, y), strokeWidth = 1.dp.toPx())
        }
    }
    Row(Modifier.fillMaxWidth()) {
        labels.forEach { Text(it, Modifier.weight(1f), style = MaterialTheme.typography.labelSmall, maxLines = 1) }
    }
}

/** Vu-mètre en direct avec plancher de bruit et seuil de détection. */
@Composable
fun LevelMeter(levelDb: Float, floorDb: Float, thresholdDb: Float, capturing: Boolean) {
    val lo = -90f; val hi = -10f
    fun f(db: Float) = ((db - lo) / (hi - lo)).coerceIn(0f, 1f)
    Canvas(Modifier.fillMaxWidth().height(28.dp)) {
        drawRoundRect(com.somnia.app.ui.theme.Themes.cur.text.copy(alpha = 0.08f), cornerRadius = CornerRadius(8.dp.toPx()))
        drawRoundRect(
            if (capturing) Color(0xFFE08A7E) else com.somnia.app.ui.theme.Badge,
            size = Size(size.width * f(levelDb), size.height), cornerRadius = CornerRadius(8.dp.toPx()),
        )
        drawLine(Color(0xFF9E9EB8), Offset(size.width * f(floorDb), 0f), Offset(size.width * f(floorDb), size.height), 3.dp.toPx())
        drawLine(Color(0xFFFFD166), Offset(size.width * f(thresholdDb), 0f), Offset(size.width * f(thresholdDb), size.height), 3.dp.toPx())
    }
}

/** Jauge circulaire du score de sommeil. */
@Composable
fun ScoreRing(score: Int?, modifier: Modifier = Modifier) {
    Canvas(modifier) {
        val stroke = (size.minDimension / 9f)
        val inset = stroke / 2
        val arcSize = Size(size.width - stroke, size.height - stroke)
        val tl = Offset(inset, inset)
        drawArc(com.somnia.app.ui.theme.Themes.cur.text.copy(alpha = 0.1f), 135f, 270f, false, tl, arcSize, style = Stroke(stroke, cap = StrokeCap.Round))
        if ((score ?: 0) > 0) drawArc(scoreColor(score), 135f, 270f * ((score ?: 0) / 100f), false, tl, arcSize, style = Stroke(stroke, cap = StrokeCap.Round))
    }
}
