package com.somnia.app.ui.components

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.runtime.setValue
import androidx.compose.runtime.getValue
import androidx.compose.ui.text.drawText
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.foundation.gestures.detectHorizontalDragGestures
import androidx.compose.foundation.gestures.detectTapGestures
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

/**
 * Hypnogramme : éveillé en haut, profond en bas, sons en dessous, heures en bas.
 * Toucher ou glisser dessus affiche le détail de ce moment de la nuit.
 */
@Composable
fun Hypnogram(minutes: List<MinuteStat>, events: List<SoundEventEntity>, sessionStart: Long, initialSelected: Int? = null) {
    if (minutes.isEmpty()) { Text("Pas encore de données."); return }
    val total = minutes.size.coerceAtLeast(1)
    var selected by androidx.compose.runtime.remember(sessionStart) { androidx.compose.runtime.mutableStateOf(initialSelected) }
    val measurer = androidx.compose.ui.text.rememberTextMeasurer()
    val labelStyle = MaterialTheme.typography.labelSmall.copy(color = com.somnia.app.ui.theme.TextSoft)
    val cursor = com.somnia.app.ui.theme.Themes.cur.text
    // Une graduation par heure (toutes les 2 h si la nuit est longue)
    val cal = java.util.Calendar.getInstance().apply {
        timeInMillis = sessionStart; set(java.util.Calendar.MINUTE, 0); set(java.util.Calendar.SECOND, 0); set(java.util.Calendar.MILLISECOND, 0)
        add(java.util.Calendar.HOUR_OF_DAY, 1)
    }
    val step = if (total > 9 * 60) 2 else 1
    val ticks = generateSequence(cal.timeInMillis) { it + step * 3_600_000L }
        .takeWhile { it < sessionStart + total * 60_000L }.toList()

    Canvas(
        Modifier.fillMaxWidth().height(172.dp)
            .pointerInput(total) { detectTapGestures { selected = ((it.x / size.width) * total).toInt().coerceIn(0, total - 1) } }
            .pointerInput(total) {
                detectHorizontalDragGestures { ch, _ -> selected = ((ch.position.x / size.width) * total).toInt().coerceIn(0, total - 1) }
            },
    ) {
        val w = size.width / total
        val rowH = 30.dp.toPx()
        for (m in minutes) {
            val (row, c) = when (m.stage) { 2 -> 0 to AwakeColor; 0 -> 2 to DeepColor; else -> 1 to LightColor }
            drawRect(c, Offset(m.minute * w, row * rowH), Size(w + 0.5f, rowH))
        }
        val y0 = rowH * 3 + 8.dp.toPx()
        val evH = 40.dp.toPx()
        for (e in events) {
            val x = ((e.timestamp - sessionStart) / 60_000f) * w
            val ew = (e.durationMs / 60_000f * w).coerceAtLeast(2.dp.toPx())
            drawRect(SoundType.fromName(e.type).color(), Offset(x, y0), Size(ew, evH))
        }
        // Heures
        val yt = y0 + evH + 4.dp.toPx()
        for (t in ticks) {
            val x = ((t - sessionStart) / 60_000f) * w
            drawLine(labelStyle.color.copy(alpha = 0.5f), Offset(x, 0f), Offset(x, yt), 1.dp.toPx())
            val label = measurer.measure(com.somnia.app.ui.Fmt.time(t).substringBefore(":") + " h", labelStyle)
            val lx = (x - label.size.width / 2f).coerceIn(0f, size.width - label.size.width)
            drawText(label, topLeft = Offset(lx, yt))
        }
        selected?.let { m ->
            val x = (m + 0.5f) * w
            drawLine(cursor, Offset(x, 0f), Offset(x, y0 + evH), 2.dp.toPx())
        }
    }
    Row(Modifier.padding(top = 6.dp), verticalAlignment = Alignment.CenterVertically) {
        Legend(AwakeColor, "Éveil"); Legend(LightColor, "Léger"); Legend(DeepColor, "Profond")
    }
    val sel = selected
    if (sel == null) {
        Text("Touchez le graphique pour voir le détail d'un moment.", style = MaterialTheme.typography.labelSmall, color = com.somnia.app.ui.theme.TextSoft)
    } else {
        MinuteDetail(minutes, events, sessionStart, sel)
    }
}

/** Détail d'une minute de la nuit : heure, phase, bruit, mouvements, sons proches. */
@Composable
private fun MinuteDetail(minutes: List<MinuteStat>, events: List<SoundEventEntity>, sessionStart: Long, minute: Int) {
    val m = minutes.getOrNull(minute) ?: return
    val t = sessionStart + minute * 60_000L
    val stage = when (m.stage) { 2 -> "Éveil" to AwakeColor; 0 -> "Sommeil profond" to DeepColor; else -> "Sommeil léger" to LightColor }
    val mv = minutes.map { it.movement }.sorted()
    val med = mv[mv.size / 2].coerceAtLeast(0.1f)
    val movement = when {
        m.movement <= 0.05f -> "aucun"
        m.movement < med * 1.5f -> "calme"
        m.movement < med * 3f -> "quelques mouvements"
        else -> "agité"
    }
    val floor = minutes.map { it.avgDb }.sorted()[minutes.size / 5]
    val near = events.filter { kotlin.math.abs(it.timestamp - t) <= 3 * 60_000L || (t >= it.timestamp && t <= it.timestamp + it.durationMs) }
    Column(
        Modifier.fillMaxWidth().padding(top = 8.dp)
            .background(com.somnia.app.ui.theme.Themes.cur.text.copy(alpha = 0.07f), androidx.compose.foundation.shape.RoundedCornerShape(12.dp))
            .padding(12.dp),
    ) {
        Row(verticalAlignment = Alignment.CenterVertically) {
            Text(com.somnia.app.ui.Fmt.time(t), style = MaterialTheme.typography.titleLarge)
            Text("   ${stage.first}", color = stage.second, style = MaterialTheme.typography.titleMedium)
        }
        Text(
            "Bruit : %+.0f dB par rapport au calme de la nuit (pic %+.0f dB)".format(m.avgDb - floor, m.maxDb - floor),
            style = MaterialTheme.typography.bodySmall,
        )
        Text("Mouvements : $movement", style = MaterialTheme.typography.bodySmall)
        if (near.isEmpty()) Text("Aucun son relevé autour de ce moment.", style = MaterialTheme.typography.bodySmall, color = com.somnia.app.ui.theme.TextSoft)
        else near.take(4).forEach { e ->
            val ty = SoundType.fromName(e.type)
            Text(
                "• ${com.somnia.app.ui.Fmt.time(e.timestamp)} : ${ty.label} (${com.somnia.app.ui.Fmt.seconds(e.durationMs)})" +
                    (e.transcript?.let { " « $it »" } ?: ""),
                style = MaterialTheme.typography.bodySmall, color = ty.color(), maxLines = 2,
            )
        }
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
    // Échelle centrée sur le bruit de fond (de -10 à +35 dB) : on voit bien le seuil bouger
    val lo = floorDb - 10f; val hi = floorDb + 35f
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


/** Courbe du ronflement : une barre par tranche de 10 min (hauteur = durée, couleur = intensité), heures en bas. */
@Composable
fun SnoreCurve(events: List<SoundEventEntity>, sessionStart: Long, totalMinutes: Int) {
    val curve = com.somnia.app.analysis.Snoring.curve(events, sessionStart, totalMinutes)
    if (curve.all { it.seconds == 0f }) { Text("Pas de ronflement relevé cette nuit.", color = com.somnia.app.ui.theme.TextSoft); return }
    val measurer = androidx.compose.ui.text.rememberTextMeasurer()
    val labelStyle = MaterialTheme.typography.labelSmall.copy(color = com.somnia.app.ui.theme.TextSoft)
    val base = SoundType.SNORING.color()
    val maxSec = curve.maxOf { it.seconds }.coerceAtLeast(30f)
    val cal = java.util.Calendar.getInstance().apply {
        timeInMillis = sessionStart; set(java.util.Calendar.MINUTE, 0); set(java.util.Calendar.SECOND, 0); set(java.util.Calendar.MILLISECOND, 0)
        add(java.util.Calendar.HOUR_OF_DAY, 1)
    }
    val step = if (totalMinutes > 9 * 60) 2 else 1
    val ticks = generateSequence(cal.timeInMillis) { it + step * 3_600_000L }.takeWhile { it < sessionStart + totalMinutes * 60_000L }.toList()
    Canvas(Modifier.fillMaxWidth().height(120.dp)) {
        val chartH = size.height - 18.dp.toPx()
        val slot = size.width / curve.size
        curve.forEachIndexed { i, b ->
            if (b.seconds <= 0f) return@forEachIndexed
            val h = chartH * (b.seconds / maxSec).coerceIn(0.04f, 1f)
            val strength = (b.intensityDb / 30f).coerceIn(0.35f, 1f)
            drawRoundRect(base.copy(alpha = strength), Offset(i * slot + slot * 0.12f, chartH - h), Size(slot * 0.76f, h), CornerRadius(3.dp.toPx()))
        }
        for (t in ticks) {
            val x = ((t - sessionStart) / 60_000f) / totalMinutes * size.width
            val label = measurer.measure(com.somnia.app.ui.Fmt.time(t).substringBefore(":") + " h", labelStyle)
            drawText(label, topLeft = Offset((x - label.size.width / 2f).coerceIn(0f, size.width - label.size.width), chartH + 2.dp.toPx()))
        }
    }
}
